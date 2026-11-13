#!/usr/bin/env zsh
################################################################################
#                                                                              #
#  PROGRAM:      verify.sh                                                     #
#  DESCRIPTION:  Comprehensive regression test harness for C23 dos2unix        #
#                modernization. Compares output of the refactored C23 binary   #
#                against the untouched upstream baseline across five core      #
#                operational modes, validates querycp.c -DTEST under C23, and  #
#                runs the full 174-test upstream Perl suite.                   #
#                Portable and self-contained: every path is resolved relative  #
#                to the location of this script, and when the host has no      #
#                pre-built baseline binary, one is built automatically from    #
#                the master branch (see BASELINE below).                       #
#                                                                              #
#  AUTHOR:       BazTek                                                        #
#  SYSTEM:       CachyOS Linux / zsh                                           #
#  CREATED:      2026-11-12                                                    #
#  REVISION:     3.0                                                           #
#                                                                              #
#  USAGE:        ./c23-verification/verify.sh                                  #
#                  Works from any current directory. Takes no arguments.       #
#                  Build dos2unix/dos2unix first (the script prints the make   #
#                  command when the binary is missing).                        #
#                bash c23-verification/verify.sh                               #
#                  The same, on a host without zsh. The script only uses       #
#                  syntax that zsh and bash share.                             #
#                                                                              #
#  TEST MATRIX:  [1] Standard CRLF -> LF in-place file conversion              #
#                [2] Binary file detection & skip guard (embedded NUL bytes)   #
#                [3] Classic Macintosh CR -> LF mode (-c mac)                  #
#                [4] UTF-16LE with Byte-Order Mark (BOM) -> UTF-8 conversion   #
#                [5] Filter / pipe mode (stdin -> stdout)                      #
#                [6] Standalone querycp.c -DTEST C23 build & execution         #
#                [7] Full upstream Perl test suite (174 tests across 9 files)  #
#                                                                              #
#  PATHS:        Derived from the directory that holds this script:            #
#                  <repo>/c23-verification/            this script             #
#                  <repo>/c23-verification/test-data/  input fixtures          #
#                  <repo>/dos2unix/                    C sources and Makefile  #
#                  <repo>/dos2unix/dos2unix            newly built C23 binary  #
#                Nothing depends on $HOME/Projects/c23-sandbox any more.       #
#                                                                              #
#  BASELINE:     The baseline dos2unix binary is the first of:                 #
#                  1. $D2U_BASELINE, when set: the path of a pre-built         #
#                     upstream dos2unix binary.                                #
#                  2. $HOME/Projects/c23-sandbox/baselines/dos2unix.orig,      #
#                     when present: the author's local copy.                   #
#                  3. Automatic fallback: the upstream sources are exported    #
#                     with "git archive master dos2unix | tar -x" into a       #
#                     temporary directory, and dos2unix and unix2dos are       #
#                     built there with make.                                   #
#                Environment variables for the fallback:                       #
#                  D2U_BASELINE_REF          git ref to build instead of       #
#                                            master. Without it, master is     #
#                                            used, or origin/master when the   #
#                                            clone has no local master.        #
#                  D2U_FORCE_BASELINE_BUILD  set to 1 to skip 1. and 2. and    #
#                                            always build from git.            #
#                  CC                        compiler for the baseline build   #
#                                            (default: clang).                 #
#                The fallback needs a git checkout. In a source tree without   #
#                git history (a downloaded archive), set D2U_BASELINE.         #
#                                                                              #
#  NOTES:        - Exits immediately on any non-zero return status (set -e).   #
#                - Scratch files are isolated in a temporary directory made    #
#                  by mktemp (it honours $TMPDIR). An EXIT trap removes that   #
#                  directory, the temporary baseline build included.           #
#                - The same trap removes what test 7 leaves in the source      #
#                  tree (the output files of the upstream suite, and the       #
#                  mac2unix / unix2mac links when this script created them),   #
#                  so a run leaves no new files in the repository.             #
#                - Needs clang, make, perl and the usual POSIX tools. The      #
#                  baseline fallback also needs git and tar.                   #
#                                                                              #
################################################################################

#------------------------------------------------------------------------------#
#  SHELL OPTIONS                                                               #
#------------------------------------------------------------------------------#
set -e                          # Abort script immediately if any test fails
set -o pipefail                 # ... also when the first command of a pipe fails

