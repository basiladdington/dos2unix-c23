#!/usr/bin/env zsh
################################################################################
#                                                                              #
#  PROGRAM:      sweep.sh                                                      #
#  DESCRIPTION:  Code page sweep for the C23 dos2unix modernization. Sends     #
#                all 256 byte values through every conversion table of the     #
#                newly built programs and of the untouched upstream baseline,  #
#                and compares stdout, stderr and exit status.                  #
#                4 programs (dos2unix, mac2unix, unix2dos, unix2mac)           #
#                x 9 modes (-ascii -7 -iso -437 -850 -860 -863 -865 -1252)     #
#                x 4 option sets (none, -l, -e, -l -e) = 144 cases.            #
#                                                                              #
#  USAGE:        ./c23-verification/sweep.sh                                   #
#                  Automatic mode, works from any current directory. Tests     #
#                  <repo>/dos2unix/dos2unix and <repo>/dos2unix/unix2dos       #
#                  (build them first) against the baseline binaries, see       #
#                  BASELINE below.                                             #
#                ./c23-verification/sweep.sh MODERN_DIR BASELINE_DIR [WORKDIR] #
#                  Explicit mode. Each of the two directories must hold        #
#                  dos2unix, mac2unix, unix2dos and unix2mac. WORKDIR is       #
#                  created when needed; only the files all.bin, a.out,         #
#                  a.err, b.out and b.err are written there, and they are      #
#                  removed at the end. Without WORKDIR a temporary directory   #
#                  is used.                                                    #
#                bash c23-verification/sweep.sh ...                            #
#                  The same, on a host without zsh. The script only uses       #
#                  syntax that zsh and bash share.                             #
#                                                                              #
#  EXIT STATUS:  0  all cases identical and no sanitizer report                #
#                1  at least one difference or sanitizer report                #
#                2  usage or setup error                                       #
#                                                                              #
#  PATHS:        Derived from the directory that holds this script:            #
#                  <repo>/c23-verification/   this script                      #
#                  <repo>/dos2unix/           the newly built programs         #
#                                                                              #
#  BASELINE:     Automatic mode looks for each baseline program on its own:    #
#                  1. $D2U_BASELINE (dos2unix) and $U2D_BASELINE (unix2dos),   #
#                     when set: paths of pre-built upstream binaries.          #
#                  2. $HOME/Projects/c23-sandbox/baselines/dos2unix.orig and   #
#                     unix2dos.orig, when present: the author's local copies.  #
#                  3. Automatic fallback for what is still missing: the        #
#                     upstream sources are exported with                       #
#                     "git archive master dos2unix | tar -x" into a temporary  #
#                     directory, and dos2unix and unix2dos are built there     #
#                     with make.                                               #
#                Environment variables for the fallback:                       #
#                  D2U_BASELINE_REF          git ref to build instead of       #
#                                            master (origin/master is tried    #
#                                            when there is no local master).   #
#                  D2U_FORCE_BASELINE_BUILD  set to 1 to always build both     #
#                                            baseline programs from git.       #
#                  CC                        compiler for the baseline build   #
#                                            (default: clang).                 #
#                                                                              #
#  NOTES:        - mac2unix and unix2mac are the same programs started under   #
#                  another name. Automatic mode makes the four names as        #
#                  symbolic links in its temporary directory.                  #
#                - All temporary files, the baseline build included, are in a  #
#                  directory made by mktemp (it honours $TMPDIR). An EXIT      #
#                  trap removes it. Nothing is written into the repository.    #
#                                                                              #
################################################################################

#------------------------------------------------------------------------------#
#  SHELL OPTIONS                                                               #
#------------------------------------------------------------------------------#
# No "set -e": a failing case must be counted, not end the script.
set -o pipefail                 # A failing "git archive | tar" must be noticed

#------------------------------------------------------------------------------#
#  ENVIRONMENT & PATH DEFINITIONS                                              #
#------------------------------------------------------------------------------#
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)"
REPO_ROOT="$(dirname -- "$SCRIPT_DIR")"             # Root of the git repository
REPO_SRC="$REPO_ROOT/dos2unix"                      # Newly built programs
LOCAL_BASELINES="${HOME:-}/Projects/c23-sandbox/baselines"
BASELINE_REF="${D2U_BASELINE_REF:-}"                # Empty: master, origin/master
BUILT_BASELINE=""                                   # Directory of the git build

usage() {
  echo "usage: sweep.sh                                    (automatic mode)"
  echo "       sweep.sh MODERN_DIR BASELINE_DIR [WORKDIR]  (explicit mode)"
  echo "See the header of this script for details."
}

