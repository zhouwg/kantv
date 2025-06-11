# Standard definitions and error codes

## Standard definitions

AEEStdDef.h contains definitions of common data types used on the Hexagon DSPs and the application processor. It also has definitions of MIN, MAX values for common data types.

## Standard error codes

AEEStdErr.h file contains error codes returned by functions running on the DSPs and the application processor. 
The application invoking these APIs is expected to check for the error codes and implement appropriate error handling. Each API in the header files will have required documentation on the error codes it can return.

