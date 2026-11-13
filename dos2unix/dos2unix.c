/*
 *  Name: dos2unix
 *  Documentation:
 *    Remove cr ('\x0d') characters from a file.
 *
 *  The dos2unix package is distributed under FreeBSD style license.
 *  See also https://www.freebsd.org/copyright/freebsd-license.html
 *  --------
 *
 *  Copyright (C) 2009-2026 Erwin Waterlander
 *  Copyright (C) 1998 Christian Wurll
 *  Copyright (C) 1998 Bernd Johannes Wuebben
 *  Copyright (C) 1994-1995 Benjamin Lin.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice in the documentation and/or other materials provided with
 *     the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 *  OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  == 1.0 == 1989.10.04 == John Birchfield (jb@koko.csustan.edu)
 *  == 1.1 == 1994.12.20 == Benjamin Lin (blin@socs.uts.edu.au)
 *     Cleaned up for Borland C/C++ 4.02
 *  == 1.2 == 1995.03.16 == Benjamin Lin (blin@socs.uts.edu.au)
 *     Modified to more conform to UNIX style.
 *  == 2.0 == 1995.03.19 == Benjamin Lin (blin@socs.uts.edu.au)
 *     Rewritten from scratch.
 *  == 2.1 == 1995.03.29 == Benjamin Lin (blin@socs.uts.edu.au)
 *     Conversion to SunOS charset implemented.
 *  == 2.2 == 1995.03.30 == Benjamin Lin (blin@socs.uts.edu.au)
 *     Fixed a bug in 2.1 where in new-file mode, if outfile already exists
 *     conversion cannot be completed properly.
 *
 * Added Mac text file translation, i.e. \r to \n conversion
 * Bernd Johannes Wuebben, wuebben@kde.org
 * Wed Feb  4 19:12:58 EST 1998
 *
 * Added extra newline if ^M occurs
 * Christian Wurll, wurll@ira.uka.de
 * Thu Nov 19 1998
 *
 *  See ChangeLog.txt for complete version history.
 *
 */

/* #define DEBUG 1 */
/* D2U_DOS2UNIX_C selects the conversion tables of dos2unix.h for this file only,
 * see the explanation in dos2unix.h. It must be defined before the header is
 * included. */
#define D2U_DOS2UNIX_C

#include "dos2unix.h"
#include "common.h"
#if (defined(_WIN32) && !defined(__CYGWIN__))
#include <windows.h>
#endif
#ifdef D2U_UNICODE
#if !defined(__MSDOS__) && !defined(_WIN32) && !defined(__OS2__) /* Unix, Cygwin */
#include <langinfo.h>
#endif
#endif

/* ISO C23 constexpr constants. They replace literals that were repeated
 * through this file. Unlike a #define they have a type, and being 'static'
 * they are private to this file (common.c has its own line break
 * constants). */

/* Line break characters for the byte oriented conversion functions.
 * They are int, the type that d2u_getc() returns and fputc() takes. */
static constexpr int D2U_CHAR_LF = 0x0a; /* Newline/Line Feed (LF) */
static constexpr int D2U_CHAR_CR = 0x0d; /* Carriage Return (CR) */

#ifdef D2U_UNICODE
/* The same characters for the UTF-16 conversion functions. They are
 * wint_t, the type that d2u_getwc() returns and d2u_putwc() takes, so that
 * comparing and passing them needs no signed/unsigned conversion. */
static constexpr wint_t D2U_WCHAR_LF = 0x0a; /* Newline/Line Feed (LF) */
static constexpr wint_t D2U_WCHAR_CR = 0x0d; /* Carriage Return (CR) */
#endif

/* Last year of the copyright notice that PrintLicense() prints. Update it
 * together with the copyright lines in the header of this file. */
static constexpr int D2U_COPYRIGHT_LAST_YEAR = 2026;

/* Size of the program name buffer in main(): the 8 characters of
 * "dos2unix" or "mac2unix" and the terminating null. The static_assert
 * makes the build fail if the two names ever get a different length,
 * instead of truncating the longer name at run time. */
static constexpr size_t D2U_PROGNAME_SIZE = sizeof("dos2unix");
static_assert(sizeof("mac2unix") == D2U_PROGNAME_SIZE,
              "dos2unix and mac2unix must have the same name length");

/* Size of the buffer in main() for the directory of the message catalogs
 * (ENABLE_NLS). A longer value of DOS2UNIX_LOCALEDIR is rejected. */
static constexpr size_t D2U_LOCALEDIR_SIZE = 1024;

/* Print the copyright notice of this program and the license text.
 * main() passes this function to parse_options(), which calls it for the
 * --license option. */
