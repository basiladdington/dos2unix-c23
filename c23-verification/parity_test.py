#!/usr/bin/env python3
"""Byte-for-byte parity test: modernized dos2unix against the baseline binary.

For every case both binaries get the same input and arguments. The stdout
bytes, the stderr text, the exit status and (for file cases) the resulting
file contents must be identical. Any sanitizer report in stderr is a failure.

How to run
----------
  ./c23-verification/parity_test.py
      Automatic mode, works from any current directory. Compares the newly
      built <repo>/dos2unix/dos2unix and <repo>/dos2unix/unix2dos (build them
      first) with the baseline binaries, see "Baseline" below. Both programs
      run the same cases.

  ./c23-verification/parity_test.py MODERN_BINARY BASELINE_BINARY [WORKDIR]
      Explicit mode: compare two given binaries. The cases run in the
      directory WORKDIR/case, which is deleted and created again for every
      case. Without WORKDIR a temporary directory is used.

The exit status is 0 when every case is identical, 1 when a case differs and
2 for a usage or setup error.

Paths
-----
All paths are derived from the directory that holds this script:
<repo>/c23-verification/ for the script and <repo>/dos2unix/ for the newly
built programs. Nothing is written into the repository.

Baseline (automatic mode)
-------------------------
Each baseline program is looked up on its own:
  1. $D2U_BASELINE (dos2unix) and $U2D_BASELINE (unix2dos), when set: paths of
     pre-built upstream binaries.
  2. $HOME/Projects/c23-sandbox/baselines/dos2unix.orig and unix2dos.orig,
     when present: the author's local copies.
  3. Automatic fallback for what is still missing: the upstream sources are
     exported with "git archive master dos2unix | tar -x" into a temporary
     directory, and dos2unix and unix2dos are built there with make.
Environment variables for the fallback:
  D2U_BASELINE_REF          git ref to build instead of master (origin/master
                            is tried when the clone has no local master).
  D2U_FORCE_BASELINE_BUILD  set to 1 to always build both from git.
  CC                        compiler for the baseline build (default: clang).
The temporary directory, with the baseline build and the working files, is
removed when the script ends, also after an error or Ctrl-C.

Needs Python 3.8 or newer. The baseline fallback also needs git, tar, make
and a C compiler.
"""
import os
import shutil
import signal
import subprocess
import sys
import tempfile

# All paths start at the directory of this script, so the test runs from any
# clone and from any current directory.
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_SRC = os.path.join(REPO_ROOT, "dos2unix")
LOCAL_BASELINES = os.path.join(os.path.expanduser("~"), "Projects", "c23-sandbox", "baselines")
BUILD_HINT = (
    f'  make -C "{REPO_SRC}" CC=clang '
    'CFLAGS="-std=c23 -O2 -Wall -Wextra -Wconversion -Wpedantic -g" dos2unix unix2dos'
)

ENV = dict(os.environ, LC_ALL="C.UTF-8", LANGUAGE="C")
SAN_MARKERS = (b"AddressSanitizer", b"runtime error", b"LeakSanitizer", b"UndefinedBehaviorSanitizer")


def u16(s, order):
    return s.encode("utf-16-le" if order == "le" else "utf-16-be")


BOM_LE, BOM_BE, BOM_U8, BOM_GB = b"\xff\xfe", b"\xfe\xff", b"\xef\xbb\xbf", b"\x84\x31\x95\x33"
SMILE = "\U0001F600"  # needs a surrogate pair in UTF-16: D83D DE00
TEXT = "plain\r\nca" + SMILE + "t é\r\nlast"
LONE_LEAD_LE = BOM_LE + u16("a\r\n", "le") + b"\x3d\xd8" + u16("b\r\n", "le")
LONE_TRAIL_LE = BOM_LE + u16("a\r\n", "le") + b"\x00\xde" + u16("b\r\n", "le")
LEAD_AT_EOF_LE = BOM_LE + u16("a\r\n", "le") + b"\x3d\xd8"
ODD_BYTES_LE = BOM_LE + u16("a\r\nb", "le") + b"\x41"

