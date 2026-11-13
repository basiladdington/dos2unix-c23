# dos2unix — ISO C23 Modernization Branch (`c23-modernize`)

This branch modernizes the core C source and header files of `dos2unix` (`dos2unix.c`, `unix2dos.c`, `common.c`, `querycp.c`) to **ISO C23 (`-std=c23`)** while preserving 100% byte-for-byte CLI output parity with the upstream release.

## Key C23 & Safety Upgrades
- **Type Safety & Null Pointers:** Replaced `NULL` with `nullptr`, converted boolean integer flags in `CFlag` and predicates to native `bool`, and migrated bitmask fields (`status`, `file_info`) to unsigned `uint32_t`.
- **Compile-Time Guarantees:** Replaced magic numbers and lookup tables with `constexpr` constants and `static_assert` bounds checks, and added `[[nodiscard]]` to functions returning error codes or allocated streams.
- **Checked Arithmetic:** Used `<stdckdint.h>` (`ckd_add`) with bounded `snprintf()` for temporary filename generation.
- **In-Code Documentation:** Preserved all original BSD license headers and ASCII docblocks while documenting buffer invariants, BOM push-back rules, exit-code contracts, and untestable platform `#ifdef` branches.

## Building & Running the Verification Suite
```zsh
# 1. Build with strict Clang C23 warnings
make -C dos2unix clean
make -C dos2unix CC=clang CFLAGS="-std=c23 -O2 -Wall -Wextra -Wconversion -Wpedantic -g" dos2unix unix2dos

# 2. Build with AddressSanitizer + UndefinedBehaviorSanitizer
make -C dos2unix clean
make -C dos2unix CC=clang CFLAGS="-std=c23 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra" dos2unix unix2dos

# 3. Run the verification scripts inside c23-verification/
./c23-verification/verify.sh
```

See `CLAUDE.md` for the 4-phase engineering workflow, 3-wave refactoring strategy, and comment-auditing rules used on this branch.