#------------------------------------------------------------------------------#
#  ENVIRONMENT & PATH DEFINITIONS                                              #
#------------------------------------------------------------------------------#
# All paths start at the directory of this script, so the suite runs from any
# clone and from any current directory. "pwd -P" gives the physical path.
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)"
REPO_ROOT="$(dirname -- "$SCRIPT_DIR")"             # Root of the git repository
REPO_SRC="$REPO_ROOT/dos2unix"                      # C sources and Makefile
MODERN="$REPO_SRC/dos2unix"                         # Newly built C23 binary
DATA_DIR="$SCRIPT_DIR/test-data"                    # Master fixture directory

# Optional pre-built baseline on the author's machine, see BASELINE above.
LOCAL_BASELINE="${HOME:-}/Projects/c23-sandbox/baselines/dos2unix.orig"
BASELINE_REF="${D2U_BASELINE_REF:-}"                # Empty: master, origin/master
ORIG=""                                             # Untouched baseline binary
ORIG_FROM=""                                        # Where ORIG came from

FIX_CRLF="$DATA_DIR/sample_crlf.txt"
FIX_BIN="$DATA_DIR/sample_binary.bin"
FIX_MAC="$DATA_DIR/sample_mac.txt"
FIX_UTF16="$DATA_DIR/sample_utf16le.txt"

#------------------------------------------------------------------------------#
#  SCRATCH DIRECTORY & EXIT TRAP                                               #
#------------------------------------------------------------------------------#
SCRATCH=""                      # Temporary directory, created below
SUITE_STARTED=0                 # Set to 1 when test 7 has touched the source tree
MADE_MAC2UNIX=0                 # Set to 1 when test 7 created dos2unix/mac2unix
MADE_UNIX2MAC=0                 # Set to 1 when test 7 created dos2unix/unix2mac

# Runs on every exit, also after a failed test or Ctrl-C. It removes the
# scratch directory (with the temporary baseline build inside it) and the
# files that test 7 leaves in the source tree. The exit status of the script
# is not changed.
cleanup() {
  if [[ -n "$SCRATCH" && -d "$SCRATCH" ]]; then
    rm -rf "$SCRATCH"
  fi
  if [[ "$SUITE_STARTED" == 1 ]]; then
    make -s -C "$REPO_SRC/test" clean > /dev/null 2>&1 || true
    rm -f "$REPO_SRC/test/err.txt"
    if [[ "$MADE_MAC2UNIX" == 1 ]]; then
      rm -f "$REPO_SRC/mac2unix"
    fi
    if [[ "$MADE_UNIX2MAC" == 1 ]]; then
      rm -f "$REPO_SRC/unix2mac"
    fi
  fi
}
trap cleanup EXIT
trap 'exit 130' INT             # Ctrl-C: leave through the EXIT trap
trap 'exit 143' TERM

SCRATCH="$(mktemp -d "${TMPDIR:-/tmp}/d2u_verify.XXXXXX")"

#------------------------------------------------------------------------------#
#  PRE-FLIGHT SANITY CHECKS                                                    #
#------------------------------------------------------------------------------#
for tool in clang make; do
  if ! command -v "$tool" > /dev/null 2>&1; then
    echo "ERROR: Required tool not found in PATH: $tool"
    exit 1
  fi
done

if [[ ! -x "$MODERN" ]]; then
  echo "ERROR: Modern binary not found at $MODERN. Build the project first:"
  echo "  make -C \"$REPO_SRC\" CC=clang CFLAGS=\"-std=c23 -O2 -Wall -Wextra -Wconversion -Wpedantic -g\" dos2unix unix2dos"
  exit 1
fi

for fixture in "$FIX_CRLF" "$FIX_BIN" "$FIX_MAC" "$FIX_UTF16"; do
  if [[ ! -f "$fixture" ]]; then
    echo "ERROR: Missing test fixture: $fixture"
    exit 1
  fi
done

