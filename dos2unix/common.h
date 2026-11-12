/*
 *   Copyright (C) 2009-2026 Erwin Waterlander
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice in the documentation and/or other materials provided with
 *      the distribution.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 *   EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *   PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 *   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 *   OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __D2U_COMMON_H
#define __D2U_COMMON_H

/* define feature test macros for realpath() -- needed on    */
/* systems that have S_ISLNK, but chicken/egg means we must  */
/* define early, before including stdlib.h (or sys/stat.h)   */
/*   Defining _XOPEN_SOURCE results in undefined lstat() on FreeBSD 10.1. EW 2015-05-03 */
/* #define _XOPEN_SOURCE 500 */

/* similarly, instead of realpath we like to use, if         */
/* available, the canonicalize_file_name() function, which   */
/* is a GNU extension. We only ACTUALLY use the function if  */
/* USE_CANONICALIZE_FILE_NAME is defined, but we don't define*/
/* that until later. So...define the feature test macro now. */
#define _GNU_SOURCE

#ifdef ENABLE_NLS

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop(String)

#else

#define _(String) (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)

#endif

#if defined(__DJGPP__) || defined(__TURBOC__) /* DJGPP */
#include <dir.h>
#else
#if !defined(__MSYS__) && !defined(_MSC_VER)
#include <libgen.h>
#endif
#endif
#if !defined(__TURBOC__) && !defined(_MSC_VER)
#include <unistd.h>
#endif
/* <stdint.h> (ISO C23 modernization): uint32_t is the type of the bitmask
 * fields CFlag.status and CFlag.file_info and of their bit constants below.
 * Bitwise operations on a signed int are not portable for all values; a
 * fixed-width unsigned type makes '|=', '&' and '~' well defined. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
#ifndef strcmpi
#include <strings.h>
#define strcmpi(s1, s2) strcasecmp(s1, s2)
#endif
#endif
#ifdef _MSC_VER
#include <sys/utime.h>
#else
#include <utime.h>
#endif
#include <limits.h>
#ifdef __TURBOC__
#define __FLAT__
#endif
#include <errno.h>
#include <sys/stat.h>
#if defined(D2U_UNICODE) || defined(_WIN32)
#include <wchar.h>
#endif

#if (defined(__WATCOMC__) && defined(__NT__)) /* Watcom */
#define _WIN32 1
#endif

#if defined(__WATCOMC__) && defined(__I86__) /* Watcom C, 16 bit Intel */
#define __MSDOS__ 1
#endif

#if defined(__WATCOMC__) && defined(__DOS__) /* Watcom C, 32 bit DOS */
#define __MSDOS__ 1
#endif

#if defined(ENABLE_NLS) ||                                                                         \
    (defined(D2U_UNICODE) && !defined(__MSDOS__) && !defined(_WIN32) && !defined(__OS2__))
/* setlocale() is also needed for nl_langinfo() */
#include <locale.h>
#endif

/* Watcom C has mkstemp, but no mktemp().
 * MinGW has mktemp() and mkstemp(). MinGW mkstemp() is not working for me.
 * MSVC has mktemp(), but no mkstemp().
 * Assume that none of the Windows compilers have mkstemp().
 * On Windows I need something that can also work with Unicode file names (UTF-16).
 * On Windows GetTempFileName() will be used, as is adviced on MSDN. */
#if defined(__TURBOC__) || defined(__DJGPP__) || (defined(_WIN32) && !defined(__CYGWIN__))
/* Some compilers have no mkstemp().
 * Use mktemp() instead.
 * BORLANDC, DJGPP, MINGW32, MSVC */
#define NO_MKSTEMP 1
#endif

#if defined(__TURBOC__) || defined(__DJGPP__) || defined(__MINGW32__) || defined(__WATCOMC__) ||   \
    defined(_MSC_VER)
/* Some compilers have no chown(). */
#define NO_CHOWN 1
#endif

/* Watcom C defines S_ISLNK */
#ifdef __WATCOMC__
#undef S_ISLNK
#endif

/* Microsoft Visual C++ */
#ifdef _MSC_VER
#define S_ISCHR(m) (((m) & _S_IFMT) == _S_IFCHR)
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#define S_ISFIFO(m) (((m) & _S_IFMT) == _S_IFIFO)
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#define NO_CHMOD 1 /* no chmod() available */
#endif

