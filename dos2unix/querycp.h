/* querycp.h is in the public domain */

/* Include guard: this header is included by common.c and by querycp.c
 * itself, and must stay safe to include more than once per translation
 * unit. The guard name deliberately has no leading underscores, because
 * identifiers starting with "__" are reserved for the implementation. */
#ifndef D2U_QUERYCP_H
#define D2U_QUERYCP_H

/*
 * query_con_codepage() - get the currently active DOS/console code page.
 *
 * Used for the SunOS compatible "-iso" conversion mode, which converts
 * between a DOS code page and ISO-8859-1.
 *
 * What is returned depends on the platform branch compiled in querycp.c:
 *
 *   DOS (DJGPP, Watcom C 16/32 bit):
 *       The selected display (CON) code page, queried with int 21h,
 *       AX=440Ch, CX=036Ah, with the undocumented int 2Fh, AX=AD02h as
 *       fallback. If both methods fail the default 437 is returned.
 *   Windows (not Cygwin):
 *       The console's DOS (OEM) code page, GetConsoleOutputCP().
 *   OS/2:
 *       The current process code page, first entry of DosQueryCp().
 *   Unix, Cygwin and all other systems:
 *       Always 0. These systems have no DOS console code page.
 *
 * Return value: the code page number (e.g. 437, 850), or 0 when the code
 * page is unknown. Callers must not treat the value as a supported code
 * page without checking it: the "-iso" option parsing in common.c maps
 * every value below 2 to the default CONVMODE_437, and any other number
 * that has no conversion table is later rejected with "code page %d is
 * not supported".
 *
 * The return type stays 'unsigned short' on purpose. It matches the 16 bit
 * register and IOCTL parameter block words the DOS implementations read
 * the value from, and every code page number fits in 16 bits. Callers
 * widen it to the 'int' ConvMode field.
 *
 * [[nodiscard]] (ISO C23): the function has no side effects, its only
 * purpose is the returned code page. A call that ignores the result is
 * always a programming error, so the compiler is asked to diagnose it.
 * The attribute on this declaration also applies to every platform
 * specific definition in querycp.c, which includes this header.
 */
[[nodiscard]] unsigned short query_con_codepage(void);

#endif /* D2U_QUERYCP_H */
