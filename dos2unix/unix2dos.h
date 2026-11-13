/*
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
 */

/* Include guard. It was named __UNIX2DOS_H before. Identifiers that begin with
 * two underscores are reserved for the implementation in ISO C, so the
 * guard now has the D2U_ prefix, like D2U_QUERYCP_H in querycp.h. */
#ifndef D2U_UNIX2DOS_H
#define D2U_UNIX2DOS_H

#include "common.h"
/* <limits.h>: UCHAR_MAX gives the number of entries of a conversion table. */
#include <limits.h>

/* Conversion tables.
 *
 * Why the tables are in this header, behind D2U_UNIX2DOS_C:
 * The tables are 'static' data: every source file that compiles them gets
 * its own copy. Only unix2dos.c converts characters, and only unix2dos.c
 * defines D2U_UNIX2DOS_C before it includes this header. Every other file that
 * includes this header gets only the function prototypes at the end, and
 * no unused copy of the tables. Keeping the tables in the header keeps
 * eight tables of 256 entries each out of the conversion code in
 * unix2dos.c.
 * (The macro was named __UNIX2DOS_C before, it was renamed for the same reason
 * as the include guard.)
 *
 * Table invariants:
 * - Every table has exactly U2D_CONV_TABLE_SIZE (256) entries, one for
 *   each value of an unsigned char. A table is indexed with a character
 *   that d2u_getc() returned. That is 0..255 or EOF, and unix2dos.c handles
 *   EOF before it uses a character as an index.
 * - The entries are character constants. A character constant has type
 *   int, and '\x80'..'\xff' are negative on platforms where char is
 *   signed. That is harmless: the entries are only passed to fputc(),
 *   which converts its argument to unsigned char.
 *
 * ISO C23 safeguards:
 * - 'constexpr' makes the tables read-only compile time constants. The
 *   conversion functions take a pointer to const int, so they cannot
 *   change a table by accident.
 * - The tables are declared without a size, and a static_assert after
 *   each table checks the number of initializers. With a fixed size of
 *   256 a table with too few initializers would still compile; the
 *   missing entries would be zero and convert characters to NUL. */
#ifdef D2U_UNIX2DOS_C

/* Number of entries of every conversion table: one per unsigned char value. */
constexpr int U2D_CONV_TABLE_SIZE = UCHAR_MAX + 1;

/* ASCII mode. No conversion. */

static constexpr int U2DAsciiTable[] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b',
    '\x0c', '\x0d', '\x0e', '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
    '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22', '\x23',
    '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
    '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b',
    '\x3c', '\x3d', '\x3e', '\x3f', '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
    '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f', '\x50', '\x51', '\x52', '\x53',
    '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
    '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b',
    '\x6c', '\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
    '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f', '\x80', '\x81', '\x82', '\x83',
    '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
    '\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99', '\x9a', '\x9b',
    '\x9c', '\x9d', '\x9e', '\x9f', '\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',
    '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf', '\xb0', '\xb1', '\xb2', '\xb3',
    '\xb4', '\xb5', '\xb6', '\xb7', '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',
    '\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7', '\xc8', '\xc9', '\xca', '\xcb',
    '\xcc', '\xcd', '\xce', '\xcf', '\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7',
    '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf', '\xe0', '\xe1', '\xe2', '\xe3',
    '\xe4', '\xe5', '\xe6', '\xe7', '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',
    '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7', '\xf8', '\xf9', '\xfa', '\xfb',
    '\xfc', '\xfd', '\xfe', '\xff',
};
static_assert(sizeof(U2DAsciiTable) / sizeof(U2DAsciiTable[0]) == (size_t)U2D_CONV_TABLE_SIZE,
              "U2DAsciiTable must have one entry for every byte value");

/* 7bit mode. */
/* All 8 bit non-ASCII characters are converted to a space (\x20) */

static constexpr int U2D7BitTable[] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b',
    '\x0c', '\x0d', '\x0e', '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
    '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22', '\x23',
    '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
    '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b',
    '\x3c', '\x3d', '\x3e', '\x3f', '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
    '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f', '\x50', '\x51', '\x52', '\x53',
    '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
    '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b',
    '\x6c', '\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
    '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f', '\x20', '\x20', '\x20', '\x20',
    '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',
    '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',
    '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',
    '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',
    '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',
    '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',
    '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',
    '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',
    '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',
    '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',
    '\x20', '\x20', '\x20', '\x20',
};
static_assert(sizeof(U2D7BitTable) / sizeof(U2D7BitTable[0]) == (size_t)U2D_CONV_TABLE_SIZE,
              "U2D7BitTable must have one entry for every byte value");