#if defined(__MSDOS__) || defined(_WIN32) || defined(__OS2__)
/* Systems without soft links use 'stat' instead of 'lstat'. */
#define STAT stat
#else
#define STAT lstat
#endif

#if defined(__MSDOS__) || defined(_WIN32) || defined(__OS2__)
/* On some systems rename() will always fail if target file already exists. */
#define NEED_REMOVE 1
#endif

#if defined(__MSDOS__) || defined(_WIN32) || defined(__CYGWIN__) ||                                \
    defined(__OS2__) /* DJGPP, MINGW32 and OS/2 */
/* required for setmode() and O_BINARY */
#include <fcntl.h>
#include <io.h>
#endif

#if defined(__MSDOS__) || defined(_WIN32) || defined(__CYGWIN__) || defined(__OS2__)
#define R_CNTRL "rb"
#define W_CNTRL "wb"
#else
#define R_CNTRL "r"
#define W_CNTRL "w"
#endif
#define R_CNTRLW L"rb"
#define W_CNTRLW L"wb"

/*
 * ISO C23 constexpr constants.
 *
 * The integer constants below were '#define' macros. They are now constexpr
 * objects: named compile time constants that have a type and obey scope
 * rules, and that are still constant expressions (usable as array size, in
 * 'case' labels and in static_assert). A file scope constexpr object has
 * internal linkage, so defining them in this header does not create
 * duplicate symbols.
 * They cannot be used in '#if' preprocessor conditions. None of them is.
 * The feature switches (NO_MKSTEMP, NO_CHOWN, STAT, ...) and the fopen()
 * mode strings (R_CNTRL, ...) above stay macros, because they are tested
 * with '#ifdef' or are string literals.
 */

/* Conversion status bits, stored in CFlag.status. The field is reset to 0
 * at the start of every file, more than one bit can be set. The
 * print_messages*() functions in common.c translate the bits to messages.
 * Type uint32_t, the same type as CFlag.status, so that every '|=' and '&'
 * on the status is done on unsigned operands. */
constexpr uint32_t BINARY_FILE = 0x1;
constexpr uint32_t NO_REGFILE = 0x2;
constexpr uint32_t WRONG_CODEPAGE = 0x4;
constexpr uint32_t OUTPUTFILE_SYMLINK = 0x8;
constexpr uint32_t INPUT_TARGET_NO_REGFILE = 0x10;
constexpr uint32_t OUTPUT_TARGET_NO_REGFILE = 0x20;
constexpr uint32_t LOCALE_NOT_UTF = 0x40; /* Locale not an Unicode Transformation Format */
constexpr uint32_t WCHAR_T_TOO_SMALL = 0x80;
constexpr uint32_t UNICODE_CONVERSION_ERROR = 0x100;
constexpr uint32_t UNICODE_NOT_SUPPORTED = 0x200;

/* Conversion modes, stored in CFlag.ConvMode.
 * The values of the iso modes are the DOS/Windows code page numbers. The
 * option -iso stores the number returned by query_con_codepage() directly
 * in ConvMode, so these values must stay equal to the code page numbers. */
constexpr int CONVMODE_ASCII = 0;
constexpr int CONVMODE_UTF16LE = 1;
constexpr int CONVMODE_UTF16BE = 2;
constexpr int CONVMODE_7BIT = 3;
constexpr int CONVMODE_437 = 437;
constexpr int CONVMODE_850 = 850;
constexpr int CONVMODE_860 = 860;
constexpr int CONVMODE_863 = 863;
constexpr int CONVMODE_865 = 865;
constexpr int CONVMODE_1252 = 1252;

/* Conversion direction, stored in CFlag.FromToMode. */
constexpr int FROMTO_DOS2UNIX = 0;
constexpr int FROMTO_MAC2UNIX = 1;
constexpr int FROMTO_UNIX2DOS = 2;
constexpr int FROMTO_UNIX2MAC = 3;