# (name, argv, input bytes)   -- converted through stdin/stdout
STDIO = [
    ("crlf", [], b"a\r\nb\r\nc"),
    ("mac mode", ["-c", "mac"], b"a\rb\rc\r"),
    ("iso", ["-iso"], b"caf\x82\r\n"),
    ("iso 850", ["-850"], b"caf\x82\r\n"),
    ("7bit", ["-7"], b"caf\x82\r\n"),
    ("newline -l", ["-l"], b"a\r\nb\r\n"),
    ("add eol -e", ["-e"], b"a\r\nb"),
    ("binary skip", [], b"a\x00b\r\n"),
    ("binary force", ["-f"], b"a\x00b\r\n"),
    ("binary error", ["--error-binary"], b"a\x01b\r\n"),
    ("empty", [], b""),
    ("one byte ff", [], b"\xff"),
    ("one byte ef", [], b"\xef"),
    ("partial utf8 bom ef bb", [], b"\xef\xbb"),
    ("partial utf8 bom ef bb 41", [], b"\xef\xbb\x41\r\n"),
    ("partial gb bom 84 31", [], b"\x84\x31"),
    ("partial gb bom 84 31 95", [], b"\x84\x31\x95"),
    ("partial gb bom 84 31 95 41", [], b"\x84\x31\x95\x41\r\n"),
    ("fe then text", [], b"\xfe\x41\r\n"),
    ("utf8 bom", [], BOM_U8 + b"a\r\nb\r\n"),
    ("utf8 bom keep -b", ["-b"], BOM_U8 + b"a\r\nb\r\n"),
    ("gb18030 bom", [], BOM_GB + b"a\r\nb\r\n"),
    ("gb18030 bom keep -b", ["-b"], BOM_GB + b"a\r\nb\r\n"),
    ("add bom -m", ["-m"], b"a\r\nb\r\n"),
    ("add then remove bom", ["-m", "-r"], b"a\r\nb\r\n"),
    ("utf16le surrogate pair", [], BOM_LE + u16(TEXT, "le")),
    ("utf16be surrogate pair", [], BOM_BE + u16(TEXT, "be")),
    ("utf16le keep -u", ["-u"], BOM_LE + u16(TEXT, "le")),
    ("utf16be keep -u", ["-u"], BOM_BE + u16(TEXT, "be")),
    ("utf16le keep -u -b", ["-u", "-b"], BOM_LE + u16(TEXT, "le")),
    ("utf16be keep -u -m", ["-u", "-m"], BOM_BE + u16(TEXT, "be")),
    ("utf16le add bom -m", ["-m"], BOM_LE + u16(TEXT, "le")),
    ("utf16le assumed -ul", ["-ul"], u16(TEXT, "le")),
    ("utf16be assumed -ub", ["-ub"], u16(TEXT, "be")),
    ("utf16le verbose", ["-v"], BOM_LE + u16(TEXT, "le")),
    ("utf16le mac mode", ["-c", "mac"], BOM_LE + u16("a\rb\r" + SMILE + "\r", "le")),
    ("utf16le lone lead", [], LONE_LEAD_LE),
    ("utf16le lone trail", [], LONE_TRAIL_LE),
    ("utf16le lead at eof", [], LEAD_AT_EOF_LE),
    ("utf16le odd byte count", [], ODD_BYTES_LE),
    ("utf16le binary", [], BOM_LE + u16("a\x01b\r\n", "le")),
    ("utf16le nul char -f", ["-f"], BOM_LE + u16("a\x00b\r\n", "le")),
    ("info stdin -i", ["-i"], b"a\r\nb\nc\rd"),
    ("info stdin --info", ["--info"], b"a\r\nb\nc\rd"),
    ("info stdin --info=dumbte", ["--info=dumbte"], BOM_U8 + b"a\r\nb\nc\r"),
    ("info stdin -ih", ["-ih"], b"a\r\nb\n"),
    ("info stdin -i0", ["-i0"], b"a\r\nb\n"),
    ("info stdin -ic", ["-ic"], b"a\r\nb\n"),
    ("info stdin bad flag", ["-iz"], b"a\r\n"),
    ("info stdin --info=bad", ["--info=q"], b"a\r\n"),
    ("info stdin utf16", ["-i"], BOM_LE + u16(TEXT, "le")),
    ("info stdin utf16be -ib", ["-ib"], BOM_BE + u16(TEXT, "be")),
    ("info stdin binary", ["-it"], b"a\x00b\r\n"),
    ("bad option", ["--nonsense"], b""),
    ("bad convmode", ["-c", "nope"], b""),
    ("convmode missing arg", ["-c"], b""),
    ("help", ["-h"], b""),
    ("license", ["-L"], b""),
    ("quiet binary", ["-q"], b"a\x00b\r\n"),
]

