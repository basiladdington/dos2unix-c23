#!/usr/bin/env python3
"""Byte-for-byte parity test: modernized dos2unix against the baseline binary.

For every case both binaries get the same input and arguments. The stdout
bytes, the stderr text, the exit status and (for file cases) the resulting
file contents must be identical. Any sanitizer report in stderr is a failure.

usage: parity_test.py MODERN_BINARY BASELINE_BINARY WORKDIR
"""
import os
import shutil
import subprocess
import sys

modern, orig, work = sys.argv[1:4]
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


failures = 0
total = 0


def check(name, results):
    global failures, total
    total += 1
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
        failures += 1
        print(f"FAIL  {name}: {', '.join(problems)}")
        if "sanitizer report" in problems or "stderr differs" in problems:
            print("   modern stderr:", err_m[:600])
            print("   orig   stderr:", err_o[:600])


for name, argv, data in STDIO:
    res = []
    for binary in (modern, orig):
        d = os.path.join(work, "case")
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        rc, out, err = run(binary, argv, data, d)
        res.append((rc, out, err, snapshot(d)))
    check("stdio: " + name, res)

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
    check("file: " + name, res)

print(f"{total - failures} of {total} cases identical to the baseline, {failures} failed")
sys.exit(1 if failures else 0)
