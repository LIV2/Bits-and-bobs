// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Matthew Harlum <matt@harlum.net>
 *
 * BootSelectWB
 * 
 * A ROM Module to select the relevant WB partition depending on the Kickstart version
 * 
 * Requires 2 partitions, one with a device name of WB_1.3 and one with WB_2.X
 * 
 * The module will make the opposite WB partition non-bootable, i.e if booting Kick 2.0 or up, WB 1.3 will be made non bootable
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


const char modname[] = "bootselectwb";
const char partName_1[] = "\6WB_1.3";
const char partName_2[] = "\6WB_2.X";

/**
 * toLower
 *
 * Pretty self explanatory
 *
 * @param c char to return in lowercase
 * @returns lowecase char
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
        struct BootNode   *bn_wb13 = NULL;
        struct BootNode   *bn_wb2x = NULL;

        struct DeviceNode *dn;

        for (bn = (struct BootNode *)mountList->lh_Head;
             bn->bn_Node.ln_Succ != NULL;
             bn = (struct BootNode *)bn->bn_Node.ln_Succ)
        {

            dn = bn->bn_DeviceNode;

            if (compareBstr((char *)&partName_1,(char *)BADDR(dn->dn_Name))) {
                bn_wb13 = bn;
                continue;
            }
            if (compareBstr((char *)&partName_2,(char *)BADDR(dn->dn_Name))) {
                bn_wb2x = bn;
                continue;
            }
        }

        if (SysBase->SoftVer >= 36) {
            if (bn_wb13)
                demotePart(mountList, bn_wb13);
        } else {
            if (bn_wb2x)
                demotePart(mountList, bn_wb2x);
        }

    }

    if (ExpansionBase) CloseLibrary((struct Library *)ExpansionBase);

    return NULL;
}

