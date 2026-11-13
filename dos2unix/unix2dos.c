/*
 *  Name: unix2dos
 *  Documentation:
 *    Convert lf ('\x0a') characters in a file to cr lf ('\x0d' '\x0a')
 *    combinations.
 *
 *  The dos2unix package is distributed under FreeBSD style license.
 *  See also https://www.freebsd.org/copyright/freebsd-license.html
 *  --------
 *
 *  Copyright (C) 2009-2026 Erwin Waterlander
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
 *  == 1.2 == 1995.03.09 == Benjamin Lin (blin@socs.uts.edu.au)
 *     Fixed minor typo error
 *  == 1.3 == 1995.03.16 == Benjamin Lin (blin@socs.uts.edu.au)
 *     Modified to more conform to UNIX style.
 *  == 2.0 == 1995.03.19 == Benjamin Lin (blin@socs.uts.edu.au)
 *     Rewritten from scratch.
 *  == 2.2 == 1995.03.30 == Benjamin Lin (blin@socs.uts.edu.au)
 *     Conversion from SunOS charset implemented.
 *
 *  See ChangeLog.txt for complete version history.
 *
 */

/* #define DEBUG 1 */
/* D2U_UNIX2DOS_C selects the conversion tables of unix2dos.h for this file only,
 * see the explanation in unix2dos.h. It must be defined before the header is
 * included. */
#define D2U_UNIX2DOS_C

#include "unix2dos.h"
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
 * "unix2dos" or "unix2mac" and the terminating null. The static_assert
 * makes the build fail if the two names ever get a different length,
 * instead of truncating the longer name at run time. */
static constexpr size_t D2U_PROGNAME_SIZE = sizeof("unix2dos");
static_assert(sizeof("unix2mac") == D2U_PROGNAME_SIZE,
              "unix2dos and unix2mac must have the same name length");

/* Size of the buffer in main() for the directory of the message catalogs
 * (ENABLE_NLS). A longer value of DOS2UNIX_LOCALEDIR is rejected. */
static constexpr size_t D2U_LOCALEDIR_SIZE = 1024;

/* Print the copyright notice of this program and the license text.
 * main() passes this function to parse_options(), which calls it for the
 * --license option. */
void PrintLicense(void) {
    D2U_ANSI_FPRINTF(stdout, _("\
Copyright (C) 2009-%d Erwin Waterlander\n\
Copyright (C) 1994-1995 Benjamin Lin\n\
All rights reserved.\n\n"),
                     D2U_COPYRIGHT_LAST_YEAR);
    PrintBSDLicense();
}

