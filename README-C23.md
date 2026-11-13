# `dos2unix-c23` — ISO C23 Modernization & Verification Case Study

## Who & When
* **Author & Maintainer:** Basil Addington
* **Date:** November 12–13, 2026
* **Environment:** CachyOS Linux (`zsh`), Clang (`-std=c23`), `clang-tidy`, `clang-format`, `cflow`, KDevelop, and Git on a 2011 iMac 12,1.
* **Upstream Base:** Erwin Waterlander's [`dos2unix`](https://xs4all.nl) (2-Clause BSD License). The untouched upstream code is preserved on the `master` branch; all modernization work lives on `c23-modernize`.

---

## What This Repository Is
`dos2unix-c23` is a complete **ISO C23 (`-std=c23`)** modernization of the core C source and header files (`querycp.c`, `querycp.h`, `common.c`, `common.h`, `dos2unix.c`, `dos2unix.h`, `unix2dos.c`, `unix2dos.h`), paired with:
1. **A repeatable AI-assisted engineering rulebook (`CLAUDE.md`)** enforcing read-only reconnaissance, strict compiler/sanitizer checks, and comment auditing.
2. **A self-contained verification harness (`c23-verification/`)** capable of exporting and building a baseline directly from `master` to prove 100% byte-for-byte CLI output parity.

---

## Why Modernize Legacy C to ISO C23?
Legacy C89/C99 utilities rely on implicit conventions—`NULL` integer/pointer ambiguity, `int` flags acting as booleans, signed integer bitmasks, unguarded buffer size arithmetic, and magic numbers—that hide subtle bugs from the compiler.

Modernizing to ISO C23 strengthens compile-time guarantees and inline maintainability without altering runtime CLI behavior:
* **Null Pointer Safety:** Replaced 147 `NULL` occurrences with C23 `nullptr`.
* **Native Boolean Types:** Converted 12 `CFlag` fields, 4 predicate functions (`symbolic_link`, `binaryChar`, `binaryCharW`, `is_dos2unix`), and local state variables to `bool` / `true` / `false`.
* **Unsigned Bitmasks (`uint32_t`):** Migrated `CFlag.status`, `CFlag.file_info`, and UTF-16 surrogate bit shifts to unsigned `uint32_t`, eliminating signed-shift undefined behavior (`bugprone-signed-bitwise` dropped from **84 to 1** in `common.c`).
* **Compile-Time Constants & Bounds Checks:** Replaced 52 header macros and 16 conversion lookup tables with `constexpr` constants and `static_assert` size checks (`UCHAR_MAX + 1` entries per table; max BOM length checked against `MAX_LOOKAHEAD`).
* **Return-Value Enforcement:** Added `[[nodiscard]]` across 22 shared prototypes and 27 internal functions returning error codes, streams, or allocated pointers.
* **Checked Buffer Math:** Replaced raw size addition in `MakeTempFileFrom()` with `<stdckdint.h>` (`ckd_add`) and bounded `snprintf()`.
* **Process Exit Readability:** Standardized debug exits on `<stdlib.h>` `EXIT_FAILURE` while preserving and documenting the `dos2unix(1)` / `unix2dos(1)` manual-page `errno` exit contract in `main()`.

---

## How: The 4-Phase Engineering Workflow
Rather than letting an AI assistant rewrite files freely, every module was refactored under the deterministic harness documented in [`CLAUDE.md`](CLAUDE.md):

1. **Phase 0 — Tooling Baseline:** Enforced `.clang-format` and automated `clang-tidy` brace fixes in an isolated baseline commit first so formatting churn never obscured logic diffs. Generated `compile_commands.json` via `bear` and reverse caller maps via `cflow --cpp`.
2. **Phase 1 — Read-Only Recon & Plan:** Audited callers, `#ifdef` platform boundaries, and `clang-tidy` baselines before modifying a single line.
3. **Phase 2 — Scoped C23 Implementation:** Applied C23 upgrades one module at a time while preserving all legacy `#ifdef` blocks (DOS, Watcom, Windows, OS/2).
4. **Phase 3 — Multi-Layer Verification:**
   * **Strict C23 Build:** `clang -std=c23 -O2 -Wall -Wextra -Wconversion -Wpedantic -g` (**0 warnings, 0 errors**).
   * **Dynamic Sanitizers:** Compiled with `-fsanitize=address,undefined` (**0 sanitizer reports**).
   * **Regression & Parity Suites (`c23-verification/`):**
     * `verify.sh`: **7/7 pass** (including upstream's 174-test suite).
     * `parity_test.py`: **91/91 byte-identical cases** covering UTF-16LE/BE surrogate pairs, lone surrogates, partial BOMs, GB18030, symlinks, and `--info=` modes.
     * `sweep.sh`: **144/144 byte-identical cases** testing all 256 byte values across 9 conversion modes and all 4 personalities (`dos2unix`, `mac2unix`, `unix2dos`, `unix2mac`).
   * **Static Analysis (`clang-tidy`):** Warnings dropped from **500 → 348** (`common.c`), **82 → 37** (`dos2unix.c`), and **88 → 39** (`unix2dos.c`).
5. **Phase 4 — Comment & Upstream Bug Audit:** Verified every original BSD license header and ASCII banner remained intact, added "why" documentation for non-obvious invariants (same-directory atomic `rename()`, metadata ordering, lookahead rules), and reported portable C89/C99 bug discoveries upstream.

---

## Lessons Learned
1. **Separate Formatting from Logic:** Always commit `.clang-format` and brace standardization before starting semantic refactoring so diffs remain clean and reviewable.
2. **Treat AI Like a Junior Engineer Inside Guardrails:** Compiler flags (`-Wconversion`, `-Wpedantic`), `ASan`/`UBSan`, and automated byte-parity scripts catch mistakes immediately—turning AI speed into deterministic reliability.
3. **Document "Why," Not Just "What":** Recording *why* a loop uses single-character lookahead or *why* `main()` returns `errno` prevents future maintainers from breaking subtle CLI contracts.
4. **Respect Upstream Portability:** Modern C23 features belong in a dedicated showcase fork, while portable bug fixes (like missing `<stdio.h>` in `-DTEST` harnesses or unguarded OS/2 API calls) should be isolated in minimal C89/C99 patches for upstream maintainers.

---

## Running the Verification Suite Locally
```zsh
# 1. Build with strict Clang C23 flags
make -C dos2unix clean
make -C dos2unix CC=clang CFLAGS="-std=c23 -O2 -Wall -Wextra -Wconversion -Wpedantic -g" dos2unix unix2dos

# 2. Run the self-contained verification suite (builds baseline from master automatically)
./c23-verification/verify.sh
./c23-verification/parity_test.py
./c23-verification/sweep.sh
```
