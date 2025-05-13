#ifndef VERSION_H
#define VERSION_H
/*===========================================================================

FILE:  version.h

SERVICES:  Hexagon Access Program (HAP) SDK version_string

GENERAL DESCRIPTION:
	Definitions for versioning

        Copyright © 2012 QUALCOMM Incorporated.
               All Rights Reserved.
            QUALCOMM Proprietary/GTDR
===========================================================================*/

#define VERSION_MAJOR    6
#define VERSION_MINOR    2
#define VERSION_MAINT    0
#define VERSION_BUILD    1

#define VERSION_STRING "HAP SDK 6.2.0.1 (srvr=qtcp406;br=main;cl=1242374)"


/*
=======================================================================
MACROS DOCUMENTATION
=======================================================================

VERSION_MAJOR

Description:
	Defines the major release number

Comments:
    It has to be a valid numerical value
=======================================================================
 
VERSION_MINOR

Description:
	Defines the minor release number

Comments:
    It has to be a valid numerical value
=======================================================================
 
VERSION_MAINT

Description:
	Defines the maintenance release

Comments:
    It has to be a valid numerical value
=======================================================================
 
VERSION_BUILD

Description:
	Defines the build ID

Comments:
    It has to be a valid numerical value
=======================================================================
 
VERSION_STRING

Description:
	Defines the version string

Definition:

   #define VERSION_STRING "a.b.c.d (name=value;name=value;...)"
	where a=major release number
	      b=minor release number
	      c=maintenance release number
	      d=build number

	name=value pair provides additional information about the build.
	Example: 
	patch/feature=comma separated list of features/patches that have been installed.
	br=p4 branch that was used for the build
	cl=p4 change list number
	machine=hostname of the machine that was used for the build.

Comments:

=======================================================================
*/

#endif // VERSION_H