void PrintLicense(void) {
    D2U_ANSI_FPRINTF(stdout, _("\
Copyright (C) 2009-%d Erwin Waterlander\n\
Copyright (C) 1998      Christian Wurll (Version 3.1)\n\
Copyright (C) 1998      Bernd Johannes Wuebben (Version 3.0)\n\
Copyright (C) 1994-1995 Benjamin Lin\n\
All rights reserved.\n\n"),
                     D2U_COPYRIGHT_LAST_YEAR);
    PrintBSDLicense();
}

/* Design notes for the conversion functions in this file.
 *
 * Why there are two families of conversion functions:
 * - dos2unix() and mac2unix() work on bytes. They read with d2u_getc() and
 *   write with fputc(). They are used for ASCII, the 8 bit code pages,
 *   UTF-8 and GB18030. In these encodings a byte 0x0a or 0x0d is always a
 *   real LF or CR, so a file can be converted byte by byte. Every byte that
 *   is written passes through a conversion table of dos2unix.h; that is
 *   how the -7 and -iso modes translate the 8 bit characters.
 * - dos2unixW() and mac2unixW() exist only with D2U_UNICODE and are used
 *   for UTF-16 input. A UTF-16 code unit is two bytes, and a byte 0x0a or
 *   0x0d can be one half of another character (U+010A for example), so the
 *   bytes cannot be judged one by one. d2u_getwc() reads whole 16 bit code
 *   units as wint_t, and d2u_putwc() writes them in the output encoding
 *   and joins surrogate pairs. The conversion tables are not used here.
 *   common.c calls the W function when the input has a UTF-16 BOM or when
 *   UTF-16 is assumed (-ul, -ub), and the byte function otherwise.
 *
 * Why the functions look one character ahead:
 * A CR can only be judged when the next character is known. CR LF is a DOS
 * line break; a CR without LF is a Mac line break. Therefore the functions
 * hold one character back in PreviousChar and write it only after NextChar
 * has been read.
 * - dos2unix mode: the CR of CR LF is dropped. A CR that is not followed
 *   by LF is written unchanged (a Mac line break is not touched).
 * - mac2unix mode: a CR that is not followed by LF becomes LF. CR LF is
 *   written unchanged (an existing DOS line break is preserved).
 * The character is held in a variable; nothing is pushed back to the input
 * stream. d2u_ungetc() is only used by read_bom() in common.c, and there
 * is no push back function for wide characters. Holding the character back
 * needs no buffer and cannot fail.
 *
 * Common to all four functions:
 * - They return 0 on success and -1 on error. Errors are: a binary symbol
 *   in the input when conversion is not forced (BINARY_FILE is set in
 *   ipFlag->status), a read error and a write error (d2u_getc_error(),
 *   d2u_putc_error() and d2u_putwc_error() store errno in ipFlag->error).
 * - [[nodiscard]] (ISO C23): the return value is the only sign that the
 *   output is incomplete. Ignoring it would let a damaged temporary file
 *   replace the input file.
 */

#ifdef D2U_UNICODE
/* Convert UTF-16 text from DOS to Unix line breaks (dos2unix mode).
 * See the design notes above. */