# (name, argv before the file names, {filename: bytes}, argv after)  -- file cases
FILES = [
    ("oldfile crlf", [], {"a.txt": b"a\r\nb\r\n"}, ["a.txt"]),
    ("oldfile verbose", ["-v"], {"a.txt": b"a\r\nb\r\n"}, ["a.txt"]),
    ("oldfile keepdate", ["-k"], {"a.txt": b"a\r\nb\r\n"}, ["a.txt"]),
    ("oldfile two files", [], {"a.txt": b"a\r\n", "b.txt": b"b\r\n"}, ["a.txt", "b.txt"]),
    ("oldfile binary", [], {"a.bin": b"a\x00b\r\n"}, ["a.bin"]),
    ("oldfile binary error", ["--error-binary"], {"a.bin": b"a\x00b\r\n"}, ["a.bin"]),
    ("oldfile missing", [], {}, ["missing.txt"]),
    ("oldfile directory", [], {}, ["."]),
    ("newfile", ["-n"], {"in.txt": b"a\r\nb\r\n"}, ["in.txt", "out.txt"]),
    ("newfile unpaired", ["-n"], {"in.txt": b"a\r\nb\r\n"}, ["in.txt"]),
    ("newfile then oldfile", ["-n"], {"in.txt": b"a\r\n", "c.txt": b"c\r\n"}, ["in.txt", "out.txt", "-o", "c.txt"]),
    ("to stdout -O", ["-O"], {"a.txt": b"a\r\nb\r\n"}, ["a.txt"]),
    ("to stdout utf16", ["-O"], {"a.txt": BOM_LE + u16(TEXT, "le")}, ["a.txt"]),
    ("oldfile utf16le", [], {"a.txt": BOM_LE + u16(TEXT, "le")}, ["a.txt"]),
    ("oldfile utf16be verbose", ["-v"], {"a.txt": BOM_BE + u16(TEXT, "be")}, ["a.txt"]),
    ("oldfile utf16 keep", ["-u", "-v"], {"a.txt": BOM_LE + u16(TEXT, "le")}, ["a.txt"]),
    ("oldfile utf16 lone lead", [], {"a.txt": LONE_LEAD_LE}, ["a.txt"]),
    ("oldfile utf16 lead at eof", ["-v"], {"a.txt": LEAD_AT_EOF_LE}, ["a.txt"]),
    ("two utf16 files, lead at eof first", [], {"a.txt": LEAD_AT_EOF_LE, "b.txt": BOM_LE + u16(TEXT, "le")}, ["a.txt", "b.txt"]),
    ("oldfile gb18030 bom verbose", ["-v"], {"a.txt": BOM_GB + b"a\r\n"}, ["a.txt"]),
    ("oldfile utf8 bom verbose", ["-v", "-b"], {"a.txt": BOM_U8 + b"a\r\n"}, ["a.txt"]),
    ("info files", ["-i"], {"a.txt": b"a\r\nb\n", "b.txt": BOM_LE + u16(TEXT, "le"), "c.bin": b"\x00\r\n"}, ["a.txt", "b.txt", "c.bin"]),
    ("info header nopath", ["-ihp"], {"a.txt": b"a\r\nb\n"}, ["./a.txt"]),
    ("info convert only", ["-ic"], {"a.txt": b"a\r\n", "b.txt": b"b\n"}, ["a.txt", "b.txt"]),
    ("info --info=due", ["--info=due"], {"a.txt": b"a\r\nb"}, ["a.txt"]),
    ("info missing", ["-i"], {}, ["missing.txt"]),
    ("symlink skip", [], {"t.txt": b"a\r\n", "@l.txt": b"t.txt"}, ["l.txt"]),
    ("symlink follow -F", ["-F"], {"t.txt": b"a\r\n", "@l.txt": b"t.txt"}, ["l.txt"]),
    ("symlink replace -R", ["-R"], {"t.txt": b"a\r\n", "@l.txt": b"t.txt"}, ["l.txt"]),
    ("symlink to dir", [], {"@l.txt": b"."}, ["l.txt"]),
    ("symlink info", ["-i"], {"t.txt": b"a\r\n", "@l.txt": b"t.txt"}, ["l.txt"]),
    ("newfile out is symlink -F", ["-F", "-n"], {"in.txt": b"a\r\n", "t.txt": b"x", "@l.txt": b"t.txt"}, ["in.txt", "l.txt"]),
]


