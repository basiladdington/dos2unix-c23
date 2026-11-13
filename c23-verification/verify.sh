#!/usr/bin/env zsh
################################################################################
#                                                                              #
#  PROGRAM:      verify.sh                                                     #
#  DESCRIPTION:  Comprehensive regression test harness for C23 dos2unix        #
#                modernization. Compares output of the refactored C23 binary   #
#                against the untouched upstream baseline across five core      #
#                operational modes, validates querycp.c -DTEST under C23, and  #
#                runs the full 174-test upstream Perl suite.                   #
#                                                                              #
#  AUTHOR:       BazTek                                                        #
#  SYSTEM:       CachyOS Linux / zsh                                           #
#  CREATED:      2026-11-12                                                    #
#  REVISION:     2.1                                                           #
#                                                                              #
#  TEST MATRIX:  [1] Standard CRLF -> LF in-place file conversion              #
#                [2] Binary file detection & skip guard (embedded NUL bytes)   #
#                [3] Classic Macintosh CR -> LF mode (-c mac)                  #
#                [4] UTF-16LE with Byte-Order Mark (BOM) -> UTF-8 conversion   #
#                [5] Filter / pipe mode (stdin -> stdout)                      #
#                [6] Standalone querycp.c -DTEST C23 build & execution         #
#                [7] Full upstream Perl test suite (174 tests across 9 files)  #
#                                                                              #
#  NOTES:        - Exits immediately on any non-zero return status (set -e).   #
#                - Scratch files are isolated in /tmp and purged after each    #
#                  test case completes.                                        #
#                                                                              #
################################################################################

#------------------------------------------------------------------------------#
#  SHELL OPTIONS                                                               #
#------------------------------------------------------------------------------#
set -e                          # Abort script immediately if any test fails

#------------------------------------------------------------------------------#
#  ENVIRONMENT & PATH DEFINITIONS                                              #
#------------------------------------------------------------------------------#
SANDBOX="$HOME/Projects/c23-sandbox"
REPO_SRC="$SANDBOX/repos/dos2unix/dos2unix"
ORIG="$SANDBOX/baselines/dos2unix.orig"             # Untouched baseline binary
MODERN="$REPO_SRC/dos2unix"                         # Newly built C23 binary
DATA_DIR="$SANDBOX/test-data"                       # Master fixture directory

FIX_CRLF="$DATA_DIR/sample_crlf.txt"
FIX_BIN="$DATA_DIR/sample_binary.bin"
FIX_MAC="$DATA_DIR/sample_mac.txt"
FIX_UTF16="$DATA_DIR/sample_utf16le.txt"

#------------------------------------------------------------------------------#
#  PRE-FLIGHT SANITY CHECKS                                                    #
#------------------------------------------------------------------------------#
if [[ ! -x "$ORIG" ]]; then
  echo "ERROR: Baseline binary not found or not executable: $ORIG"
  exit 1
fi

if [[ ! -x "$MODERN" ]]; then
  echo "ERROR: Modern binary not found at $MODERN. Build the project first (F8)."
  exit 1
fi

for fixture in "$FIX_CRLF" "$FIX_BIN" "$FIX_MAC" "$FIX_UTF16"; do
  if [[ ! -f "$fixture" ]]; then
    echo "ERROR: Missing test fixture: $fixture"
    exit 1
  fi
done

echo "========================================================================"
echo "  RUNNING C23 DOS2UNIX REGRESSION SUITE"
echo "========================================================================"

#------------------------------------------------------------------------------#
#  TEST 1: STANDARD CRLF IN-PLACE CONVERSION                                   #
#------------------------------------------------------------------------------#
echo "[TEST 1/7] Standard CRLF -> LF in-place conversion..."
cp "$FIX_CRLF" /tmp/d2u_t1_orig.txt
cp "$FIX_CRLF" /tmp/d2u_t1_mod.txt

"$ORIG" -q /tmp/d2u_t1_orig.txt
"$MODERN" -q /tmp/d2u_t1_mod.txt

diff -u /tmp/d2u_t1_orig.txt /tmp/d2u_t1_mod.txt
rm -f /tmp/d2u_t1_orig.txt /tmp/d2u_t1_mod.txt
echo "  -> PASS"

#------------------------------------------------------------------------------#
#  TEST 2: BINARY FILE DETECTION & SKIP GUARD                                  #
#------------------------------------------------------------------------------#
echo "[TEST 2/7] Binary file guard (must skip files with NUL bytes)..."
cp "$FIX_BIN" /tmp/d2u_t2_orig.bin
cp "$FIX_BIN" /tmp/d2u_t2_mod.bin

