/*=============================================================================

                                    qurt_vtlb.h

GENERAL DESCRIPTION

EXTERNAL FUNCTIONS
        None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
        None.

Copyright (c) 2019, 2021, 2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
=============================================================================*/
#ifndef QURT_VTLB_H
#define QURT_VTLB_H

#ifdef __cplusplus
extern "C" {
#endif

/*
||  Names starting with "qurt_i_vtlb" are the internal low-level functions.
||  These should be considered subject to change.
*/

int qurt_i_vtlb_entry_create(unsigned *pIndex,
                             unsigned tlb_lo,
                             unsigned tlb_hi,
                             unsigned extension);

int qurt_i_vtlb_entry_create_with_pid(unsigned *pIndex,
                                      unsigned tlb_lo,
                                      unsigned tlb_hi,
                                      unsigned extension,
                                      unsigned target_pid);

int qurt_i_vtlb_entry_delete(unsigned index);

int qurt_i_vtlb_entry_read(unsigned index, unsigned *tlbinfo);

int qurt_i_vtlb_entry_write(unsigned index, unsigned tlb_lo, unsigned tlb_hi, unsigned extension);

int qurt_i_vtlb_entry_write_with_pid(unsigned index, unsigned tlb_lo, unsigned tlb_hi, unsigned extension, unsigned target_pid);

int qurt_i_vtlb_entry_probe(const void *vaddr, unsigned *tlbinfo, unsigned *pIndex);

int qurt_i_vtlb_entry_probe_with_pid(const void *vaddr, unsigned *tlbinfo, unsigned *pIndex, unsigned target_pid);


int qurt_i_vtlb_statistics(unsigned *stats); // Returns stats[0] -- total number of VTLB entries
                                             //         stats[1] -- number of available VTLB entries
                                             //         stats[2] -- max size of VTLB tree since boot

//can return index to an entry that was specialed, change it to take addresses instead of pages
int qurt_i_vtlb_set_special(int index, unsigned pageno, unsigned asid, unsigned size);

int qurt_i_vtlb_queue_ppage(unsigned pageno, unsigned vtlb_index);

#define QURT_VTLB_EXT_DEFAULT      0U
#define QURT_VTLB_EXT_LOCKED       1U
#define QURT_VTLB_EXT_EXCLUDE_DUMP 2U      /* Temporary ability to skip certain mappings in pd dump */
#define QURT_VTLB_EXT_FREELIST     0x800000u

#define QURT_VTLB_ERR_OVERLAP           -64
#define QURT_VTLB_ERR_TREE_NO_SPACE     -65
#define QURT_VTLB_ERR_INVALID_SIZE      -68
#define QURT_VTLB_ERR_INVALID_EXT       -69
#define QURT_VTLB_ERR_DEL_PGT_LOCKED    -70
#define QURT_VTLB_ERR_PGT_LOCK_CNT      -71

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif // QURT_VTLB_H