def snapshot(d):
    out = {}
    for name in sorted(os.listdir(d)):
        p = os.path.join(d, name)
        if os.path.islink(p):
            out[name] = b"SYMLINK->" + os.readlink(p).encode()
        elif os.path.isfile(p):
            out[name] = open(p, "rb").read()
    return out


def run(binary, argv, stdin, cwd):
    # Both binaries are started under the same argv[0], so messages are comparable.
    p = subprocess.run(["dos2unix"] + argv, executable=binary, input=stdin, cwd=cwd, env=ENV, capture_output=True)
    return p.returncode, p.stdout, p.stderr


def check(name, results):
    """Compare the results of the two binaries. Returns True when they are identical."""
    (rc_m, out_m, err_m, snap_m), (rc_o, out_o, err_o, snap_o) = results
    problems = []
    if any(m in err_m for m in SAN_MARKERS):
        problems.append("sanitizer report")
    if rc_m != rc_o:
        problems.append(f"exit status {rc_m} != {rc_o}")
    if out_m != out_o:
        problems.append("stdout differs")
    if err_m != err_o:
        problems.append("stderr differs")
    if snap_m != snap_o:
        problems.append("resulting files differ")
    if problems:
        print(f"FAIL  {name}: {', '.join(problems)}")
        if "sanitizer report" in problems or "stderr differs" in problems:
            print("   modern stderr:", err_m[:600])
            print("   orig   stderr:", err_o[:600])
    return not problems


def run_suite(modern, orig, work):
    """Run all cases for one pair of binaries. Returns (total, failures)."""
    failures = 0
    total = 0

    for name, argv, data in STDIO:
        res = []
        for binary in (modern, orig):
            d = os.path.join(work, "case")
            shutil.rmtree(d, ignore_errors=True)
            os.makedirs(d)
            rc, out, err = run(binary, argv, data, d)
            res.append((rc, out, err, snapshot(d)))
        total += 1
        if not check("stdio: " + name, res):
            failures += 1

    for name, pre, files, post in FILES:
        res = []
        for binary in (modern, orig):
            d = os.path.join(work, "case")
            shutil.rmtree(d, ignore_errors=True)
            os.makedirs(d)
            for fn, content in files.items():
                if fn.startswith("@"):
                    os.symlink(content.decode(), os.path.join(d, fn[1:]))
                else:
                    with open(os.path.join(d, fn), "wb") as fh:
                        fh.write(content)
                    os.utime(os.path.join(d, fn), (1_600_000_000, 1_600_000_000))
            rc, out, err = run(binary, pre + post, b"", d)
            snap = snapshot(d)
            if "-k" in pre:
                snap["__mtime"] = str(int(os.stat(os.path.join(d, post[0])).st_mtime)).encode()
            res.append((rc, out, err, snap))
        total += 1
        if not check("file: " + name, res):
            failures += 1

    return total, failures


def setup_error(message):
    print("ERROR: " + message, file=sys.stderr)
    sys.exit(2)


def is_executable(p):
    return os.path.isfile(p) and os.access(p, os.X_OK)


def git_ref_exists(ref):
    return subprocess.run(
        ["git", "-C", REPO_ROOT, "rev-parse", "--verify", "--quiet", ref + "^{commit}"],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    ).returncode == 0