#------------------------------------------------------------------------------#
#  BASELINE BINARY: PRE-BUILT, OR BUILT FROM THE MASTER BRANCH                 #
#------------------------------------------------------------------------------#
# Fallback for hosts without a pre-built baseline: export the upstream sources
# of the baseline ref from git into the scratch directory and build dos2unix
# and unix2dos there. The EXIT trap removes the build with the scratch
# directory. The modernization branch starts at master, so master is the
# untouched upstream code.
build_baseline_from_git() {
  for tool in git tar; do
    if ! command -v "$tool" > /dev/null 2>&1; then
      echo "ERROR: Building the baseline needs '$tool', which is not in PATH."
      exit 1
    fi
  done
  if ! git -C "$REPO_ROOT" rev-parse --git-dir > /dev/null 2>&1; then
    echo "ERROR: $REPO_ROOT is not a git checkout, so the baseline cannot be"
    echo "       built from the master branch. Set D2U_BASELINE to the path of"
    echo "       a pre-built upstream dos2unix binary."
    exit 1
  fi
  if [[ -z "$BASELINE_REF" ]]; then
    # A clone whose default branch is not master has only origin/master.
    for ref in master origin/master; do
      if git -C "$REPO_ROOT" rev-parse --verify --quiet "${ref}^{commit}" > /dev/null; then
        BASELINE_REF="$ref"
        break
      fi
    done
  fi
  if [[ -z "$BASELINE_REF" ]] ||
     ! git -C "$REPO_ROOT" rev-parse --verify --quiet "${BASELINE_REF}^{commit}" > /dev/null; then
    echo "ERROR: Baseline git ref not found: ${BASELINE_REF:-master, origin/master}"
    echo "       Set D2U_BASELINE_REF to the upstream ref, or D2U_BASELINE to a"
    echo "       pre-built upstream dos2unix binary."
    exit 1
  fi

  echo "  Building the baseline from git ref '$BASELINE_REF' (temporary)..."
  mkdir -p "$SCRATCH/baseline"
  git -C "$REPO_ROOT" archive "$BASELINE_REF" dos2unix | tar -x -C "$SCRATCH/baseline"
  if ! make -s -C "$SCRATCH/baseline/dos2unix" CC="${CC:-clang}" dos2unix unix2dos \
       > "$SCRATCH/baseline_build.log" 2>&1; then
    cat "$SCRATCH/baseline_build.log"
    echo "ERROR: Building the baseline from git ref '$BASELINE_REF' failed."
    exit 1
  fi
  ORIG="$SCRATCH/baseline/dos2unix/dos2unix"
  ORIG_FROM="built from git ref '$BASELINE_REF' in a temporary directory"
}

echo "========================================================================"
echo "  RUNNING C23 DOS2UNIX REGRESSION SUITE"
echo "========================================================================"

if [[ "${D2U_FORCE_BASELINE_BUILD:-0}" == 1 ]]; then
  build_baseline_from_git
elif [[ -n "${D2U_BASELINE:-}" ]]; then
  if [[ ! -x "$D2U_BASELINE" ]]; then
    echo "ERROR: Baseline binary not found or not executable: $D2U_BASELINE"
    exit 1
  fi
  ORIG="$D2U_BASELINE"
  ORIG_FROM="pre-built, from D2U_BASELINE"
elif [[ -x "$LOCAL_BASELINE" ]]; then
  ORIG="$LOCAL_BASELINE"
  ORIG_FROM="pre-built local copy"
else
  build_baseline_from_git
fi

if [[ ! -x "$ORIG" ]]; then
  echo "ERROR: Baseline binary not found or not executable: $ORIG"
  exit 1
fi
echo "  Modern binary:   $MODERN"
echo "  Baseline binary: $ORIG"
echo "                   ($ORIG_FROM)"

#------------------------------------------------------------------------------#
#  TEST 1: STANDARD CRLF IN-PLACE CONVERSION                                   #
#------------------------------------------------------------------------------#
echo "[TEST 1/7] Standard CRLF -> LF in-place conversion..."
cp "$FIX_CRLF" "$SCRATCH/t1_orig.txt"
cp "$FIX_CRLF" "$SCRATCH/t1_mod.txt"

"$ORIG" -q "$SCRATCH/t1_orig.txt"
"$MODERN" -q "$SCRATCH/t1_mod.txt"

diff -u "$SCRATCH/t1_orig.txt" "$SCRATCH/t1_mod.txt"
rm -f "$SCRATCH/t1_orig.txt" "$SCRATCH/t1_mod.txt"
echo "  -> PASS"

#------------------------------------------------------------------------------#
#  TEST 2: BINARY FILE DETECTION & SKIP GUARD                                  #
#------------------------------------------------------------------------------#
echo "[TEST 2/7] Binary file guard (must skip files with NUL bytes)..."
cp "$FIX_BIN" "$SCRATCH/t2_orig.bin"
cp "$FIX_BIN" "$SCRATCH/t2_mod.bin"

"$ORIG" -q "$SCRATCH/t2_orig.bin"
"$MODERN" -q "$SCRATCH/t2_mod.bin"

cmp "$SCRATCH/t2_orig.bin" "$SCRATCH/t2_mod.bin"
cmp "$FIX_BIN" "$SCRATCH/t2_mod.bin"
rm -f "$SCRATCH/t2_orig.bin" "$SCRATCH/t2_mod.bin"
echo "  -> PASS"

