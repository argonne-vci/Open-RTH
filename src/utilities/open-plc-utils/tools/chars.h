/*====================================================================*
 *
 *   chars.h - character selection and matcing macros;
 *
 *   Motley Tools by Charles Maier;
 *   Copyright 2001-2006 by Charles Maier Associates;
 *   Licensed under the Internet Software Consortium License;
 *
 *--------------------------------------------------------------------*/

#ifndef CHARS_HEADER
#define CHARS_HEADER

/*====================================================================*
 *
 *--------------------------------------------------------------------*/

#ifndef nobreak
#define nobreak(c) ((char)(c) != '\r') && ((char)(c) != '\n') && ((int)(c) != EOF)
#endif

#ifndef isquote
#define isquote(c) ((char)(c) == '\'') || ((char)(c) == '\"')
#endif

#ifndef isslash
#define isslash(c) ((char)(c) == '/') || ((char)(c) == '\\')
#endif

#ifndef isident
#define isident(c) (isalnum (c) || (c == '_') || (c == '-') || (c == '.') || (c == ':'))
#endif

#ifndef isoctal
#define isoctal(c) ((char)(c) >= '0') && ((char)(c) <= '7')
#endif

#ifndef nomatch
#define nomatch(c,o) ((char)(c) != (char)(o)) && ((int)(c) != EOF)
#endif

#ifndef iskey
#define iskey(c) ((int)(c) < 0x20) || ((int)(c) > 0x7E)
#endif

/*====================================================================*
 *   end definitions;
 *--------------------------------------------------------------------*/

#endif