/* Design notes for the conversion functions in this file.
 *
 * Why there are two families of conversion functions:
 * - unix2dos() and unix2mac() work on bytes. They read with d2u_getc() and
 *   write with fputc(). They are used for ASCII, the 8 bit code pages,
 *   UTF-8 and GB18030. In these encodings a byte 0x0a or 0x0d is always a
 *   real LF or CR, so a file can be converted byte by byte. Every byte that
 *   is written passes through a conversion table of unix2dos.h; that is
 *   how the -7 and -iso modes translate the 8 bit characters.
 * - unix2dosW() and unix2macW() exist only with D2U_UNICODE and are used
 *   for UTF-16 input. A UTF-16 code unit is two bytes, and a byte 0x0a or
 *   0x0d can be one half of another character (U+010A for example), so the
 *   bytes cannot be judged one by one. d2u_getwc() reads whole 16 bit code
 *   units as wint_t, and d2u_putwc() writes them in the output encoding
 *   and joins surrogate pairs. The conversion tables are not used here.
 *   common.c calls the W function when the input has a UTF-16 BOM or when
 *   UTF-16 is assumed (-ul, -ub), and the byte function otherwise.
 *
 * Why the functions remember the previous character and look ahead:
 * An LF must only get a CR in front of it when it does not already follow
 * a CR. Otherwise converting a DOS file a second time would turn CR LF
 * into CR CR LF.
 * - unix2dos mode: an LF gets a CR only when PreviousChar is not a CR. When
 *   a CR is read, the next character is read at once and both are written
 *   in the same pass of the loop, with PreviousChar set to CR. A following
 *   LF is then seen as an existing DOS line break and is written
 *   unchanged, and a CR that is not followed by LF is written unchanged.
 * - unix2mac mode: an LF becomes CR, except the LF of CR LF, which is
 *   written unchanged (an existing DOS line break is preserved).
 * The character that is read ahead is kept in a variable and used in the
 * same pass; nothing is pushed back to the input stream. d2u_ungetc() is
 * only used by read_bom() in common.c, and there is no push back function
 * for wide characters. This needs no buffer and cannot fail.
 *
 * AUDIT NOTE (seen during the C23 modernization review, behavior
 * intentionally unchanged): in unix2dos() and unix2dosW() the character
 * that is read ahead after a CR is not passed to binaryChar() or
 * binaryCharW(). A binary symbol directly after a CR is therefore not
 * detected.
 *
 * Common to the four conversion functions:
 * - They return 0 on success and -1 on error. Errors are: a binary symbol
 *   in the input when conversion is not forced (BINARY_FILE is set in
 *   ipFlag->status), a read error and a write error (d2u_getc_error(),
 *   d2u_putc_error() and d2u_putwc_error() store errno in ipFlag->error).
 * - [[nodiscard]] (ISO C23): the return value is the only sign that the
 *   output is incomplete. Ignoring it would let a damaged temporary file
 *   replace the input file. The same holds for the four small line break
 *   helpers below, which return EOF or WEOF when writing failed.
 */

#ifdef D2U_UNICODE
/* Write a DOS line break (CR LF) to ipOutF as wide characters.
 * Returns the LF on success. Returns WEOF when writing failed; the error
 * was then already reported with d2u_putwc_error(). */
[[nodiscard]] wint_t PutDOSNewLineW(FILE *ipOutF, CFlag *ipFlag, const char *progname) {
    if (d2u_putwc(D2U_WCHAR_CR, ipOutF, ipFlag, progname) == WEOF) {
        d2u_putwc_error(ipFlag, progname);
        return WEOF;
    }
    if (d2u_putwc(D2U_WCHAR_LF, ipOutF, ipFlag, progname) == WEOF) {
        d2u_putwc_error(ipFlag, progname);
        return WEOF;
    }
    return D2U_WCHAR_LF;
}

/* Option -l (--newline): write an extra DOS line break after a Unix line
 * break. CurChar is the character that was just written and PrevChar the
 * one before it. Returns CurChar when nothing was added, the LF of the
 * added line break otherwise, and WEOF when writing failed. */
[[nodiscard]] wint_t AddExtraDOSNewLineW(FILE *ipOutF, CFlag *ipFlag, wint_t CurChar,
                                         wint_t PrevChar, const char *progname) {
    wint_t c = CurChar;
    /* Don't add line ending if it is a DOS line ending. Only in case of Unix line ending. */
    if ((CurChar == D2U_WCHAR_LF) && (PrevChar != D2U_WCHAR_CR)) {
        c = PutDOSNewLineW(ipOutF, ipFlag, progname);
        if (c == WEOF) {
            return WEOF;
        }
    }
    return c;
}
#endif

/* Write a DOS line break (CR LF) to ipOutF as bytes.
 * Returns the LF on success. Returns EOF when writing failed; the error
 * was then already reported with d2u_putc_error(). */
[[nodiscard]] int PutDOSNewLine(FILE *ipOutF, CFlag *ipFlag, const char *progname) {
    if (fputc(D2U_CHAR_CR, ipOutF) == EOF) {
        d2u_putc_error(ipFlag, progname);
        return EOF;
    }
    if (fputc(D2U_CHAR_LF, ipOutF) == EOF) {
        d2u_putc_error(ipFlag, progname);
        return EOF;
    }
    return D2U_CHAR_LF;
}

