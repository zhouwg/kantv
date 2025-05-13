/*==============================================================================
  Copyright (c) 2022, 2023 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

#ifndef VERSION_NOTE_H
#define VERSION_NOTE_H
#define VERSION_NOTE_LENGTH 100

 typedef struct {
	int sizename;				//Size of the NOTE section
	int sizedesc;				// Size of the descriptor(unused)
	int type;				// Type of section(unused)//stores version and library name
	char name[VERSION_NOTE_LENGTH];		// Name of NOTE section(version of shared object)
	int desc[3];				// used for labeling note segment version (lib.ver.V1.V2.V3)
 } lib_ver_note_t;

#endif //VERSION_NOTE_H

