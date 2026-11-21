// SPDX-License-Identifier: MIT
//! Code discovery on the extracted Heroes IV binaries in `../orig/heroes4/<variant>/`
//! (not available in CI): `cargo test --release -p rc-cli -- --ignored`.
//!
//! The M2 exit criterion, with the manifest hints applied: every byte of every code section
//! classified as code, data or padding, no unresolved jump tables, every `no-return?` candidate
//! settled by a hint, and for the DLLs a result that agrees with their base relocations even
//! when discovery does not use them.

use rc_discover::{Image, Options};
use std::path::Path;

fn repo() -> &'static Path {
    Path::new(concat!(env!("CARGO_MANIFEST_DIR"), "/../../.."))
}

#[test]
#[ignore = "needs the extracted game files in orig/heroes4/"]
fn heroes4_binaries_are_discovered() {
    let ms = rc_manifest::load_dir(&repo().join("games")).expect("manifests");
    let c1 = ms.iter().find(|m| m.game.id == "heroes4").expect("heroes4");
    let mut checked = 0;
    for b in c1
        .binaries
        .iter()
        .filter(|b| b.treatment == rc_manifest::Treatment::Lift)
    {
        let v = &b.variants[0];
        let path = repo().join("orig/heroes4").join(v).join(&b.name);
        let Ok(data) = std::fs::read(&path) else {
            eprintln!("skip {} (not extracted)", path.display());
            continue;
        };
        assert_eq!(
            rc_manifest::sha256_hex(&data),
            b.sha256,
            "{}",
            path.display()
        );
        let pe = rc_pe::parse(&data).expect("pe");
        let img = Image::load(&data, &pe, b.base).expect("image");
        let hints = b.discover.clone().unwrap_or_default();
        for use_relocs in [true, false] {
            let opts = Options {
                seeds: hints.seeds.clone(),
                no_return: hints.no_return.clone(),
                returns: hints.returns.clone(),
                data: hints.data.iter().map(|r| (r.start, r.end)).collect(),
                jump_tables: hints
                    .jump_tables
                    .iter()
                    .map(|t| (t.at, t.table, t.count))
                    .collect(),
                use_relocs,
                ..Options::default()
            };
            let m = rc_discover::discover(&img, &opts, &b.name, &b.sha256);
            let what = format!("{v}/{} (relocs used: {use_relocs})", b.name);
            for c in &m.coverage {
                assert_eq!(
                    c.unknown, 0,
                    "{what}: {} has {} unknown bytes",
                    c.section, c.unknown
                );
            }
            assert!(
                m.no_return_candidates.is_empty(),
                "{what}: unchecked no-return candidates {:?}",
                m.no_return_candidates
            );
            assert!(m.warnings.is_empty(), "{what}: {:?}", m.warnings);
            assert!(
                m.jump_tables.iter().all(|t| !t.targets.is_empty()),
                "{what}: empty jump table"
            );
            if let Some(r) = &m.reloc_check {
                assert!(r.bad_targets.is_empty(), "{what}: {:?}", r.bad_targets);
                assert!(r.bad_sites.is_empty(), "{what}: {:?}", r.bad_sites);
            }
            if !img.is_dll {
                assert!(
                    m.functions.len() > 60000,
                    "{what}: {} functions",
                    m.functions.len()
                );
                break;
            }
        }
        checked += 1;
    }
    assert!(checked > 0, "nothing extracted in orig/heroes4/");
}