"$ORIG" -q /tmp/d2u_t2_orig.bin
"$MODERN" -q /tmp/d2u_t2_mod.bin

cmp /tmp/d2u_t2_orig.bin /tmp/d2u_t2_mod.bin
cmp "$FIX_BIN" /tmp/d2u_t2_mod.bin
rm -f /tmp/d2u_t2_orig.bin /tmp/d2u_t2_mod.bin
echo "  -> PASS"

#------------------------------------------------------------------------------#
#  TEST 3: CLASSIC MACINTOSH CR MODE (-c mac)                                  #
#------------------------------------------------------------------------------#
echo "[TEST 3/7] Classic Mac CR -> LF mode (-c mac)..."
cp "$FIX_MAC" /tmp/d2u_t3_orig.txt
cp "$FIX_MAC" /tmp/d2u_t3_mod.txt

"$ORIG" -q -c mac /tmp/d2u_t3_orig.txt
"$MODERN" -q -c mac /tmp/d2u_t3_mod.txt

diff -u /tmp/d2u_t3_orig.txt /tmp/d2u_t3_mod.txt
rm -f /tmp/d2u_t3_orig.txt /tmp/d2u_t3_mod.txt
echo "  -> PASS"

#------------------------------------------------------------------------------#
#  TEST 4: UTF-16LE WITH BOM -> UTF-8 CONVERSION                               #
#------------------------------------------------------------------------------#
echo "[TEST 4/7] UTF-16LE with BOM conversion..."
cp "$FIX_UTF16" /tmp/d2u_t4_orig.txt
cp "$FIX_UTF16" /tmp/d2u_t4_mod.txt

"$ORIG" -q /tmp/d2u_t4_orig.txt
"$MODERN" -q /tmp/d2u_t4_mod.txt

cmp /tmp/d2u_t4_orig.txt /tmp/d2u_t4_mod.txt
rm -f /tmp/d2u_t4_orig.txt /tmp/d2u_t4_mod.txt
echo "  -> PASS"

#------------------------------------------------------------------------------#
#  TEST 5: STDIN / STDOUT PIPE FILTER MODE                                     #
#------------------------------------------------------------------------------#
echo "[TEST 5/7] Standard input/output pipe mode..."
"$ORIG" < "$FIX_CRLF" > /tmp/d2u_t5_orig.txt
"$MODERN" < "$FIX_CRLF" > /tmp/d2u_t5_mod.txt

diff -u /tmp/d2u_t5_orig.txt /tmp/d2u_t5_mod.txt
rm -f /tmp/d2u_t5_orig.txt /tmp/d2u_t5_mod.txt
echo "  -> PASS"

#------------------------------------------------------------------------------#
#  TEST 6: STANDALONE QUERYCP.C -DTEST C23 CHECK                               #
#------------------------------------------------------------------------------#
echo "[TEST 6/7] Standalone querycp.c -DTEST C23 build & execution..."
clang -std=c23 -Wall -Wextra -Wpedantic -Wconversion -DTEST \
  "$REPO_SRC/querycp.c" -o /tmp/d2u_t6_dtest
DTEST_OUT="$(/tmp/d2u_t6_dtest | tr -d '[:space:]')"
rm -f /tmp/d2u_t6_dtest

if [[ "$DTEST_OUT" != "CP0" ]]; then
  echo "ERROR: Unexpected -DTEST output: '$DTEST_OUT' (expected 'CP0')"
  exit 1
fi
echo "  -> PASS ($DTEST_OUT)"

#------------------------------------------------------------------------------#
#  TEST 7: FULL UPSTREAM PERL SUITE (174 TESTS)                                #
#------------------------------------------------------------------------------#
echo "[TEST 7/7] Upstream Perl test suite (174 tests)..."
make -s -C "$REPO_SRC" dos2unix unix2dos mac2unix unix2mac \
  CC=clang CFLAGS_USER="-std=c23 -Wall -Wextra -Wpedantic -Wconversion"
make -s -C "$REPO_SRC/test" check > /tmp/d2u_t7_prove.log 2>&1 || {
  cat /tmp/d2u_t7_prove.log
  rm -f /tmp/d2u_t7_prove.log "$REPO_SRC/test/err.txt"
  exit 1
}
rm -f /tmp/d2u_t7_prove.log "$REPO_SRC/test/err.txt"
echo "  -> PASS (174/174 tests)"

#------------------------------------------------------------------------------#
#  SUMMARY                                                                     #
#------------------------------------------------------------------------------#
echo "========================================================================"
echo "  ALL 7 REGRESSION TESTS PASSED (BYTE-FOR-BYTE PARITY & 174 SUITE OK)"
echo "========================================================================"
exit 0
