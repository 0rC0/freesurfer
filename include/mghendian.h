/*
// mghendian.h
//
// purpose: 
// define BYTE_ORDER, LITTLE_ENDIAN, BIG_ENDIAN and which BYTE_ORDER it is
//
// by y.tosa
// July 7th, 2004
//
// SGI       /usr/include/sys/endian.h (decides on _MIPSEB or _MIPSEL)       LITTLE_ENDIAN/BIG_ENDIAN
// Mac       /usr/include/machine/endian.h (decides on __ppc__ or __i386__)  LITTLE_EHNDIAN/BIG_ENDIAN
// Linux     /usr/include/endian.h -> /usr/include/bits/endian.h             __LITTLE_ENDIAN/__BIG_ENDIAN
// Solaris   /usr/include/sys/isa_defs.h                                     _LITTLE_ENDIUAN/_BIG_ENDIAN
//
//
// BSD defines non-underscore variables
*/
/* See if it is set in the imake config first */
#ifndef mghendian_h
#define mghendian_h

/*/////////////////////////////////////////////////////*/
/* allow override */
#ifndef BYTE_ORDER

/*////////////Linux///////////////////////////*/
#ifdef linux
#include <endian.h>

#ifndef BYTE_ORDER
#define BYTE_ORDER __BYTE_ORDER
#endif

#ifndef LITTLE_ENDIAN 
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN __BIG_ENDIAN
#endif

#endif

/*///////////MacOS X and BSD ///////////////////////////*/
#if defined(__APPLE__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <machine/endian.h>
#endif

/*///////////Solaris 2.5.1/////////////////////*/
#ifdef sun

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN    4321
#endif

#include <sys/isa_defs.h>
/* only defines one of _LITTLE_ENDIAN or _BIG_ENDIAN */ 
#ifdef _LITTLE_ENDIAN
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#ifdef _BIG_ENDIAN
#define BYTE_ORDER BIG_ENDIAN
#endif

#endif

/*////////////IRIX  ///////////////////////////*/
#if defined(__sgi) || defined(Mips)
#include <sys/endian.h>
#endif

/*/////////////////////////////////////////////*/
#ifndef BYTE_ORDER
 #error "Unknown OS to mghendian.h.   Please report"
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN    4321
#endif

/*/////////////////////////////////////////////////*/
#endif /* BYTE_ORDER */
/*/////////////////////////////////////////////////*/

/* final check   bomb if not */
#if !defined(BYTE_ORDER) || \
    (BYTE_ORDER != BIG_ENDIAN && BYTE_ORDER != LITTLE_ENDIAN)
/*  && BYTE_ORDER != PDP_ENDIAN) */
  #error "Undefined or invalid BYTE_ORDER";
#endif

#endif /* mghendian.h */