#------------------------------------------------------------------------------#
#  SCRATCH DIRECTORY & EXIT TRAP                                               #
#------------------------------------------------------------------------------#
SCRATCH=""
cleanup() {
  if [[ -n "$SCRATCH" && -d "$SCRATCH" ]]; then
    rm -rf "$SCRATCH"
  fi
}
trap cleanup EXIT
trap 'exit 130' INT             # Ctrl-C: leave through the EXIT trap
trap 'exit 143' TERM

SCRATCH="$(mktemp -d "${TMPDIR:-/tmp}/d2u_sweep.XXXXXX")" || exit 2

#------------------------------------------------------------------------------#
#  BASELINE: PRE-BUILT, OR BUILT FROM THE MASTER BRANCH                        #
#------------------------------------------------------------------------------#
# Export the upstream sources of the baseline ref from git into the scratch
# directory and build dos2unix and unix2dos there. Done at most once.
build_baseline_from_git() {
  if [[ -n "$BUILT_BASELINE" ]]; then
    return 0
  fi
  for tool in git tar make; do
    if ! command -v "$tool" > /dev/null 2>&1; then
      echo "ERROR: Building the baseline needs '$tool', which is not in PATH."
      exit 2
    fi
  done
  if ! git -C "$REPO_ROOT" rev-parse --git-dir > /dev/null 2>&1; then
    echo "ERROR: $REPO_ROOT is not a git checkout, so the baseline cannot be"
    echo "       built from the master branch. Set D2U_BASELINE and U2D_BASELINE"
    echo "       to pre-built upstream binaries."
    exit 2
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
    echo "       Set D2U_BASELINE_REF, or D2U_BASELINE and U2D_BASELINE."
    exit 2
  fi

  echo "Building the baseline from git ref '$BASELINE_REF' (temporary)..."
  mkdir -p "$SCRATCH/baseline_src" || exit 2
  if ! git -C "$REPO_ROOT" archive "$BASELINE_REF" dos2unix | tar -x -C "$SCRATCH/baseline_src"; then
    echo "ERROR: Exporting git ref '$BASELINE_REF' failed."
    exit 2
  fi
  if ! make -s -C "$SCRATCH/baseline_src/dos2unix" CC="${CC:-clang}" dos2unix unix2dos \
       > "$SCRATCH/baseline_build.log" 2>&1; then
    cat "$SCRATCH/baseline_build.log"
    echo "ERROR: Building the baseline from git ref '$BASELINE_REF' failed."
    exit 2
  fi
  BUILT_BASELINE="$SCRATCH/baseline_src/dos2unix"
}

# find_baseline NAME ENV_VALUE: store the path of the baseline binary NAME
# (dos2unix or unix2dos) in the variable FOUND. ENV_VALUE is the value of
# D2U_BASELINE or U2D_BASELINE, or empty.
find_baseline() {
  FOUND=""
  if [[ "${D2U_FORCE_BASELINE_BUILD:-0}" != 1 ]]; then
    if [[ -n "$2" ]]; then
      if [[ ! -x "$2" ]]; then
        echo "ERROR: Baseline binary not found or not executable: $2"
        exit 2
      fi
      FOUND="$2"
      return 0
    fi
    if [[ -x "$LOCAL_BASELINES/$1.orig" ]]; then
      FOUND="$LOCAL_BASELINES/$1.orig"
      return 0
    fi
  fi
  build_baseline_from_git
  FOUND="$BUILT_BASELINE/$1"
}

