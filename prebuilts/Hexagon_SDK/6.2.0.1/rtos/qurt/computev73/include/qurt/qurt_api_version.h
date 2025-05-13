#ifndef QURT_API_VERSION_H
#define QURT_API_VERSION_H
/*==============================================================================

qurt_api_version.h

GENERAL DESCRIPTION
    API version file

EXTERNAL FUNCTIONS
    None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
    None.

Copyright (c) Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

==============================================================================*/

/*==============================================================================
                         CONSTANTS AND DEFINITIONS
==============================================================================*/
/**
 * Each field of the QURT_API_VERSION definitions is an 8-bit unsigned integer.
 * Main release has first 3 fields updated - Major, Minor and Release.
 *  - QURT_API_VERSION = Major, Minor, Release.
 * Patch releases are supported by adding the extra field.
 *  - QURT_API_VERSION = Major, Minor, Release, Patch.
 */
// Major version is incremented for incompatible API changes.
#define QURT_API_VER_MAJOR 1

// Minor version is incremented for backward-compatible enhancements in the API
// set.
#define QURT_API_VER_MINOR 4

// RELEASE version is incremented for each release within a `MAJOR.MINOR`
// release.
#define QURT_API_VER_RELEASE 1

// Patch version is incremented when new API content is introduced on older LTS
// release.
#define QURT_API_VER_PATCH 0

/* Update the QURT_API_VERSION function macro. */
#define QURT_API_VERSION_ENCODE(major, minor, release, patch) \
    ((((major) & 0xFF) << 24) | (((minor) & 0xFF) << 16) | \
        (((release) & 0xFF) << 8) | ((patch) & 0xFF))

/* Update the QURT_API_VERSION Macro. */
#define QURT_API_VERSION \
    QURT_API_VERSION_ENCODE(QURT_API_VER_MAJOR, QURT_API_VER_MINOR, \
        QURT_API_VER_RELEASE, QURT_API_VER_PATCH)

/** Usage:
 *
 * #if QURT_API_VERSION >= QURT_API_VERSION_ENCODE(1,4,0,0)
 *  qurt_func_2(a,b,c);
 * #else
 *  qurt_func(a);
 * #endif
 *
 */
/*
   Gets the QuRT API version.

  @return
  QuRT API version.

  @dependencies
  None.
 */
unsigned int qurt_api_version(void);

#endif /* QURT_API_VERSION_H */