/* File information flags, stored in CFlag.file_info. Set by the option
 * -i/--info[=FLAGS]. The value 0 means: no information mode, files are
 * converted. INFO_DEFAULT is the combination of the first five flags.
 * INFO_DOS, INFO_UNIX and INFO_MAC are also used as the type of the last
 * line break of a file ('last_eol' in common.c, 0 means no line break).
 * Type uint32_t, for the same reason as the status bits above. */
constexpr uint32_t INFO_DOS = 0x1;
constexpr uint32_t INFO_UNIX = 0x2;
constexpr uint32_t INFO_MAC = 0x4;
constexpr uint32_t INFO_BOM = 0x8;
constexpr uint32_t INFO_TEXT = 0x10;
constexpr uint32_t INFO_DEFAULT = 0x1F;
constexpr uint32_t INFO_CONVERT = 0x20;
constexpr uint32_t INFO_HEADER = 0x40;
constexpr uint32_t INFO_NOPATH = 0x80;
constexpr uint32_t INFO_PRINT0 = 0x100;
constexpr uint32_t INFO_EOL = 0x200;

/* Symbolic link handling, stored in CFlag.Follow. SYMLINK_SKIP must stay 0:
 * common.c tests '!ipFlag->Follow' for the skip mode. */
constexpr int SYMLINK_SKIP = 0;
constexpr int SYMLINK_FOLLOW = 1;
constexpr int SYMLINK_REPLACE = 2;

/* Type of byte order mark (BOM) of a file, stored in CFlag.bomtype and set
 * by read_bom(). FILE_MBS must stay 0: it means "no BOM", and common.c
 * tests 'bomtype > 0' for "the file has a BOM". */
constexpr int FILE_MBS = 0;     /* Multi-byte string or 8-bit char */
constexpr int FILE_UTF16LE = 1; /* UTF-16 Little Endian */
constexpr int FILE_UTF16BE = 2; /* UTF-16 Big Endian */
constexpr int FILE_UTF8 = 3;    /* UTF-8 */
constexpr int FILE_GB18030 = 4; /* GB18030 */

/* Encoding of the messages printed in the Windows console, see
 * d2u_utf8_fprintf() in common.c (Windows with Unicode file names only). */
constexpr int D2U_DISPLAY_ANSI = 0;
constexpr int D2U_DISPLAY_UNICODE = 1;
constexpr int D2U_DISPLAY_UNICODEBOM = 2;
constexpr int D2U_DISPLAY_UTF8 = 3;
constexpr int D2U_DISPLAY_UTF8BOM = 4;

/* locale conversion targets */
constexpr int TARGET_UTF8 = 0;
constexpr int TARGET_GB18030 = 1;
/* Size, in characters, of the buffers for file names and messages in the
 * Windows specific code (UTF-8 <-> UTF-16 conversion of names). Type int,
 * because it is also passed as the int buffer size of the Windows API. */
constexpr int D2U_MAX_PATH = 2048;

/* Conversion options and state, shared by all conversion functions. One
 * instance is allocated in main() of dos2unix.c and unix2dos.c and passed
 * down as ipFlag or pFlag. It is used inside one program only (never
 * written to a file or shared with another binary), so the field types
 * can change without compatibility concerns.
 *
 * ISO C23 types:
 * - 'bool' for the fields that only ever hold yes or no.
 * - 'uint32_t' for the two bitmask fields 'status' and 'file_info'.
 * - 'int' is kept for the fields that hold one of a group of constants
 *   (ConvMode, FromToMode, Follow, bomtype, locale_target), a level
 *   (verbose), or an errno value (error). These are not booleans. */