/* ISO mode. ISO-8859-1 -> CP437 */
/* Non-convertible characters are converted to a dot (\x2e) */

static constexpr int U2DIso437Table[] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b',
    '\x0c', '\x0d', '\x0e', '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
    '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22', '\x23',
    '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
    '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b',
    '\x3c', '\x3d', '\x3e', '\x3f', '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
    '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f', '\x50', '\x51', '\x52', '\x53',
    '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
    '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b',
    '\x6c', '\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
    '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\xff', '\xad', '\x9b', '\x9c', '\x2e', '\x9d', '\x2e', '\x2e',
    '\x2e', '\x2e', '\xa6', '\xae', '\xaa', '\x2e', '\x2e', '\x2e', '\xf8', '\xf1', '\xfd', '\x2e',
    '\x2e', '\xe6', '\x2e', '\xfa', '\x2e', '\x2e', '\xa7', '\xaf', '\xac', '\xab', '\x2e', '\xa8',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x8e', '\x8f', '\x92', '\x80', '\x2e', '\x90', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\xa5', '\x2e', '\x2e', '\x2e', '\x2e', '\x99', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x9a', '\x2e', '\x2e', '\xe1', '\x85', '\xa0', '\x83', '\x2e',
    '\x84', '\x86', '\x91', '\x87', '\x8a', '\x82', '\x88', '\x89', '\x8d', '\xa1', '\x8c', '\x8b',
    '\x2e', '\xa4', '\x95', '\xa2', '\x93', '\x2e', '\x94', '\xf6', '\x2e', '\x97', '\xa3', '\x96',
    '\x81', '\x2e', '\x2e', '\x98',
};
static_assert(sizeof(U2DIso437Table) / sizeof(U2DIso437Table[0]) == (size_t)U2D_CONV_TABLE_SIZE,
              "U2DIso437Table must have one entry for every byte value");

/* ISO mode. ISO-8859-1 -> CP850 */
/* Non-convertible characters are converted to a dot (\x2e) */

static constexpr int U2DIso850Table[] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b',
    '\x0c', '\x0d', '\x0e', '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
    '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22', '\x23',
    '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
    '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b',
    '\x3c', '\x3d', '\x3e', '\x3f', '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
    '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f', '\x50', '\x51', '\x52', '\x53',
    '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
    '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b',
    '\x6c', '\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
    '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\xff', '\xad', '\xbd', '\x9c', '\xcf', '\xbe', '\xdd', '\xf5',
    '\xf9', '\xb8', '\xa6', '\xae', '\xaa', '\xf0', '\xa9', '\xee', '\xf8', '\xf1', '\xfd', '\xfc',
    '\xef', '\xe6', '\xf4', '\xfa', '\xf7', '\xfb', '\xa7', '\xaf', '\xac', '\xab', '\xf3', '\xa8',
    '\xb7', '\xb5', '\xb6', '\xc7', '\x8e', '\x8f', '\x92', '\x80', '\xd4', '\x90', '\xd2', '\xd3',
    '\xde', '\xd6', '\xd7', '\xd8', '\xd1', '\xa5', '\xe3', '\xe0', '\xe2', '\xe5', '\x99', '\x9e',
    '\x9d', '\xeb', '\xe9', '\xea', '\x9a', '\xed', '\xe8', '\xe1', '\x85', '\xa0', '\x83', '\xc6',
    '\x84', '\x86', '\x91', '\x87', '\x8a', '\x82', '\x88', '\x89', '\x8d', '\xa1', '\x8c', '\x8b',
    '\xd0', '\xa4', '\x95', '\xa2', '\x93', '\xe4', '\x94', '\xf6', '\x9b', '\x97', '\xa3', '\x96',
    '\x81', '\xec', '\xe7', '\x98',
};
static_assert(sizeof(U2DIso850Table) / sizeof(U2DIso850Table[0]) == (size_t)U2D_CONV_TABLE_SIZE,
              "U2DIso850Table must have one entry for every byte value");

