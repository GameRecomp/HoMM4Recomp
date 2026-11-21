/* SPDX-License-Identifier: MIT */
/*
 * ole32.dll: COM with the classes HLE modules register (rc_com_register).
 * CoInitialize and CoUninitialize succeed; CoCreateInstance of any other
 * class finds none (what a program sees on a PC without the component) and
 * logs which class was asked for.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "k32.h"
#include "rc/log.h"
#include "rc/process.h"

#define OLE "ole32.dll"
#define S_OK 0u
#define REGDB_E_CLASSNOTREG 0x80040154u

static void k_CoInitialize(rc_cpu *c) { RC_RETURN(c, S_OK, 1); }

static void k_CoUninitialize(rc_cpu *c) { RC_RETURN(c, 0, 0); }

/* CoInitializeEx(reserved, model): as CoInitialize; the threading model does not matter here. */
static void k_CoInitializeEx(rc_cpu *c) { RC_RETURN(c, S_OK, 2); }

static void guid(uint32_t p, char *out, size_t n)
{
    snprintf(out, n, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", rc_ld32(p), rc_ld16(p + 4),
             rc_ld16(p + 6), rc_ld8(p + 8), rc_ld8(p + 9), rc_ld8(p + 10), rc_ld8(p + 11), rc_ld8(p + 12),
             rc_ld8(p + 13), rc_ld8(p + 14), rc_ld8(p + 15));
}

#define CLASSES 16
static struct {
    char clsid[40];
    rc_com_create create;
} classes[CLASSES];
static int nclasses;

void rc_com_register(const char *clsid, rc_com_create create)
{
    if (nclasses < CLASSES) {
        snprintf(classes[nclasses].clsid, sizeof classes[nclasses].clsid, "%s", clsid);
        classes[nclasses].create = create;
        nclasses++;
    }
}

static void k_CoCreateInstance(rc_cpu *c)
{
    uint32_t clsid = RC_ARG(c, 0), iid = RC_ARG(c, 3), out = RC_ARG(c, 4);
    char a[48], b[48];
    guid(clsid, a, sizeof a);
    guid(iid, b, sizeof b);
    for (int i = 0; i < nclasses; i++) {
        if (strcmp(classes[i].clsid, a) == 0) {
            uint32_t obj = classes[i].create(c, iid);
            if (out) {
                rc_st32(out, obj);
            }
            RC_RETURN(c, obj ? S_OK : 0x80004002u /* E_NOINTERFACE */, 5);
        }
    }
    rc_log("ole32: no class %s (interface %s)", a, b);
    if (out) {
        rc_st32(out, 0);
    }
    RC_RETURN(c, REGDB_E_CLASSNOTREG, 5);
}

/* CoCreateGuid(guid): a version-4 GUID from a counter mixed with the clock (unique within a
 * run and across runs, which is all programs use it for). */
static void k_CoCreateGuid(rc_cpu *c)
{
    static uint64_t n;
    uint64_t t = rc_ticks_ms() * 0x9e3779b97f4a7c15ull + ++n * 0xbf58476d1ce4e5b9ull + (uint64_t)time(NULL);
    uint32_t g = RC_ARG(c, 0);
    for (int i = 0; i < 16; i++) {
        t ^= t >> 31;
        t *= 0x94d049bb133111ebull;
        rc_st8(g + (uint32_t)i, (uint8_t)(t >> 56));
    }
    rc_st8(g + 7, (uint8_t)((rc_ld8(g + 7) & 0x0f) | 0x40));
    rc_st8(g + 8, (uint8_t)((rc_ld8(g + 8) & 0x3f) | 0x80));
    RC_RETURN(c, S_OK, 1);
}

const rc_import_def rc_ole32_defs[] = {
    {OLE, "CoCreateGuid", k_CoCreateGuid},
    {OLE, "CoInitialize", k_CoInitialize},
    {OLE, "CoInitializeEx", k_CoInitializeEx},
    {OLE, "CoUninitialize", k_CoUninitialize},
    {OLE, "CoCreateInstance", k_CoCreateInstance},
};
const uint32_t rc_ole32_count = sizeof rc_ole32_defs / sizeof rc_ole32_defs[0];