#------------------------------------------------------------------------------#
#  TEST 3: CLASSIC MACINTOSH CR MODE (-c mac)                                  #
#------------------------------------------------------------------------------#
echo "[TEST 3/7] Classic Mac CR -> LF mode (-c mac)..."
cp "$FIX_MAC" "$SCRATCH/t3_orig.txt"
cp "$FIX_MAC" "$SCRATCH/t3_mod.txt"

"$ORIG" -q -c mac "$SCRATCH/t3_orig.txt"
"$MODERN" -q -c mac "$SCRATCH/t3_mod.txt"

diff -u "$SCRATCH/t3_orig.txt" "$SCRATCH/t3_mod.txt"
rm -f "$SCRATCH/t3_orig.txt" "$SCRATCH/t3_mod.txt"
echo "  -> PASS"

#------------------------------------------------------------------------------#
#  TEST 4: UTF-16LE WITH BOM -> UTF-8 CONVERSION                               #
#------------------------------------------------------------------------------#
echo "[TEST 4/7] UTF-16LE with BOM conversion..."
cp "$FIX_UTF16" "$SCRATCH/t4_orig.txt"
cp "$FIX_UTF16" "$SCRATCH/t4_mod.txt"

"$ORIG" -q "$SCRATCH/t4_orig.txt"
"$MODERN" -q "$SCRATCH/t4_mod.txt"

cmp "$SCRATCH/t4_orig.txt" "$SCRATCH/t4_mod.txt"
rm -f "$SCRATCH/t4_orig.txt" "$SCRATCH/t4_mod.txt"
echo "  -> PASS"

#------------------------------------------------------------------------------#
#  TEST 5: STDIN / STDOUT PIPE FILTER MODE                                     #
#------------------------------------------------------------------------------#
echo "[TEST 5/7] Standard input/output pipe mode..."
"$ORIG" < "$FIX_CRLF" > "$SCRATCH/t5_orig.txt"
"$MODERN" < "$FIX_CRLF" > "$SCRATCH/t5_mod.txt"

diff -u "$SCRATCH/t5_orig.txt" "$SCRATCH/t5_mod.txt"
rm -f "$SCRATCH/t5_orig.txt" "$SCRATCH/t5_mod.txt"
echo "  -> PASS"

#------------------------------------------------------------------------------#
#  TEST 6: STANDALONE QUERYCP.C -DTEST C23 CHECK                               #
#------------------------------------------------------------------------------#
echo "[TEST 6/7] Standalone querycp.c -DTEST C23 build & execution..."
clang -std=c23 -Wall -Wextra -Wpedantic -Wconversion -DTEST \
  "$REPO_SRC/querycp.c" -o "$SCRATCH/t6_dtest"
DTEST_OUT="$("$SCRATCH/t6_dtest" | tr -d '[:space:]')"
rm -f "$SCRATCH/t6_dtest"

if [[ "$DTEST_OUT" != "CP0" ]]; then
  echo "ERROR: Unexpected -DTEST output: '$DTEST_OUT' (expected 'CP0')"
  exit 1
fi
echo "  -> PASS ($DTEST_OUT)"

#------------------------------------------------------------------------------#
#  TEST 7: FULL UPSTREAM PERL SUITE (174 TESTS)                                #
#------------------------------------------------------------------------------#
echo "[TEST 7/7] Upstream Perl test suite (174 tests)..."
# The upstream suite runs inside the source tree. Remember which of the
# mac2unix / unix2mac links do not exist yet, so that the EXIT trap removes
# only what this run creates.
if [[ ! -e "$REPO_SRC/mac2unix" && ! -L "$REPO_SRC/mac2unix" ]]; then
  MADE_MAC2UNIX=1
fi
if [[ ! -e "$REPO_SRC/unix2mac" && ! -L "$REPO_SRC/unix2mac" ]]; then
  MADE_UNIX2MAC=1
fi
SUITE_STARTED=1
make -s -C "$REPO_SRC" dos2unix unix2dos mac2unix unix2mac \
  CC=clang CFLAGS_USER="-std=c23 -Wall -Wextra -Wpedantic -Wconversion"
make -s -C "$REPO_SRC/test" check > "$SCRATCH/t7_prove.log" 2>&1 || {
  cat "$SCRATCH/t7_prove.log"
  exit 1
}
echo "  -> PASS (174/174 tests)"

#------------------------------------------------------------------------------#
#  SUMMARY                                                                     #
#------------------------------------------------------------------------------#
echo "========================================================================"
echo "  ALL 7 REGRESSION TESTS PASSED (BYTE-FOR-BYTE PARITY & 174 SUITE OK)"
echo "========================================================================"
exit 0
