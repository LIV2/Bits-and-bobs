// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Matthew Harlum <matt@harlum.net>
 *
 * BootSelectWB
 *
 * A ROM Module to select the relevant WB partition depending on the Kickstart version.
 *
 * Requires n partitions, with specific device names.  No-op if only one partition or naming doesn't match. Names:
 * - WB_1.3
 * - WB_2.X
 * - WB_3.0
 * - WB_3.1
 * - WB_3.2
 *
 * The module will make the other WB partitions non-bootable, i.e if booting Kick 2.0, WB 1.3 will be made non bootable.
 *
 * LICENSE: GPL 2.0 Only
 */
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <dos/filehandler.h>

#define STR(s) #s      /* Turn s into a string literal without expanding macro definitions (however, \
                          if invoked from a macro, macro arguments are expanded). */
#define XSTR(s) STR(s) /* Turn s into a string literal after macro-expanding it. */

#define MOD_VERSION 40
#define MOD_PRIORITY -50    // Do not change! must run after romboot but before strap!

#ifdef __INTELLISENSE__
// Stop VSCode from flipping out about asm reg arguments
#define asm(x)
#endif

asm("romtag:                                \n"
    "       dc.w    "XSTR(RTC_MATCHWORD)"   \n"
    "       dc.l    romtag                  \n"
    "       dc.l    _endskip                \n"
    "       dc.b    "XSTR(RTF_COLDSTART)"   \n"
    "       dc.b    "XSTR(MOD_VERSION)"     \n"
    "       dc.b    "XSTR(NT_UNKNOWN)"      \n"
    "       dc.b    "XSTR(MOD_PRIORITY)"    \n"
    "       dc.l    _modname                \n"
    "       dc.l    _modname                \n"
    "       dc.l    _init                   \n");

enum bootNodeID
{
    BootNodeWB13 = 0,
    BootNodeWB2x,
    BootNodeWB30,
    BootNodeWB31,
    BootNodeWB314,
    BootNodeWB32,
    BootNodeMax
};

const char modname[] = "bootselectwb";
const char * const partName[BootNodeMax] = {"\6WB_1.3", "\6WB_2.X", "\6WB_3.0", "\6WB_3.1", "\8WB_3.1.4", "\6WB_3.2"};
const UWORD kickVers[BootNodeMax] = {34, 36, 39, 40, 46, 47};

/**
 * toLower
 *
 * Pretty self explanatory
 *
 * @param c char to return in lowercase
 * @returns lowercase char
 */
char toLower(char c) {
    if (c >= 'A' && c <= 'Z')
        c |= 0x20;

    return c;
}

/**
 * compareBstr
 *
 * Compare two bstrings in a case-insensitive manner
 * @param str1 Pointer to String 1
 * @param str2 Pointer to String 2
 * @returns boolean result
 */
BOOL compareBstr(UBYTE *str1, UBYTE *str2) {
    UBYTE str1Len = str1[0];

    if (str2[0] != str1Len) {
        return FALSE;
    }
    for (int i=1; i <= str1Len; i++) {
        if (toLower(str1[i]) != toLower(str2[i])) {
            return FALSE;
        }
    }

    return TRUE;

}

/**
 * demotePart
 *
 * Set the boot priority of a given BootNode to -128, marking it as unbootable
 *
 * @param mountList Pointer to ExpansionBase->MountList
 * @param bn Pointer to the BootNode you wish to demote
 */
void demotePart(struct List *mountList, struct BootNode *bn) {
    struct ExecBase *SysBase = *(struct ExecBase **)4UL;

    struct DeviceNode        *dn   = bn->bn_DeviceNode;
    struct FileSysStartupMsg *fssm = BADDR(dn->dn_Startup);
    struct DosEnvec          *de   = BADDR(fssm->fssm_Environ);

    Forbid();

    Remove((struct Node *)bn);
    bn->bn_Node.ln_Pri = -128;
    de->de_BootPri     = -128;
    Enqueue((struct List *)mountList,(struct Node *)bn);

    Permit();
}

static struct Library __attribute__((used)) * init(BPTR seg_list asm("a0"))
{
    struct ExecBase *SysBase = *(struct ExecBase **)4UL;
    struct ExpansionBase *ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",0);

    if (ExpansionBase) {
        struct List *mountList = &ExpansionBase->MountList;

        struct BootNode   *bn      = NULL;
        struct BootNode   *bnodes[BootNodeMax] = {};
        UWORD bnodesSet = 0;
        struct DeviceNode *dn;
        enum bootNodeID id;

        for (bn = (struct BootNode *)mountList->lh_Head;
             bn->bn_Node.ln_Succ != NULL;
             bn = (struct BootNode *)bn->bn_Node.ln_Succ)
        {
            dn = bn->bn_DeviceNode;

            for (id = BootNodeWB13; id < BootNodeMax; id++)
            {
                if (compareBstr((char *)partName[id],(char *)BADDR(dn->dn_Name))) {
                    bnodes[id] = bn;
                    bnodesSet++;
                    break;
                }
            }
        }

        if (bnodesSet > 1)
        {
            for (id = BootNodeWB13; id < BootNodeMax; id++)
            {
                if ((SysBase->SoftVer != kickVers[id]) && (bnodes[id] != NULL))
                  demotePart(mountList, bnodes[id]);
            }
        }

        CloseLibrary((struct Library *)ExpansionBase);
    }

    return NULL;
}