def build_baseline_from_git(tmp):
    """Fallback for hosts without pre-built baseline binaries.

    Export the upstream sources of the baseline ref from git into the
    temporary directory tmp and build dos2unix and unix2dos there. The caller
    removes tmp. The modernization branch starts at master, so master is the
    untouched upstream code. Returns (directory of the binaries, git ref).
    """
    for tool in ("git", "tar", "make"):
        if shutil.which(tool) is None:
            setup_error(f"building the baseline needs '{tool}', which is not in PATH.")
    inside = subprocess.run(["git", "-C", REPO_ROOT, "rev-parse", "--git-dir"],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if inside.returncode != 0:
        setup_error(f"{REPO_ROOT} is not a git checkout, so the baseline cannot be built from the "
                    "master branch. Set D2U_BASELINE and U2D_BASELINE to pre-built upstream binaries.")
    ref = os.environ.get("D2U_BASELINE_REF", "")
    if not ref:
        # A clone whose default branch is not master has only origin/master.
        ref = next((r for r in ("master", "origin/master") if git_ref_exists(r)), "")
    if not ref or not git_ref_exists(ref):
        setup_error(f"baseline git ref not found: {ref or 'master, origin/master'}. "
                    "Set D2U_BASELINE_REF, or D2U_BASELINE and U2D_BASELINE.")

    print(f"Building the baseline from git ref '{ref}' (temporary)...", flush=True)
    src = os.path.join(tmp, "baseline_src")
    os.makedirs(src)
    # git archive REF dos2unix | tar -x -C src
    archive = subprocess.Popen(["git", "-C", REPO_ROOT, "archive", ref, "dos2unix"], stdout=subprocess.PIPE)
    untar = subprocess.run(["tar", "-x", "-C", src], stdin=archive.stdout)
    archive.stdout.close()
    if archive.wait() != 0 or untar.returncode != 0:
        setup_error(f"exporting git ref '{ref}' failed.")
    build = subprocess.run(
        ["make", "-s", "-C", os.path.join(src, "dos2unix"), "CC=" + os.environ.get("CC", "clang"),
         "dos2unix", "unix2dos"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    if build.returncode != 0:
        sys.stderr.buffer.write(build.stdout)
        setup_error(f"building the baseline from git ref '{ref}' failed.")
    return os.path.join(src, "dos2unix"), ref


def find_baselines(tmp):
    """Return {program: (path, origin)} for dos2unix and unix2dos, see the header."""
    force = os.environ.get("D2U_FORCE_BASELINE_BUILD", "0") == "1"
    found = {}
    built = None
    for prog, var in (("dos2unix", "D2U_BASELINE"), ("unix2dos", "U2D_BASELINE")):
        given = os.environ.get(var, "")
        local = os.path.join(LOCAL_BASELINES, prog + ".orig")
        if not force and given:
            if not is_executable(given):
                setup_error(f"baseline binary not found or not executable: {given}")
            found[prog] = (given, "pre-built, from " + var)
        elif not force and is_executable(local):
            found[prog] = (local, "pre-built local copy")
        else:
            if built is None:
                built = build_baseline_from_git(tmp)
            found[prog] = (os.path.join(built[0], prog), f"built from git ref '{built[1]}'")
    return found


def main(argv):
    # Leave through the normal exit path on SIGTERM, so that the temporary
    # directory is removed.
    signal.signal(signal.SIGTERM, lambda signum, frame: sys.exit(143))

    if len(argv) not in (0, 2, 3):
        print("usage: parity_test.py                                          (automatic mode)\n"
              "       parity_test.py MODERN_BINARY BASELINE_BINARY [WORKDIR]  (explicit mode)\n"
              "See the header of this script for details.", file=sys.stderr)
        return 2

    # The temporary directory holds the baseline build and the working files.
    # It is removed when the 'with' block is left, also by an error or Ctrl-C.
    with tempfile.TemporaryDirectory(prefix="d2u_parity.") as tmp:
        if argv:
            # Explicit mode.
            modern, orig = os.path.abspath(argv[0]), os.path.abspath(argv[1])
            for binary in (modern, orig):
                if not is_executable(binary):
                    setup_error(f"binary not found or not executable: {binary}")
            work = os.path.abspath(argv[2]) if len(argv) == 3 else os.path.join(tmp, "work")
            os.makedirs(work, exist_ok=True)
            total, failures = run_suite(modern, orig, work)
            print(f"{total - failures} of {total} cases identical to the baseline, {failures} failed")
            return 1 if failures else 0

        # Automatic mode: both programs of this repository.
        for prog in ("dos2unix", "unix2dos"):
            if not is_executable(os.path.join(REPO_SRC, prog)):
                setup_error(f"{os.path.join(REPO_SRC, prog)} not found. Build the project first:\n{BUILD_HINT}")
        baselines = find_baselines(tmp)
        work = os.path.join(tmp, "work")
        os.makedirs(work)
        failed = False
        for prog in ("dos2unix", "unix2dos"):
            modern = os.path.join(REPO_SRC, prog)
            orig, origin = baselines[prog]
            print(f"{prog}: {modern}\n  against the baseline {orig} ({origin})", flush=True)
            total, failures = run_suite(modern, orig, work)
            print(f"{prog}: {total - failures} of {total} cases identical to the baseline, {failures} failed")
            failed = failed or failures > 0
        return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
