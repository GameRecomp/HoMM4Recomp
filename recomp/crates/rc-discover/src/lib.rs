// SPDX-License-Identifier: MIT
//! Code discovery for the Cultures recompiler (milestone M2).
//!
//! The executables have no relocations, so code and data have to be told apart by analysis:
//!
//! 1. Recursive descent from the trusted roots (entry point, exports, manifest seeds), with
//!    call and branch following, import calls through the IAT and `switch` jump tables
//!    (`jmp [idx*4 + table]`, bounded by `cmp`/`ja` or `and`, with MSVC's index byte tables).
//! 2. Candidates, each verified by trial decoding in a transaction that is rolled back on any
//!    implausible instruction or overlap: 32-bit immediates in code that point into code,
//!    SEH scope tables (`__except_handler3` frames), C++ EH `FuncInfo` records, vtables (code
//!    pointer runs stored by `mov [obj], imm32`), and aligned constants in the data sections
//!    (for DLLs: exactly the relocated dwords).
//! 3. Gap filling: padding (`int3`, `nop` and MSVC's multi-byte nops), then leftover code
//!    that nothing references directly.
//! 4. Partition into function bodies, each instruction owned by one function: a successor
//!    that is another entry is a tail transfer, and where one function's code runs into code
//!    another function also reaches, that point becomes an entry of its own (a cross jump).
//!    No-return functions are a fixed point over the bodies. Discovery repeats until neither
//!    changes, so code after calls to `exit`-like functions is not decoded.
//!
//! The rules for images without relocations follow TropicoRecomp's `rc-discover` (Tropico 2
//! has none either): speculative code that calls or runs into the middle of known code is junk.
//!
//! The result is a [`CodeMap`]: functions with their byte ranges, jump tables, vtables,
//! labels, data in code and the coverage report.

mod codemap;
mod diff;
mod engine;
mod image;

pub use codemap::{CodeMap, Flow, Function, Insn, JumpTable, Label, SectionCoverage, Va};
pub use diff::{diff, fingerprints, Diff};
pub use engine::{Options, Source};
pub use image::{Image, Section};

use std::collections::{BTreeSet, HashSet};

/// Run discovery on a mapped (and, for DLLs, possibly rebased) image.
///
/// A pointer into the middle of real code can decode as a plausible function before the code
/// around it is found, and then blocks that code. When the first pass shows such pointers
/// (functions found only through a pointer whose bytes made other candidates fail), a second
/// pass leaves them out; its result is kept only if it classifies more of the code sections.
pub fn discover(img: &Image, opts: &Options, name: &str, sha256: &str) -> CodeMap {
    let (map, suspects) = discover_with(img, opts, name, sha256, &HashSet::new());
    if suspects.is_empty() {
        return map;
    }
    let (second, _) = discover_with(img, opts, name, sha256, &suspects);
    let unknown = |m: &CodeMap| m.coverage.iter().map(|c| u64::from(c.unknown)).sum::<u64>();
    if unknown(&second) < unknown(&map) {
        second
    } else {
        map
    }
}

/// Rounds repeat until neither the no-return set nor the cross-jump entries change: a
/// round decodes without falling through calls to the no-return functions known so far,
/// and starts the joins found by the previous round's partition as entries.
fn discover_with(
    img: &Image,
    opts: &Options,
    name: &str,
    sha256: &str,
    deny: &HashSet<u32>,
) -> (CodeMap, HashSet<u32>) {
    let mut no_return: HashSet<u32> = opts.no_return.iter().copied().collect();
    let mut joins = BTreeSet::new();
    let mut rounds = 0u32;
    loop {
        rounds += 1;
        let mut e = engine::Engine::new(img, opts, no_return.clone(), &joins, deny);
        e.run_all();
        let part = e.partition();
        let grown = part.no_return.iter().any(|f| !no_return.contains(f));
        let new_joins = !part.joins.is_subset(&joins);
        if (!grown && !new_joins) || rounds >= 16 {
            let suspects = e.blocking_pointers();
            return (
                codemap::build(&e, &part, name, sha256, rounds, opts.use_relocs),
                suspects,
            );
        }
        no_return.extend(part.no_return.iter().copied());
        let found = part.joins.clone();
        drop(e);
        joins.extend(found);
    }
}

#[cfg(test)]
mod tests;