typedef struct {
    bool NewFile;         /* is in new file mode? */
    int verbose;          /* 0 = quiet, 1 = normal, 2 = verbose */
    bool KeepDate;        /* should keep date stamp? */
    int ConvMode;         /* conversion mode, one of CONVMODE_*: ascii, assumed UTF-16LE,
                             assumed UTF-16BE, 7bit, or iso with its code page number */
    int FromToMode;       /* 0: dos2unix/unix2dos, 1: mac2unix/unix2mac; one of FROMTO_* */
    bool NewLine;         /* if true, then additional newline */
    bool Force;           /* if true, force conversion of all files. */
    bool AllowChown;      /* if true, allow file ownership change in old file mode. */
    int Follow;           /* 0: skip symlink, 1: follow symbolic link, 2: replace symlink. */
    uint32_t status;      /* bitmask of the conversion status bits (BINARY_FILE, ...) of
                             the current file; reset to 0 for every file */
    bool stdio_mode;      /* if true, stdio mode */
    bool to_stdout;       /* write output to stdout in old file mode */
    int error;            /* an error occurred. 0: no error; otherwise 1 or an errno
                             value. It becomes the exit status, so it is not a bool. */
    bool error_binary;    /* if true, return an error if a binary file is detected. */
    int bomtype;          /* byte order mark, one of FILE_* */
    bool add_bom;         /* true: write BOM */
    bool keep_bom;        /* true: write BOM if input file has BOM. false: Do not write BOM */
    bool keep_utf16;      /* true: write UTF-16 format when input file is UTF-16 format */
    uint32_t file_info;   /* bitmask of INFO_* flags. 0: convert files; not 0: print
                             file information instead of converting */
    int locale_target;    /* locale conversion target. 0: UTF-8; 1: GB18030 */
    unsigned int line_nr; /* line number where UTF-16 error occurs */
    bool add_eol;         /* Add End Of Line to last line */
} CFlag;

/* ISO C23 [[nodiscard]].
 *
 * Every function below that returns an error code, a status, a stream or
 * an allocated pointer is declared [[nodiscard]]. Ignoring such a result
 * means that a failed open, read, write or allocation goes unnoticed, so
 * the compiler is asked to diagnose every call that drops the result. The
 * attribute on the declaration here also applies to the definition in
 * common.c. Where a result is ignored on purpose, the call is written with
 * a (void) cast and a comment that explains why.
 *
 * The predicates symbolic_link(), binaryChar() and binaryCharW() return
 * 'bool' (they returned int 1 or 0 before); their only effect is the
 * result, so they are [[nodiscard]] as well.
 *
 * Not marked on purpose: d2u_strncpy(). It returns its 'dest' argument,
 * like strncpy(), only as a convenience; it cannot fail. */
[[nodiscard]] bool symbolic_link(const char *path);
[[nodiscard]] int regfile(char *path, bool allowSymlinks, CFlag *ipFlag, const char *progname);
[[nodiscard]] int regfile_target(char *path, CFlag *ipFlag, const char *progname);
void PrintUsage(const char *progname);
void PrintBSDLicense(void);
void PrintVersion(const char *progname, const char *localedir);
#ifdef ENABLE_NLS
void PrintLocaledir(const char *localedir);
#endif
[[nodiscard]] FILE *OpenInFile(char *ipFN);
#if !defined(__MSDOS__) && !defined(_WIN32) && !defined(__OS2__)
[[nodiscard]] FILE *OpenInFileNoFollow(const char *ipFN, struct stat *statbuf);
#endif
[[nodiscard]] FILE *OpenOutFile(char *opFN);
[[nodiscard]] FILE *OpenOutFiled(int fd);
#if defined(__TURBOC__) || defined(__MSYS__)
char *dirname(char *path);
#endif
[[nodiscard]] FILE *MakeTempFileFrom(const char *OutFN, char **fname_ret);
[[nodiscard]] int ResolveSymbolicLink(char *lFN, char **rFN, CFlag *ipFlag, const char *progname);

/* Lookahead buffer to be able to do safe consecutive ungetc() calls (max 4).
   Standard ungetc() may fail when called more than once after each other. */
/* Invariants of BufferedStream:
 * - 0 <= count <= MAX_LOOKAHEAD. buffer[0] .. buffer[count - 1] hold the
 *   pushed back characters. d2u_getc() returns buffer[count - 1] first:
 *   the character that was pushed back last is read first (LIFO).
 * - EOF is never stored. d2u_ungetc() refuses EOF, and it returns EOF
 *   without storing anything when the buffer is full. Its result must be
 *   checked, otherwise input is lost silently.
 * - MAX_LOOKAHEAD must be at least the length of the longest byte order
 *   mark (GB18030, 4 bytes): read_bom() pushes back every byte it has read
 *   when the bytes turn out not to be a BOM. common.c verifies this with a
 *   static_assert.
 * ISO C23: MAX_LOOKAHEAD is a constexpr int instead of a macro. It is still
 * a constant expression, so it can be the array size below. */