/* ISO mode. ISO-8859-1 -> CP860 */
/* Non-convertible characters are converted to a dot (\x2e) */

static constexpr int U2DIso860Table[] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b',
    '\x0c', '\x0d', '\x0e', '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
    '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22', '\x23',
    '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
    '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b',
    '\x3c', '\x3d', '\x3e', '\x3f', '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
    '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f', '\x50', '\x51', '\x52', '\x53',
    '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
    '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b',
    '\x6c', '\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
    '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\xff', '\xad', '\x9b', '\x9c', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\xa6', '\xae', '\xaa', '\x2e', '\x2e', '\x2e', '\xf8', '\xf1', '\xfd', '\x2e',
    '\x2e', '\xe6', '\x2e', '\xfa', '\x2e', '\x2e', '\xa7', '\xaf', '\xac', '\xab', '\x2e', '\xa8',
    '\x91', '\x86', '\x8f', '\x8e', '\x2e', '\x2e', '\x2e', '\x80', '\x92', '\x90', '\x89', '\x2e',
    '\x98', '\x8b', '\x2e', '\x2e', '\x2e', '\xa5', '\xa9', '\x9f', '\x8c', '\x99', '\x2e', '\x2e',
    '\x2e', '\x9d', '\x96', '\x2e', '\x9a', '\x2e', '\x2e', '\xe1', '\x85', '\xa0', '\x83', '\x84',
    '\x2e', '\x2e', '\x2e', '\x87', '\x8a', '\x82', '\x88', '\x2e', '\x8d', '\xa1', '\x2e', '\x2e',
    '\x2e', '\xa4', '\x95', '\xa2', '\x93', '\x94', '\x2e', '\xf6', '\x2e', '\x97', '\xa3', '\x2e',
    '\x81', '\x2e', '\x2e', '\x2e',
};
static_assert(sizeof(U2DIso860Table) / sizeof(U2DIso860Table[0]) == (size_t)U2D_CONV_TABLE_SIZE,
              "U2DIso860Table must have one entry for every byte value");

/* ISO mode. ISO-8859-1 -> CP863 */
/* Non-convertible characters are converted to a dot (\x2e) */

static constexpr int U2DIso863Table[] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b',
    '\x0c', '\x0d', '\x0e', '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
    '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22', '\x23',
    '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
    '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b',
    '\x3c', '\x3d', '\x3e', '\x3f', '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
    '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f', '\x50', '\x51', '\x52', '\x53',
    '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
    '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b',
    '\x6c', '\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
    '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\xff', '\xad', '\x9b', '\x9c', '\x98', '\x9d', '\xa0', '\x8f',
    '\xa4', '\x2e', '\xa6', '\xae', '\xaa', '\x2e', '\x2e', '\xa7', '\xf8', '\xf1', '\xfd', '\xa6',
    '\xa1', '\xe6', '\x86', '\xfa', '\xa5', '\x2e', '\xa7', '\xaf', '\xac', '\xab', '\xad', '\xa8',
    '\x8e', '\x2e', '\x84', '\x2e', '\x8e', '\x8f', '\x92', '\x80', '\x91', '\x90', '\x92', '\x94',
    '\x2e', '\x2e', '\xa8', '\x95', '\x2e', '\xa5', '\x2e', '\x2e', '\x99', '\x2e', '\x99', '\x2e',
    '\x2e', '\x9d', '\x2e', '\x9e', '\x9a', '\x2e', '\x2e', '\xe1', '\x85', '\xa0', '\x83', '\x2e',
    '\x84', '\x86', '\x91', '\x87', '\x8a', '\x82', '\x88', '\x89', '\x8d', '\xa1', '\x8c', '\x8b',
    '\x2e', '\xa4', '\x95', '\xa2', '\x93', '\x2e', '\x94', '\xf6', '\x2e', '\x97', '\xa3', '\x96',
    '\x81', '\x2e', '\x2e', '\x98',
};
static_assert(sizeof(U2DIso863Table) / sizeof(U2DIso863Table[0]) == (size_t)U2D_CONV_TABLE_SIZE,
              "U2DIso863Table must have one entry for every byte value");