[[nodiscard]] int dos2unixW(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag,
                            const char *progname) {
    wint_t PreviousChar = WEOF;
    wint_t NextChar;
    unsigned int line_nr = 1;
    unsigned int converted = 0;

    PreviousChar = d2u_getwc(ipInF, ipFlag->bomtype);
    if (PreviousChar == WEOF) {
        if (ferror(ipInF->file)) {
            d2u_getc_error(ipFlag, progname);
            return -1;
        }
        logConverted(0, ipFlag->verbose, progname, converted, line_nr);
        return 0; /* empty file */
    }
    if ((!ipFlag->Force) && binaryCharW(PreviousChar)) {
        ipFlag->status |= BINARY_FILE;
        if (ipFlag->verbose) {
            logBinaryCharW(ipFlag, PreviousChar, line_nr, progname);
        }
        return -1;
    }

    /* We look one char ahead (NextChar) to see if it's
     * a CR-LF combination, and push the previous char.
     */
    while ((NextChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) { /* get character */
        if ((!ipFlag->Force) && binaryCharW(NextChar)) {
            ipFlag->status |= BINARY_FILE;
            if (ipFlag->verbose) {
                logBinaryCharW(ipFlag, NextChar, line_nr, progname);
            }
            return -1;
        }

        if (NextChar == D2U_WCHAR_LF) {         /* an LF */
            ++line_nr;                          /* Count all DOS and Unix line breaks */
            if (PreviousChar == D2U_WCHAR_CR) { /* CR-LF combination */
                converted++;
                if (ipFlag->NewLine) { /* add additional LF? */
                    if (d2u_putwc(D2U_WCHAR_LF, ipOutF, ipFlag, progname) == WEOF) {
                        d2u_putwc_error(ipFlag, progname);
                        ipFlag->line_nr = line_nr;
                        return -1;
                    }
                }
            } else { /* Single LF */
                if (d2u_putwc(PreviousChar, ipOutF, ipFlag, progname) == WEOF) {
                    d2u_putwc_error(ipFlag, progname);
                    ipFlag->line_nr = line_nr;
                    return -1;
                }
            }
        } else { /* No DOS or Unix line break */
            if (d2u_putwc(PreviousChar, ipOutF, ipFlag, progname) == WEOF) {
                d2u_putwc_error(ipFlag, progname);
                ipFlag->line_nr = line_nr;
                return -1;
            }
        }
        PreviousChar = NextChar;
    }

    if ((NextChar == WEOF) && ferror(ipInF->file)) {
        d2u_getc_error(ipFlag, progname);
        return -1;
    }

    if (d2u_putwc(PreviousChar, ipOutF, ipFlag, progname) == WEOF) {
        d2u_putwc_error(ipFlag, progname);
        ipFlag->line_nr = line_nr;
        return -1;
    }

    if (NextChar == WEOF && ipFlag->add_eol && PreviousChar != WEOF &&
        PreviousChar != D2U_WCHAR_LF) {
        /* Add missing line break at the last line. */
        if (ipFlag->verbose > 1) {
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
            D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n"));
        }
        if (d2u_putwc(D2U_WCHAR_LF, ipOutF, ipFlag, progname) == WEOF) {
            d2u_putwc_error(ipFlag, progname);
            ipFlag->line_nr = line_nr;
            return -1;
        }
    }
    d2u_check_surrogate_state(ipFlag, progname);
    if (ipFlag->status & UNICODE_CONVERSION_ERROR) {
        ipFlag->line_nr = line_nr;
    }
    logConverted(0, ipFlag->verbose, progname, converted, line_nr);
    return 0;
}

/* Convert UTF-16 text from Mac to Unix line breaks (mac2unix mode).
 * PPreviousChar, the character before PreviousChar, is only used to count
 * the lines: an LF directly after a CR belongs to a DOS line break and is
 * not counted. See the design notes above. */
[[nodiscard]] int mac2unixW(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag,
                            const char *progname) {
    wint_t PreviousChar = WEOF;
    wint_t PPreviousChar = WEOF;
    wint_t NextChar;
    unsigned int line_nr = 1;
    unsigned int converted = 0;

    PreviousChar = d2u_getwc(ipInF, ipFlag->bomtype);
    if (PreviousChar == WEOF) {
        if (ferror(ipInF->file)) {
            d2u_getc_error(ipFlag, progname);
            return -1;
        }
        logConverted(0, ipFlag->verbose, progname, converted, line_nr);
        return 0; /* empty file */
    }
    if ((!ipFlag->Force) && binaryCharW(PreviousChar)) {
        ipFlag->status |= BINARY_FILE;
        if (ipFlag->verbose) {
            logBinaryCharW(ipFlag, PreviousChar, line_nr, progname);
        }
        return -1;
    }

    /* We look one char ahead (NextChar) to see if it's
     * a CR-LF combination, and push the previous char.
     * mac2unix keeps CR-LF.
     */
    while ((NextChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) { /* get character */
        if ((!ipFlag->Force) && binaryCharW(NextChar)) {
            ipFlag->status |= BINARY_FILE;
            if (ipFlag->verbose) {
                logBinaryCharW(ipFlag, NextChar, line_nr, progname);
            }
            return -1;
        }

        if (PreviousChar == D2U_WCHAR_CR) { /* A CR */
            if (NextChar == D2U_WCHAR_LF) { /* A CR-LF combination, put CR */
                if (d2u_putwc(D2U_WCHAR_CR, ipOutF, ipFlag, progname) == WEOF) {
                    d2u_putwc_error(ipFlag, progname);
                    ipFlag->line_nr = line_nr;
                    return -1;
                }
            } else { /* No CR-LF combination, put LF */
                ++line_nr;
                converted++;
                if (d2u_putwc(D2U_WCHAR_LF, ipOutF, ipFlag, progname) == WEOF) {
                    d2u_putwc_error(ipFlag, progname);
                    ipFlag->line_nr = line_nr;
                    return -1;
                }
                if (ipFlag->NewLine) { /* add additional LF? */
                    if (d2u_putwc(D2U_WCHAR_LF, ipOutF, ipFlag, progname) == WEOF) {
                        d2u_putwc_error(ipFlag, progname);
                        ipFlag->line_nr = line_nr;
                        return -1;
                    }
                }
            }
        } else {
            if (PreviousChar == D2U_WCHAR_LF && PPreviousChar != D2U_WCHAR_CR) { /* An LF */
                ++line_nr; /* Count all Mac and Unix line breaks */
            }
            if (d2u_putwc(PreviousChar, ipOutF, ipFlag, progname) == WEOF) {
                d2u_putwc_error(ipFlag, progname);
                ipFlag->line_nr = line_nr;
                return -1;
            }
        }
        PPreviousChar = PreviousChar;
        PreviousChar = NextChar;
    }

    if ((NextChar == WEOF) && ferror(ipInF->file)) {
        d2u_getc_error(ipFlag, progname);
        return -1;
    }

    if (PreviousChar == D2U_WCHAR_CR) { /* A CR */
        ++line_nr;
        converted++;
        if (d2u_putwc(D2U_WCHAR_LF, ipOutF, ipFlag, progname) == WEOF) {
            d2u_putwc_error(ipFlag, progname);
            ipFlag->line_nr = line_nr;
            return -1;
        }
        if (ipFlag->NewLine) { /* add additional LF? */
            if (d2u_putwc(D2U_WCHAR_LF, ipOutF, ipFlag, progname) == WEOF) {
                d2u_putwc_error(ipFlag, progname);
                ipFlag->line_nr = line_nr;
                return -1;
            }
        }
    } else {
        if (PreviousChar == D2U_WCHAR_LF && PPreviousChar != D2U_WCHAR_CR) { /* An LF */
            ++line_nr; /* Count all Mac and Unix line breaks */
        }
        if (d2u_putwc(PreviousChar, ipOutF, ipFlag, progname) == WEOF) {
            d2u_putwc_error(ipFlag, progname);
            ipFlag->line_nr = line_nr;
            return -1;
        }
    }

    if (NextChar == WEOF && ipFlag->add_eol && PreviousChar != WEOF &&
        !(PreviousChar == D2U_WCHAR_LF || PreviousChar == D2U_WCHAR_CR)) {
        /* Add missing line break at the last line. */
        if (ipFlag->verbose > 1) {
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
            D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n"));
        }
        if (d2u_putwc(D2U_WCHAR_LF, ipOutF, ipFlag, progname) == WEOF) {
            d2u_putwc_error(ipFlag, progname);
            ipFlag->line_nr = line_nr;
            return -1;
        }
    }
    d2u_check_surrogate_state(ipFlag, progname);
    if (ipFlag->status & UNICODE_CONVERSION_ERROR) {
        ipFlag->line_nr = line_nr;
    }
    logConverted(0, ipFlag->verbose, progname, converted, line_nr);
    return 0;
}

/* converts stream ipInF to UNIX format text and write to stream ipOutF
 * RetVal: 0  if success
 *         -1  otherwise
 *
 * UTF-16 variant, see the design notes above. It resets ipFlag->status and
 * calls dos2unixW() or mac2unixW(), depending on ipFlag->FromToMode. An
 * unknown FromToMode converts nothing and returns 0; a DEBUG build ends
 * the program instead.
 * [[nodiscard]] (ISO C23): see the prototype in dos2unix.h.
 */
[[nodiscard]] int ConvertDosToUnixW(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag,
                                    const char *progname) {
    int RetVal = 0;

    ipFlag->status = 0;

    /* CR-LF -> LF */
    /* LF    -> LF, in case the input file is a Unix text file */
    /* CR    -> CR, in dos2unix mode (don't modify Mac file) */
    /* CR    -> LF, in Mac mode */
    /* \x0a = Newline/Line Feed (LF) */
    /* \x0d = Carriage Return (CR) */

    switch (ipFlag->FromToMode) {
    case FROMTO_DOS2UNIX: /* dos2unix */
        RetVal = dos2unixW(ipInF, ipOutF, ipFlag, progname);
        break;
    case FROMTO_MAC2UNIX: /* mac2unix */
        RetVal = mac2unixW(ipInF, ipOutF, ipFlag, progname);
        break;
    default: /* unknown FromToMode */
        ;
#if DEBUG
        D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
        D2U_UTF8_FPRINTF(stderr, _("program error, invalid conversion mode %d\n"),
                         ipFlag->FromToMode);
        /* EXIT_FAILURE (<stdlib.h>) instead of a raw 1: this ends the
         * process because of a program error. Only in a DEBUG build. */
        exit(EXIT_FAILURE);
#endif
    }
    return RetVal;
}
#endif

/* Convert byte oriented text from DOS to Unix line breaks (dos2unix mode).
 * ConvTable is one of the conversion tables of dos2unix.h, it has an entry
 * for every byte value. A character is only used as an index after it was
 * compared with EOF, so the index is always 0..255.
 * See the design notes above. */
[[nodiscard]] int dos2unix(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag, const char *progname,
                           const int *ConvTable) {
    int PreviousChar = EOF;
    int NextChar;
    unsigned int line_nr = 1;
    unsigned int converted = 0;

    PreviousChar = d2u_getc(ipInF);
    if (PreviousChar == EOF) {
        if (ferror(ipInF->file)) {
            d2u_getc_error(ipFlag, progname);
            return -1;
        }
        logConverted(0, ipFlag->verbose, progname, converted, line_nr);
        return 0; /* empty file */
    }
    if ((!ipFlag->Force) && binaryChar(PreviousChar)) {
        ipFlag->status |= BINARY_FILE;
        if (ipFlag->verbose) {
            logBinaryChar(ipFlag, PreviousChar, line_nr, progname);
        }
        return -1;
    }

    /* We look one char ahead (NextChar) to see if it's
     * a CR-LF combination, and push the previous char.
     */
    while ((NextChar = d2u_getc(ipInF)) != EOF) { /* get character */
        if ((!ipFlag->Force) && binaryChar(NextChar)) {
            ipFlag->status |= BINARY_FILE;
            if (ipFlag->verbose) {
                logBinaryChar(ipFlag, NextChar, line_nr, progname);
            }
            return -1;
        }

        if (NextChar == D2U_CHAR_LF) {         /* an LF */
            ++line_nr;                         /* Count all DOS and Unix line breaks */
            if (PreviousChar == D2U_CHAR_CR) { /* CR-LF combination */
                converted++;
                if (ipFlag->NewLine) { /* add additional LF? */
                    if (fputc(D2U_CHAR_LF, ipOutF) == EOF) {
                        d2u_putc_error(ipFlag, progname);
                        return -1;
                    }
                }
            } else { /* Single LF */
                if (fputc(ConvTable[PreviousChar], ipOutF) == EOF) {
                    d2u_putc_error(ipFlag, progname);
                    return -1;
                }
            }
        } else { /* No DOS or Unix line break */
            if (fputc(ConvTable[PreviousChar], ipOutF) == EOF) {
                d2u_putc_error(ipFlag, progname);
                return -1;
            }
        }
        PreviousChar = NextChar;
    }

    if ((NextChar == EOF) && ferror(ipInF->file)) {
        d2u_getc_error(ipFlag, progname);
        return -1;
    }

    if (fputc(ConvTable[PreviousChar], ipOutF) == EOF) {
        d2u_putc_error(ipFlag, progname);
        return -1;
    }

    if (NextChar == EOF && ipFlag->add_eol && PreviousChar != EOF && PreviousChar != D2U_CHAR_LF) {
        /* Add missing line break at the last line. */
        if (ipFlag->verbose > 1) {
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
            D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n"));
        }
        if (fputc(D2U_CHAR_LF, ipOutF) == EOF) {
            d2u_putc_error(ipFlag, progname);
            return -1;
        }
    }
    logConverted(0, ipFlag->verbose, progname, converted, line_nr);
    return 0;
}

/* Convert byte oriented text from Mac to Unix line breaks (mac2unix mode).
 * ConvTable and its index: see dos2unix(). PPreviousChar is only used to
 * count the lines, see mac2unixW(). See the design notes above. */
[[nodiscard]] int mac2unix(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag, const char *progname,
                           const int *ConvTable) {
    int PreviousChar = EOF;
    int PPreviousChar = EOF;
    int NextChar;
    unsigned int line_nr = 1;
    unsigned int converted = 0;

    PreviousChar = d2u_getc(ipInF);
    if (PreviousChar == EOF) {
        if (ferror(ipInF->file)) {
            d2u_getc_error(ipFlag, progname);
            return -1;
        }
        logConverted(0, ipFlag->verbose, progname, converted, line_nr);
        return 0; /* empty file */
    }
    if ((!ipFlag->Force) && binaryChar(PreviousChar)) {
        ipFlag->status |= BINARY_FILE;
        if (ipFlag->verbose) {
            logBinaryChar(ipFlag, PreviousChar, line_nr, progname);
        }
        return -1;
    }

    /* We look one char ahead (NextChar) to see if it's
     * a CR-LF combination, and push the previous char.
     * mac2unix keeps CR-LF.
     */
    while ((NextChar = d2u_getc(ipInF)) != EOF) { /* get character */
        if ((!ipFlag->Force) && binaryChar(NextChar)) {
            ipFlag->status |= BINARY_FILE;
            if (ipFlag->verbose) {
                logBinaryChar(ipFlag, NextChar, line_nr, progname);
            }
            return -1;
        }

        if (PreviousChar == D2U_CHAR_CR) { /* A CR */
            if (NextChar == D2U_CHAR_LF) { /* A CR-LF combination, put CR */
                if (fputc(D2U_CHAR_CR, ipOutF) == EOF) {
                    d2u_putc_error(ipFlag, progname);
                    return -1;
                }
            } else { /* No CR-LF combination, put LF */
                ++line_nr;
                converted++;
                if (fputc(D2U_CHAR_LF, ipOutF) == EOF) {
                    d2u_putc_error(ipFlag, progname);
                    return -1;
                }
                if (ipFlag->NewLine) { /* add additional LF? */
                    if (fputc(D2U_CHAR_LF, ipOutF) == EOF) {
                        d2u_putc_error(ipFlag, progname);
                        return -1;
                    }
                }
            }
        } else {
            if (PreviousChar == D2U_CHAR_LF && PPreviousChar != D2U_CHAR_CR) { /* An LF */
                ++line_nr; /* Count all Mac and Unix line breaks */
            }
            if (fputc(ConvTable[PreviousChar], ipOutF) == EOF) {
                d2u_putc_error(ipFlag, progname);
                return -1;
            }
        }
        PPreviousChar = PreviousChar;
        PreviousChar = NextChar;
    }

    if ((NextChar == EOF) && ferror(ipInF->file)) {
        d2u_getc_error(ipFlag, progname);
        return -1;
    }

    if (PreviousChar == D2U_CHAR_CR) { /* A CR */
        ++line_nr;
        converted++;
        if (fputc(D2U_CHAR_LF, ipOutF) == EOF) {
            d2u_putc_error(ipFlag, progname);
            return -1;
        }
        if (ipFlag->NewLine) { /* add additional LF? */
            if (fputc(D2U_CHAR_LF, ipOutF) == EOF) {
                d2u_putc_error(ipFlag, progname);
                return -1;
            }
        }
    } else {
        if (PreviousChar == D2U_CHAR_LF && PPreviousChar != D2U_CHAR_CR) { /* An LF */
            ++line_nr; /* Count all Mac and Unix line breaks */
        }
        if (fputc(ConvTable[PreviousChar], ipOutF) == EOF) {
            d2u_putc_error(ipFlag, progname);
            return -1;
        }
    }

    if (NextChar == EOF && ipFlag->add_eol && PreviousChar != EOF &&
        !(PreviousChar == D2U_CHAR_LF || PreviousChar == D2U_CHAR_CR)) {
        /* Add missing line break at the last line. */
        if (ipFlag->verbose > 1) {
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
            D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n"));
        }
        if (fputc(D2U_CHAR_LF, ipOutF) == EOF) {
            d2u_putc_error(ipFlag, progname);
            return -1;
        }
    }
    logConverted(0, ipFlag->verbose, progname, converted, line_nr);
    return 0;
}

/* converts stream ipInF to UNIX format text and write to stream ipOutF
 * RetVal: 0  if success
 *         -1  otherwise
 *
 * Byte oriented variant, see the design notes above. It resets
 * ipFlag->status, selects the conversion table for ipFlag->ConvMode and
 * calls dos2unix() or mac2unix(), depending on ipFlag->FromToMode.
 * An unknown ConvMode sets WRONG_CODEPAGE in ipFlag->status and returns -1.
 * An unknown FromToMode converts nothing and returns 0; a DEBUG build ends
 * the program instead.
 * [[nodiscard]] (ISO C23): see the prototype in dos2unix.h.
 */
[[nodiscard]] int ConvertDosToUnix(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag,
                                   const char *progname) {
    int RetVal = 0;
    /* Points to one of the read-only (constexpr) conversion tables of
     * dos2unix.h, hence pointer to const. */
    const int *ConvTable;

    ipFlag->status = 0;

    switch (ipFlag->ConvMode) {
    case CONVMODE_ASCII:   /* ascii */
    case CONVMODE_UTF16LE: /* Assume UTF-16LE, bomtype = FILE_UTF8 or GB18030 */
    case CONVMODE_UTF16BE: /* Assume UTF-16BE, bomtype = FILE_UTF8 or GB18030 */
        ConvTable = D2UAsciiTable;
        break;
    case CONVMODE_7BIT: /* 7bit */
        ConvTable = D2U7BitTable;
        break;
    case CONVMODE_437: /* iso */
        ConvTable = D2UIso437Table;
        break;
    case CONVMODE_850: /* iso */
        ConvTable = D2UIso850Table;
        break;
    case CONVMODE_860: /* iso */
        ConvTable = D2UIso860Table;
        break;
    case CONVMODE_863: /* iso */
        ConvTable = D2UIso863Table;
        break;
    case CONVMODE_865: /* iso */
        ConvTable = D2UIso865Table;
        break;
    case CONVMODE_1252: /* iso */
        ConvTable = D2UIso1252Table;
        break;
    default: /* unknown convmode */
        ipFlag->status |= WRONG_CODEPAGE;
        return (-1);
    }
    /* Turn off ISO and 7-bit conversion for Unicode text files */
    if (ipFlag->bomtype > 0) {
        ConvTable = D2UAsciiTable;
    }

    if ((ipFlag->ConvMode > CONVMODE_7BIT) && (ipFlag->verbose)) { /* not ascii or 7bit */
        D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
        D2U_UTF8_FPRINTF(stderr, _("using code page %d.\n"), ipFlag->ConvMode);
    }

    /* CR-LF -> LF */
    /* LF    -> LF, in case the input file is a Unix text file */
    /* CR    -> CR, in dos2unix mode (don't modify Mac file) */
    /* CR    -> LF, in Mac mode */
    /* \x0a = Newline/Line Feed (LF) */
    /* \x0d = Carriage Return (CR) */

    switch (ipFlag->FromToMode) {
    case FROMTO_DOS2UNIX: /* dos2unix */
        RetVal = dos2unix(ipInF, ipOutF, ipFlag, progname, ConvTable);
        break;
    case FROMTO_MAC2UNIX: /* mac2unix */
        RetVal = mac2unix(ipInF, ipOutF, ipFlag, progname, ConvTable);
        break;
    default: /* unknown FromToMode */
        ;
#if DEBUG
        D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
        D2U_UTF8_FPRINTF(stderr, _("program error, invalid conversion mode %d\n"),
                         ipFlag->FromToMode);
        /* EXIT_FAILURE (<stdlib.h>) instead of a raw 1: this ends the
         * process because of a program error. Only in a DEBUG build. */
        exit(EXIT_FAILURE);
#endif
    }
    return RetVal;
}

/* Program entry point. The program converts as dos2unix, or as mac2unix when
 * it is started under the name mac2unix (or mac2unix.exe).
 *
 * main() only prepares what the shared code needs: the program name, the
 * locale and the message catalog, the CFlag structure and, on Windows with
 * D2U_UNIFILE, the argument list in UTF-8. parse_options() in common.c then
 * parses the options and converts the files.
 *
 * Exit status (command-line contract, do not change):
 * The RETURN VALUE section of the dos2unix(1) manual page, which also
 * describes unix2dos, documents: zero on success, the last system error
 * (errno) when a system error occurs, and 1 for other errors. To stay
 * compatible with that, main() returns the value of parse_options(), or
 * errno when a memory allocation fails here. These returns are
 * deliberately not replaced by EXIT_SUCCESS / EXIT_FAILURE from <stdlib.h>:
 * EXIT_FAILURE would hide the system error number that the manual
 * promises. See the comments at the return statements.
 */
int main(int argc, char *argv[]) {
    /* variable declarations */
    char progname[D2U_PROGNAME_SIZE];
    CFlag *pFlag;
    char *ptr;
    /* localedir is only filled in when ENABLE_NLS is defined, and
     * parse_options() only reads it in that case. */
    char localedir[D2U_LOCALEDIR_SIZE];
    int ret;
#ifdef __MINGW64__
    /* AUDIT NOTE (C23 modernization review, code intentionally unchanged,
     * this branch cannot be built or tested on the Unix development host):
     * this is a local variable. The MinGW-w64 startup code reads a global
     * variable of this name before main() runs, so this declaration
     * appears to have no effect. */
    int _dowildcard = -1; /* enable wildcard expansion for Win64 */
#endif
    int argc_new;
    char **argv_new;
#ifdef D2U_UNIFILE
    wchar_t **wargv;
    char ***argv_glob;
#endif

    /* The name is copied with d2u_strncpy() (common.c) and not with
     * strncpy(): d2u_strncpy() always writes the terminating null into the
     * last byte of the buffer, so progname is a valid string even if a name
     * ever fills the whole buffer. strncpy() leaves the buffer without a
     * terminating null in that case. The assignment before the copy is
     * kept from the original code. */
    progname[D2U_PROGNAME_SIZE - 1] = '\0';
    d2u_strncpy(progname, "dos2unix", sizeof(progname));

#ifdef ENABLE_NLS
    ptr = getenv("DOS2UNIX_LOCALEDIR");
    if (ptr == nullptr) {
        d2u_strncpy(localedir, LOCALEDIR, sizeof(localedir));
    } else {
        if (strlen(ptr) < sizeof(localedir)) {
            d2u_strncpy(localedir, ptr, sizeof(localedir));
        } else {
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
            D2U_ANSI_FPRINTF(
                stderr, "%s",
                _("error: Value of environment variable DOS2UNIX_LOCALEDIR is too long.\n"));
            d2u_strncpy(localedir, LOCALEDIR, sizeof(localedir));
        }
    }
#endif

#if defined(ENABLE_NLS) ||                                                                         \
    (defined(D2U_UNICODE) && !defined(__MSDOS__) && !defined(_WIN32) && !defined(__OS2__))
/* setlocale() is also needed for nl_langinfo() */
#if (defined(_WIN32) && !defined(__CYGWIN__))
    /* When the locale is set to "" on Windows all East-Asian multi-byte ANSI encoded text is
       printed wrongly when you use standard printf(). Also UTF-8 code is printed wrongly. See also
       test/setlocale.c. When we set the locale to "C" gettext still translates the messages on
       Windows. On Unix this would disable gettext. */
    setlocale(LC_ALL, "C");
#else
    setlocale(LC_ALL, "");
#endif
#endif

#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, localedir);
    textdomain(PACKAGE);
#endif

    /* variable initialisations */
    /* The CFlag structure is allocated on the heap and not declared as a
     * local variable. The history of the code does not record why. It keeps
     * the stack frame of main() a little smaller, which is what counts on
     * the legacy targets with a small stack (the 16 bit DOS builds, see
     * wccdos16.mak and bcc.mak). The structure is small, though, and main()
     * keeps the much larger localedir[] on the stack.
     * What a maintainer has to know:
     * - The allocation can fail, so the result is checked.
     * - malloc() does not initialise the memory. Only FromToMode and
     *   keep_bom are set here; parse_options() sets the defaults of the
     *   other option fields, with the exception in the audit note below.
     * - The structure is freed on every path that leaves main().
     * AUDIT NOTE (C23 modernization review, code intentionally unchanged):
     * the field AllowChown gets no default value, neither here nor in
     * parse_options(). It is only assigned by the options --allow-chown
     * and --no-allow-chown, and common.c reads it when the ownership of
     * the output file cannot be set in old file mode. Without one of these
     * options that read uses uninitialised memory. The documented default
     * is "no"; setting AllowChown to false with the other defaults in
     * parse_options() would make that reliable. */
    pFlag = (CFlag *)malloc(sizeof(CFlag));
    if (pFlag == nullptr) {
        D2U_UTF8_FPRINTF(stderr, "dos2unix:");
        D2U_ANSI_FPRINTF(stderr, " %s\n", strerror(errno));
        /* Exit status: the system error number, as documented in the
         * dos2unix(1) manual page (see the comment above main()). Keep
         * 'errno', do not replace it by EXIT_FAILURE. */
        return errno;
    }
    pFlag->FromToMode = FROMTO_DOS2UNIX; /* default dos2unix */
    /* Default when converting to Unix line breaks: do not write a Byte
     * Order Mark (option -b, --keep-bom, changes this). */
    pFlag->keep_bom = false;

    /* Find the base name of the program in argv[0]: the text after the last
     * '/' or, when there is no '/', after the last '\\' (DOS and Windows
     * paths). The '\\' is only searched when no '/' was found, exactly as
     * when the two assignments were still inside one 'if' condition. */
    ptr = strrchr(argv[0], '/');
    if (ptr == nullptr) {
        ptr = strrchr(argv[0], '\\');
    }
    if (ptr == nullptr) {
        ptr = argv[0];
    } else {
        ptr++;
    }

    if ((strcmpi("mac2unix", ptr) == 0) || (strcmpi("mac2unix.exe", ptr) == 0)) {
        pFlag->FromToMode = FROMTO_MAC2UNIX;
        d2u_strncpy(progname, "mac2unix", sizeof(progname));
    }

#ifdef D2U_UNIFILE
    /* Get arguments in wide Unicode format in the Windows Command Prompt */

    /* This does not support wildcard expansion (globbing) */
    wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

    /* AUDIT NOTE (C23 modernization review, code intentionally unchanged
     * apart from nullptr, this branch cannot be built or tested on the Unix
     * development host): argv_glob points to a single 'char **', so the
     * exact size would be sizeof(char **). All object pointers have the
     * same size on the Windows targets, so the allocation is large enough. */
    argv_glob = (char ***)malloc(sizeof(char ***));
    if (argv_glob == nullptr) {
        D2U_UTF8_FPRINTF(stderr, "%s:", progname);
        D2U_ANSI_FPRINTF(stderr, " %s\n", strerror(errno));
        free(pFlag);
        /* Exit status: the system error number, as documented in the
         * dos2unix(1) manual page (see the comment above main()). */
        return errno;
    }
    /* Glob the arguments and convert them to UTF-8 */
    argc_new = glob_warg(argc, wargv, argv_glob, pFlag, progname);
    argv_new = *argv_glob;
#else
    argc_new = argc;
    argv_new = argv;
#endif

#ifdef D2U_UNICODE
    ret = parse_options(argc_new, argv_new, pFlag, localedir, progname, PrintLicense,
                        ConvertDosToUnix, ConvertDosToUnixW);
#else
    ret = parse_options(argc_new, argv_new, pFlag, localedir, progname, PrintLicense,
                        ConvertDosToUnix);
#endif
    free(pFlag);
    /* Exit status: the result of parse_options(), returned unchanged to
     * keep the command-line behavior that the dos2unix(1) manual page
     * documents (see the comment above main()): 0 on success, which means
     * success just like EXIT_SUCCESS; 1 for an error that is not a system
     * error; or the errno value of the last system error. */
    return ret;
}
