# Prototypes for string manipulation functions

## Introduction {#introduction}

`stringl.h` contains function prototypes for various string manipulation functions. These are considered safer than the standard C string manipulation functions and were developed as part of the [OpenBSD project](https://www.openbsd.org/).

## API Overview {#api-overview}

The stringl.h APIs include the following functions:

* ::strlcat

* ::wcslcat

* ::wstrlcat

* ::strlcpy

* ::wcslcpy

* ::wstrlcpy

* ::wstrlen

* ::wstrcmp

* ::wstrncmp

* ::strcasecmp

* ::strncasecmp

* ::std_scanul

* ::memscpy

* ::memscpy_i

* ::memsmove

* ::memsmove_i

* ::secure_memset

* ::timesafe_memcmp

* ::timesafe_strncmp

* ::strnlen

Header file: @b stringl.h