constexpr int MAX_LOOKAHEAD = 4;
typedef struct {
    FILE *file;
    int buffer[MAX_LOOKAHEAD];
    int count;
} BufferedStream;

[[nodiscard]] int d2u_getc(BufferedStream *stream);
[[nodiscard]] int d2u_ungetc(int c, BufferedStream *stream);

[[nodiscard]] BufferedStream *read_bom(BufferedStream *f, int *bomtype);
[[nodiscard]] FILE *write_bom(FILE *f, CFlag *ipFlag, const char *progname);
void print_bom(const int bomtype, const char *filename, const char *progname);
[[nodiscard]] int check_unicode(BufferedStream *InF, FILE *TempF, CFlag *ipFlag, const char *ipInFN,
                                const char *progname);
void print_messages_stdio(const CFlag *pFlag, const char *progname);
void print_messages_newfile(const CFlag *pFlag, const char *infile, const char *outfile,
                            const char *progname, const int RetVal);
void print_messages_oldfile(const CFlag *pFlag, const char *infile, const char *progname,
                            const int RetVal);

[[nodiscard]] int ConvertNewFile(char *ipInFN, char *ipOutFN, CFlag *ipFlag, const char *progname,
                                 int (*Convert)(BufferedStream *, FILE *, CFlag *, const char *)
#ifdef D2U_UNICODE
                                     ,
                                 int (*ConvertW)(BufferedStream *, FILE *, CFlag *, const char *)
#endif
);
[[nodiscard]] int ConvertStdio(CFlag *ipFlag, const char *progname,
                               int (*Convert)(BufferedStream *, FILE *, CFlag *, const char *)
#ifdef D2U_UNICODE
                                   ,
                               int (*ConvertW)(BufferedStream *, FILE *, CFlag *, const char *)
#endif
);
[[nodiscard]] int parse_options(int argc, char *argv[], CFlag *pFlag, const char *localedir,
                                const char *progname, void (*PrintLicense)(void),
                                int (*Convert)(BufferedStream *, FILE *, CFlag *, const char *)
#ifdef D2U_UNICODE
                                    ,
                                int (*ConvertW)(BufferedStream *, FILE *, CFlag *, const char *)
#endif
);
void d2u_getc_error(CFlag *ipFlag, const char *progname);
void d2u_putc_error(CFlag *ipFlag, const char *progname);

#ifdef D2U_UNICODE
void d2u_putwc_error(CFlag *ipFlag, const char *progname);
[[nodiscard]] wint_t d2u_getwc(BufferedStream *f, int bomtype);
void d2u_check_surrogate_state(CFlag *ipFlag, const char *progname);
[[nodiscard]] wint_t d2u_putwc(wint_t wc, FILE *f, CFlag *ipFlag, const char *progname);
#endif
char *d2u_strncpy(char *dest, const char *src, size_t dest_size);

#ifdef D2U_UNIFILE
#define UNICODE
#define _UNICODE
#define D2U_UTF8_FPRINTF d2u_utf8_fprintf
#define D2U_ANSI_FPRINTF d2u_ansi_fprintf
void d2u_utf8_fprintf(FILE *stream, const char *format, ...);
void d2u_ansi_fprintf(FILE *stream, const char *format, ...);
[[nodiscard]] int glob_warg(int argc, wchar_t *wargv[], char ***argv, CFlag *ipFlag,
                            const char *progname);
#else
#define D2U_UTF8_FPRINTF fprintf
#define D2U_ANSI_FPRINTF fprintf
#endif
void logConverted(int RetVal, int verbose, const char *progname, unsigned int converted,
                  int unsigned line_nr);
#ifdef D2U_UNICODE
void logBinaryCharW(CFlag *ipFlag, wint_t c, unsigned int line_nr, const char *progname);
[[nodiscard]] bool binaryCharW(wint_t c);
#endif
void logBinaryChar(CFlag *ipFlag, int c, unsigned int line_nr, const char *progname);
[[nodiscard]] bool binaryChar(int c);

#endif