/* Option -l (--newline): write an extra DOS line break after a Unix line
 * break. CurChar is the character that was just written and PrevChar the
 * one before it. Returns CurChar when nothing was added, the LF of the
 * added line break otherwise, and EOF when writing failed. */
[[nodiscard]] int AddExtraDOSNewLine(FILE *ipOutF, CFlag *ipFlag, int CurChar, int PrevChar,
                                     const char *progname) {
    int c = CurChar;
    /* Don't add line ending if it is a DOS line ending. Only in case of Unix line ending. */
    if ((CurChar == D2U_CHAR_LF) && (PrevChar != D2U_CHAR_CR)) {
        c = PutDOSNewLine(ipOutF, ipFlag, progname);
        if (c == EOF) {
            return EOF;
        }
    }
    return c;
}

#ifdef D2U_UNICODE
/* Convert UTF-16 text from Unix to DOS line breaks (unix2dos mode).
 * See the design notes above. */
[[nodiscard]] int unix2dosW(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag,
                            const char *progname) {
    int RetVal = 0;
    wint_t PreviousChar = WEOF;
    wint_t TempChar;
    unsigned int line_nr = 1;
    unsigned int converted = 0;

    while ((TempChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) { /* get character */
        if ((!ipFlag->Force) && binaryCharW(TempChar)) {
            RetVal = -1;
            ipFlag->status |= BINARY_FILE;
            if (ipFlag->verbose) {
                logBinaryCharW(ipFlag, TempChar, line_nr, progname);
            }
            break;
        }
        if (TempChar == D2U_WCHAR_LF) {
            if (PreviousChar != D2U_WCHAR_CR) {
                if (d2u_putwc(D2U_WCHAR_CR, ipOutF, ipFlag, progname) ==
                    WEOF) { /* got LF, put extra CR */
                    RetVal = -1;
                    d2u_putwc_error(ipFlag, progname);
                    break;
                }
                converted++;
            }
        } else {
            if (TempChar == D2U_WCHAR_CR) {                   /* got CR */
                TempChar = d2u_getwc(ipInF, ipFlag->bomtype); /* get next char (possibly LF) */
                if (TempChar == WEOF) {
                    if (ferror(ipInF->file)) { /* Read error */
                        break;
                    }
                    TempChar = D2U_WCHAR_CR; /* end of file. */
                } else {
                    if (d2u_putwc(D2U_WCHAR_CR, ipOutF, ipFlag, progname) == WEOF) { /* put CR */
                        RetVal = -1;
                        d2u_putwc_error(ipFlag, progname);
                        break;
                    }
                    PreviousChar = D2U_WCHAR_CR;
                }
            }
        }
        if (TempChar == D2U_WCHAR_LF) { /* Count all DOS and Unix line breaks */
            ++line_nr;
        }
        if (d2u_putwc(TempChar, ipOutF, ipFlag, progname) == WEOF) {
            RetVal = -1;
            d2u_putwc_error(ipFlag, progname);
            break;
        }
        if (ipFlag->NewLine) { /* add additional CR-LF? */
            if (AddExtraDOSNewLineW(ipOutF, ipFlag, TempChar, PreviousChar, progname) == WEOF) {
                RetVal = -1;
                break;
            }
        }
        PreviousChar = TempChar;
    }

    if (TempChar == WEOF && ipFlag->add_eol && PreviousChar != WEOF &&
        PreviousChar != D2U_WCHAR_LF) {
        /* Add missing line break at the last line. */
        if (ipFlag->verbose > 1) {
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
            D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n"));
        }
        if (PutDOSNewLineW(ipOutF, ipFlag, progname) == WEOF) {
            RetVal = -1;
        }
    }
    if ((TempChar == WEOF) && ferror(ipInF->file)) {
        RetVal = -1;
        d2u_getc_error(ipFlag, progname);
    }

    d2u_check_surrogate_state(ipFlag, progname);
    if (ipFlag->status & UNICODE_CONVERSION_ERROR) {
        ipFlag->line_nr = line_nr;
    }
    logConverted(RetVal, ipFlag->verbose, progname, converted, line_nr);
    return RetVal;
}

/* Convert UTF-16 text from Unix to Mac line breaks (unix2mac mode).
 * See the design notes above. */
[[nodiscard]] int unix2macW(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag,
                            const char *progname) {
    int RetVal = 0;
    wint_t PreviousChar = WEOF;
    wint_t TempChar;
    unsigned int line_nr = 1;
    unsigned int converted = 0;

    while ((TempChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) {
        if ((!ipFlag->Force) && binaryCharW(TempChar)) {
            RetVal = -1;
            ipFlag->status |= BINARY_FILE;
            if (ipFlag->verbose) {
                logBinaryCharW(ipFlag, TempChar, line_nr, progname);
            }
            break;
        }
        if (TempChar != D2U_WCHAR_LF) { /* Not an LF */
            if (d2u_putwc(TempChar, ipOutF, ipFlag, progname) == WEOF) {
                RetVal = -1;
                d2u_putwc_error(ipFlag, progname);
                break;
            }
            PreviousChar = TempChar;
            if (TempChar == D2U_WCHAR_CR) { /* Count CR */
                ++line_nr;
            }
        } else {
            /* TempChar is an LF */
            if (PreviousChar != D2U_WCHAR_CR) { /* Count Unix line break, not DOS line break */
                ++line_nr;
            } else {
                --line_nr; /* CR already counted. Correction, don't count DOS line breaks */
            }
            /* Don't touch this delimiter if it's a CR,LF pair. */
            if (PreviousChar == D2U_WCHAR_CR) {
                if (d2u_putwc(D2U_WCHAR_LF, ipOutF, ipFlag, progname) ==
                    WEOF) { /* CR,LF pair. Put LF */
                    RetVal = -1;
                    d2u_putwc_error(ipFlag, progname);
                    break;
                }
                PreviousChar = TempChar;
                continue;
            }
            PreviousChar = TempChar;
            if (d2u_putwc(D2U_WCHAR_CR, ipOutF, ipFlag, progname) ==
                WEOF) { /* Unix line end (LF). Put CR */
                RetVal = -1;
                d2u_putwc_error(ipFlag, progname);
                break;
            }
            converted++;
            if (ipFlag->NewLine) { /* add additional CR? */
                if (d2u_putwc(D2U_WCHAR_CR, ipOutF, ipFlag, progname) == WEOF) {
                    RetVal = -1;
                    d2u_putwc_error(ipFlag, progname);
                    break;
                }
            }
        }
    }

    if (TempChar == WEOF && ipFlag->add_eol && PreviousChar != WEOF &&
        !(PreviousChar == D2U_WCHAR_LF || PreviousChar == D2U_WCHAR_CR)) {
        /* Add missing line break at the last line. */
        if (ipFlag->verbose > 1) {
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
            D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n"));
        }
        if (d2u_putwc(D2U_WCHAR_CR, ipOutF, ipFlag, progname) == WEOF) {
            RetVal = -1;
            d2u_putwc_error(ipFlag, progname);
        }
    }
    if ((TempChar == WEOF) && ferror(ipInF->file)) {
        RetVal = -1;
        d2u_getc_error(ipFlag, progname);
    }

    d2u_check_surrogate_state(ipFlag, progname);
    if (ipFlag->status & UNICODE_CONVERSION_ERROR) {
        ipFlag->line_nr = line_nr;
    }
    logConverted(RetVal, ipFlag->verbose, progname, converted, line_nr);
    return RetVal;
}

/* converts stream ipInF to DOS format text and write to stream ipOutF
 * RetVal: 0  if success
 *         -1  otherwise
 *
 * UTF-16 variant, see the design notes above. It resets ipFlag->status and
 * calls unix2dosW() or unix2macW(), depending on ipFlag->FromToMode. An
 * unknown FromToMode converts nothing and returns 0; a DEBUG build ends
 * the program instead.
 * [[nodiscard]] (ISO C23): see the prototype in unix2dos.h.
 */
[[nodiscard]] int ConvertUnixToDosW(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag,
                                    const char *progname) {
    int RetVal = 0;

    ipFlag->status = 0;

    /* LF    -> CR-LF */
    /* CR-LF -> CR-LF, in case the input file is a DOS text file */
    /* \x0a = Newline/Line Feed (LF) */
    /* \x0d = Carriage Return (CR) */

    switch (ipFlag->FromToMode) {
    case FROMTO_UNIX2DOS: /* unix2dos */
        RetVal = unix2dosW(ipInF, ipOutF, ipFlag, progname);
        break;
    case FROMTO_UNIX2MAC: /* unix2mac */
        RetVal = unix2macW(ipInF, ipOutF, ipFlag, progname);
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

/* Convert byte oriented text from Unix to DOS line breaks (unix2dos mode).
 * ConvTable is one of the conversion tables of unix2dos.h, it has an entry
 * for every byte value. A character is only used as an index after it was
 * compared with EOF, so the index is always 0..255.
 * See the design notes above. */
[[nodiscard]] int unix2dos(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag, const char *progname,
                           const int *ConvTable) {
    int RetVal = 0;
    int PreviousChar = EOF;
    int TempChar;
    unsigned int line_nr = 1;
    unsigned int converted = 0;

    while ((TempChar = d2u_getc(ipInF)) != EOF) { /* get character */
        if ((!ipFlag->Force) && binaryChar(TempChar)) {
            RetVal = -1;
            ipFlag->status |= BINARY_FILE;
            if (ipFlag->verbose) {
                logBinaryChar(ipFlag, TempChar, line_nr, progname);
            }
            break;
        }
        if (TempChar == D2U_CHAR_LF) {
            if (PreviousChar != D2U_CHAR_CR) {
                if (fputc(D2U_CHAR_CR, ipOutF) == EOF) { /* got LF, put extra CR */
                    RetVal = -1;
                    d2u_putc_error(ipFlag, progname);
                    break;
                }
                converted++;
            }
        } else {
            if (TempChar == D2U_CHAR_CR) {  /* got CR */
                TempChar = d2u_getc(ipInF); /* get next char (possibly LF) */
                if (TempChar == EOF) {
                    if (ferror(ipInF->file)) { /* Read error */
                        break;
                    }
                    TempChar = D2U_CHAR_CR; /* end of file. */
                } else {
                    if (fputc(D2U_CHAR_CR, ipOutF) == EOF) { /* put CR */
                        RetVal = -1;
                        d2u_putc_error(ipFlag, progname);
                        break;
                    }
                    PreviousChar = D2U_CHAR_CR;
                }
            }
        }
        if (TempChar == D2U_CHAR_LF) { /* Count all DOS and Unix line breaks */
            ++line_nr;
        }
        if (fputc(ConvTable[TempChar], ipOutF) == EOF) { /* put LF or other char */
            RetVal = -1;
            d2u_putc_error(ipFlag, progname);
            break;
        }
        if (ipFlag->NewLine) { /* add additional CR-LF? */
            if (AddExtraDOSNewLine(ipOutF, ipFlag, TempChar, PreviousChar, progname) == EOF) {
                RetVal = -1;
                break;
            }
        }
        PreviousChar = TempChar;
    }

    if (TempChar == EOF && ipFlag->add_eol && PreviousChar != EOF && PreviousChar != D2U_CHAR_LF) {
        /* Add missing line break at the last line. */
        if (ipFlag->verbose > 1) {
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
            D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n"));
        }
        if (PutDOSNewLine(ipOutF, ipFlag, progname) == EOF) {
            RetVal = -1;
        }
    }
    if ((TempChar == EOF) && ferror(ipInF->file)) {
        RetVal = -1;
        d2u_getc_error(ipFlag, progname);
    }

    logConverted(RetVal, ipFlag->verbose, progname, converted, line_nr);
    return RetVal;
}

/* Convert byte oriented text from Unix to Mac line breaks (unix2mac mode).
 * ConvTable and its index: see unix2dos(). See the design notes above. */
[[nodiscard]] int unix2mac(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag, const char *progname,
                           const int *ConvTable) {
    int RetVal = 0;
    int PreviousChar = EOF;
    int TempChar;
    unsigned int line_nr = 1;
    unsigned int converted = 0;

    while ((TempChar = d2u_getc(ipInF)) != EOF) {
        if ((!ipFlag->Force) && binaryChar(TempChar)) {
            RetVal = -1;
            ipFlag->status |= BINARY_FILE;
            if (ipFlag->verbose) {
                logBinaryChar(ipFlag, TempChar, line_nr, progname);
            }
            break;
        }
        if (TempChar != D2U_CHAR_LF) { /* Not an LF */
            if (fputc(ConvTable[TempChar], ipOutF) == EOF) {
                RetVal = -1;
                d2u_putc_error(ipFlag, progname);
                break;
            }
            PreviousChar = TempChar;
            if (TempChar == D2U_CHAR_CR) { /* Count CR */
                ++line_nr;
            }
        } else {
            /* TempChar is an LF */
            if (PreviousChar != D2U_CHAR_CR) { /* Count Unix line break, not DOS line break */
                ++line_nr;
            } else {
                --line_nr; /* CR already counted. Correction, don't count DOS line breaks */
            }
            /* Don't touch this delimiter if it's a CR,LF pair. */
            if (PreviousChar == D2U_CHAR_CR) {
                if (fputc(D2U_CHAR_LF, ipOutF) == EOF) { /* CR,LF pair. Put LF */
                    RetVal = -1;
                    d2u_putc_error(ipFlag, progname);
                    break;
                }
                PreviousChar = TempChar;
                continue;
            }
            PreviousChar = TempChar;
            if (fputc(D2U_CHAR_CR, ipOutF) == EOF) { /* Unix line end (LF). Put CR */
                RetVal = -1;
                d2u_putc_error(ipFlag, progname);
                break;
            }
            converted++;
            if (ipFlag->NewLine) { /* add additional CR? */
                if (fputc(D2U_CHAR_CR, ipOutF) == EOF) {
                    RetVal = -1;
                    d2u_putc_error(ipFlag, progname);
                    break;
                }
            }
        }
    }

    if (TempChar == EOF && ipFlag->add_eol && PreviousChar != EOF &&
        !(PreviousChar == D2U_CHAR_LF || PreviousChar == D2U_CHAR_CR)) {
        /* Add missing line break at the last line. */
        if (ipFlag->verbose > 1) {
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
            D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n"));
        }
        if (fputc(D2U_CHAR_CR, ipOutF) == EOF) {
            RetVal = -1;
            d2u_putc_error(ipFlag, progname);
        }
    }
    if ((TempChar == EOF) && ferror(ipInF->file)) {
        RetVal = -1;
        d2u_getc_error(ipFlag, progname);
    }

    logConverted(RetVal, ipFlag->verbose, progname, converted, line_nr);
    return RetVal;
}

/* converts stream ipInF to DOS format text and write to stream ipOutF
 * RetVal: 0  if success
 *         -1  otherwise
 *
 * Byte oriented variant, see the design notes above. It resets
 * ipFlag->status, selects the conversion table for ipFlag->ConvMode and
 * calls unix2dos() or unix2mac(), depending on ipFlag->FromToMode.
 * An unknown ConvMode sets WRONG_CODEPAGE in ipFlag->status and returns -1.
 * An unknown FromToMode converts nothing and returns 0; a DEBUG build ends
 * the program instead.
 * [[nodiscard]] (ISO C23): see the prototype in unix2dos.h.
 */
[[nodiscard]] int ConvertUnixToDos(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag,
                                   const char *progname) {
    int RetVal = 0;
    /* Points to one of the read-only (constexpr) conversion tables of
     * unix2dos.h, hence pointer to const. */
    const int *ConvTable;

    ipFlag->status = 0;

    switch (ipFlag->ConvMode) {
    case CONVMODE_ASCII:   /* ascii */
    case CONVMODE_UTF16LE: /* Assume UTF-16LE, bomtype = FILE_UTF8 or GB18030 */
    case CONVMODE_UTF16BE: /* Assume UTF-16BE, bomtype = FILE_UTF8 or GB18030 */
        ConvTable = U2DAsciiTable;
        break;
    case CONVMODE_7BIT: /* 7bit */
        ConvTable = U2D7BitTable;
        break;
    case CONVMODE_437: /* iso */
        ConvTable = U2DIso437Table;
        break;
    case CONVMODE_850: /* iso */
        ConvTable = U2DIso850Table;
        break;
    case CONVMODE_860: /* iso */
        ConvTable = U2DIso860Table;
        break;
    case CONVMODE_863: /* iso */
        ConvTable = U2DIso863Table;
        break;
    case CONVMODE_865: /* iso */
        ConvTable = U2DIso865Table;
        break;
    case CONVMODE_1252: /* iso */
        ConvTable = U2DIso1252Table;
        break;
    default: /* unknown convmode */
        ipFlag->status |= WRONG_CODEPAGE;
        return (-1);
    }
    /* Turn off ISO and 7-bit conversion for Unicode text files */
    if (ipFlag->bomtype > 0) {
        ConvTable = U2DAsciiTable;
    }

    if ((ipFlag->ConvMode > CONVMODE_7BIT) && (ipFlag->verbose)) { /* not ascii or 7bit */
        D2U_UTF8_FPRINTF(stderr, "%s: ", progname);
        D2U_UTF8_FPRINTF(stderr, _("using code page %d.\n"), ipFlag->ConvMode);
    }

    /* LF    -> CR-LF */
    /* CR-LF -> CR-LF, in case the input file is a DOS text file */
    /* \x0a = Newline/Line Feed (LF) */
    /* \x0d = Carriage Return (CR) */

    switch (ipFlag->FromToMode) {
    case FROMTO_UNIX2DOS: /* unix2dos */
        RetVal = unix2dos(ipInF, ipOutF, ipFlag, progname, ConvTable);
        break;
    case FROMTO_UNIX2MAC: /* unix2mac */
        RetVal = unix2mac(ipInF, ipOutF, ipFlag, progname, ConvTable);
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

/* Program entry point. The program converts as unix2dos, or as unix2mac when
 * it is started under the name unix2mac (or unix2mac.exe).
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
    d2u_strncpy(progname, "unix2dos", sizeof(progname));

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
        D2U_UTF8_FPRINTF(stderr, "unix2dos:");
        D2U_ANSI_FPRINTF(stderr, " %s\n", strerror(errno));
        /* Exit status: the system error number, as documented in the
         * dos2unix(1) manual page (see the comment above main()). Keep
         * 'errno', do not replace it by EXIT_FAILURE. */
        return errno;
    }
    pFlag->FromToMode = FROMTO_UNIX2DOS; /* default unix2dos */
    /* Default when converting to DOS line breaks: keep the Byte Order
     * Mark of the input file (option -r, --remove-bom, changes this). */
    pFlag->keep_bom = true;

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

    if ((strcmpi("unix2mac", ptr) == 0) || (strcmpi("unix2mac.exe", ptr) == 0)) {
        pFlag->FromToMode = FROMTO_UNIX2MAC;
        d2u_strncpy(progname, "unix2mac", sizeof(progname));
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
                        ConvertUnixToDos, ConvertUnixToDosW);
#else
    ret = parse_options(argc_new, argv_new, pFlag, localedir, progname, PrintLicense,
                        ConvertUnixToDos);
#endif
    free(pFlag);
    /* Exit status: the result of parse_options(), returned unchanged to
     * keep the command-line behavior that the dos2unix(1) manual page
     * documents (see the comment above main()): 0 on success, which means
     * success just like EXIT_SUCCESS; 1 for an error that is not a system
     * error; or the errno value of the last system error. */
    return ret;
}