#------------------------------------------------------------------------------#
#  ARGUMENTS                                                                   #
#------------------------------------------------------------------------------#
case $# in
  0)
    # Automatic mode: the programs of this repository against the baseline.
    for prog in dos2unix unix2dos; do
      if [[ ! -x "$REPO_SRC/$prog" ]]; then
        echo "ERROR: $REPO_SRC/$prog not found. Build the project first:"
        echo "  make -C \"$REPO_SRC\" CC=clang CFLAGS=\"-std=c23 -O2 -Wall -Wextra -Wconversion -Wpedantic -g\" dos2unix unix2dos"
        exit 2
      fi
    done
    find_baseline dos2unix "${D2U_BASELINE:-}"
    BASE_D2U="$FOUND"
    find_baseline unix2dos "${U2D_BASELINE:-}"
    BASE_U2D="$FOUND"

    # The program name selects the conversion: give every program both names.
    P="$SCRATCH/modern"
    H="$SCRATCH/baseline"
    mkdir -p "$P" "$H" || exit 2
    ln -s "$REPO_SRC/dos2unix" "$P/dos2unix"
    ln -s "$REPO_SRC/dos2unix" "$P/mac2unix"
    ln -s "$REPO_SRC/unix2dos" "$P/unix2dos"
    ln -s "$REPO_SRC/unix2dos" "$P/unix2mac"
    ln -s "$BASE_D2U" "$H/dos2unix"
    ln -s "$BASE_D2U" "$H/mac2unix"
    ln -s "$BASE_U2D" "$H/unix2dos"
    ln -s "$BASE_U2D" "$H/unix2mac"
    W="$SCRATCH/work"
    echo "Modern:   $REPO_SRC/dos2unix"
    echo "          $REPO_SRC/unix2dos"
    echo "Baseline: $BASE_D2U"
    echo "          $BASE_U2D"
    ;;
  2|3)
    # Explicit mode: two directories that hold the four programs.
    P="$(CDPATH= cd -- "$1" 2> /dev/null && pwd -P)"
    H="$(CDPATH= cd -- "$2" 2> /dev/null && pwd -P)"
    if [[ -z "$P" || -z "$H" ]]; then
      echo "ERROR: MODERN_DIR and BASELINE_DIR must be existing directories."
      exit 2
    fi
    W="${3:-$SCRATCH/work}"
    ;;
  *)
    usage
    exit 2
    ;;
esac

for prog in dos2unix mac2unix unix2dos unix2mac; do
  for dir in "$P" "$H"; do
    if [[ ! -x "$dir/$prog" ]]; then
      echo "ERROR: $dir/$prog not found or not executable."
      exit 2
    fi
  done
done
mkdir -p "$W" || exit 2

#------------------------------------------------------------------------------#
#  INPUT FILE                                                                  #
#------------------------------------------------------------------------------#
# All 256 byte values, twice, followed by a DOS, a Unix and a Mac line break
# and a last line without line break. Made with printf, so that this script
# needs no other interpreter.
make_input() {
  for pass in 1 2; do
    i=0
    while [ "$i" -lt 256 ]; do
      printf '%b' "\\0$(printf '%o' "$i")"
      i=$((i + 1))
    done
  done
  printf 'tail\r\nline\nmac\rend'
}
make_input > "$W/all.bin"
# 2 x 256 bytes and the 18 bytes of the last printf. A printf that does not
# understand the octal escapes would make a file of another size.
if [[ "$(wc -c < "$W/all.bin" | tr -d '[:space:]')" != 530 ]]; then
  echo "ERROR: could not create the 530 byte input file $W/all.bin"
  rm -f "$W/all.bin"
  exit 2
fi

#------------------------------------------------------------------------------#
#  SWEEP                                                                       #
#------------------------------------------------------------------------------#
fail=0                          # Cases that differ from the baseline
san=0                           # Cases with a sanitizer report of the new program
n=0                             # Cases run
for prog in dos2unix mac2unix unix2dos unix2mac; do
  for mode in -ascii -7 -iso -437 -850 -860 -863 -865 -1252; do
    for extra in none l e le; do
      case "$extra" in
        none) opts=() ;;
        l)    opts=(-l) ;;
        e)    opts=(-e) ;;
        le)   opts=(-l -e) ;;
      esac
      n=$((n + 1))
      ra=0
      rb=0
      # -f: convert although the input contains binary characters.
      "$P/$prog" -f "$mode" "${opts[@]}" < "$W/all.bin" > "$W/a.out" 2> "$W/a.err" || ra=$?
      "$H/$prog" -f "$mode" "${opts[@]}" < "$W/all.bin" > "$W/b.out" 2> "$W/b.err" || rb=$?
      if ! cmp -s "$W/a.out" "$W/b.out" || ! cmp -s "$W/a.err" "$W/b.err" || [ "$ra" != "$rb" ]; then
        fail=$((fail + 1))
        echo "DIFF $prog $mode ${opts[*]}"
      fi
      if grep -qE 'AddressSanitizer|runtime error|LeakSanitizer' "$W/a.err"; then
        san=$((san + 1))
        echo "SANITIZER REPORT $prog $mode ${opts[*]}"
      fi
    done
  done
done
rm -f "$W/all.bin" "$W/a.out" "$W/a.err" "$W/b.out" "$W/b.err"

#------------------------------------------------------------------------------#
#  SUMMARY                                                                     #
#------------------------------------------------------------------------------#
echo "sweep: $n cases, $fail differences"
echo "sanitizer reports: $san"
if [ "$fail" -eq 0 ] && [ "$san" -eq 0 ]; then
  exit 0
fi
exit 1