/* ISO mode. ISO-8859-1 -> CP865 */
/* Non-convertible characters are converted to a dot (\x2e) */

static constexpr int U2DIso865Table[] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b',
    '\x0c', '\x0d', '\x0e', '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
    '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22', '\x23',
    '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
    '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b',
    '\x3c', '\x3d', '\x3e', '\x3f', '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
    '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f', '\x50', '\x51', '\x52', '\x53',
    '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
    '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b',
    '\x6c', '\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
    '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\xff', '\xad', '\x2e', '\x9c', '\xaf', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\xa6', '\xae', '\xaa', '\x2e', '\x2e', '\x2e', '\xf8', '\xf1', '\xfd', '\x2e',
    '\x2e', '\xe6', '\x2e', '\xfa', '\x2e', '\x2e', '\xa7', '\x2e', '\xac', '\xab', '\x2e', '\xa8',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x8e', '\x8f', '\x92', '\x80', '\x2e', '\x90', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\xa5', '\x2e', '\x2e', '\x2e', '\x2e', '\x99', '\x2e',
    '\x9d', '\x2e', '\x2e', '\x2e', '\x9a', '\x2e', '\x2e', '\xe1', '\x85', '\xa0', '\x83', '\x2e',
    '\x84', '\x86', '\x91', '\x87', '\x8a', '\x82', '\x88', '\x89', '\x8d', '\xa1', '\x8c', '\x8b',
    '\x2e', '\xa4', '\x95', '\xa2', '\x93', '\x2e', '\x94', '\xf6', '\x9b', '\x97', '\xa3', '\x96',
    '\x81', '\x2e', '\x2e', '\x98',
};
static_assert(sizeof(U2DIso865Table) / sizeof(U2DIso865Table[0]) == (size_t)U2D_CONV_TABLE_SIZE,
              "U2DIso865Table must have one entry for every byte value");

/* ISO-8859-1 -> CP1252 */
/* Non-convertible characters are converted to a dot (\x2e) */

static constexpr int U2DIso1252Table[] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b',
    '\x0c', '\x0d', '\x0e', '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
    '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22', '\x23',
    '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
    '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b',
    '\x3c', '\x3d', '\x3e', '\x3f', '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
    '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f', '\x50', '\x51', '\x52', '\x53',
    '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
    '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b',
    '\x6c', '\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
    '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e', '\x2e',
    '\x2e', '\x2e', '\x2e', '\x2e', '\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',
    '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf', '\xb0', '\xb1', '\xb2', '\xb3',
    '\xb4', '\xb5', '\xb6', '\xb7', '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',
    '\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7', '\xc8', '\xc9', '\xca', '\xcb',
    '\xcc', '\xcd', '\xce', '\xcf', '\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7',
    '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf', '\xe0', '\xe1', '\xe2', '\xe3',
    '\xe4', '\xe5', '\xe6', '\xe7', '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',
    '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7', '\xf8', '\xf9', '\xfa', '\xfb',
    '\xfc', '\xfd', '\xfe', '\xff',
};
static_assert(sizeof(U2DIso1252Table) / sizeof(U2DIso1252Table[0]) == (size_t)U2D_CONV_TABLE_SIZE,
              "U2DIso1252Table must have one entry for every byte value");
#endif

/* Convert the text of stream ipInF to DOS line breaks and write it to stream
 * ipOutF. ipFlag->FromToMode selects unix2dos (LF -> CR LF) or
 * unix2mac (LF -> CR) conversion.
 * ConvertUnixToDos() converts byte oriented text (ASCII, 8 bit code pages,
 * UTF-8, GB18030). ConvertUnixToDosW() converts UTF-16 text and exists only
 * with D2U_UNICODE. main() passes both to parse_options(), which calls them
 * through function pointers for every file.
 * Both return 0 on success and -1 on error; ipFlag->status and
 * ipFlag->error tell what went wrong.
 * [[nodiscard]] (ISO C23): the return value is the only sign that the output
 * is incomplete, so a direct call must not ignore it. */
#ifdef D2U_UNICODE
[[nodiscard]] int ConvertUnixToDosW(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag,
                                    const char *progname);
#endif
[[nodiscard]] int ConvertUnixToDos(BufferedStream *ipInF, FILE *ipOutF, CFlag *ipFlag,
                                   const char *progname);

#endif
