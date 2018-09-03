#ifndef DEFSHDEF

#ifdef COMPILE_32
typedef unsigned __int32 UNIT;
#else
typedef unsigned __int64 UNIT;
#endif // COMPILE_32

typedef unsigned __int16 UTF16;
typedef unsigned __int32 UTF32;
typedef unsigned  B_INDEX;
typedef UTF32 UNICODEPOINT;

const unsigned BITS_PER_UNIT = sizeof( UNIT ) * 8;
const UNIT MINUSONE = (UNIT)-1;
const UNIT NOTLSB = MINUSONE ^ 0xFF;

const UNIT kByte = 1024;
const UNIT mByte = 1024*kByte;
const UNIT gByte = 1024*mByte;

#endif // DEFSHDEF

