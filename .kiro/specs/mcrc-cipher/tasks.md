# Implementation Plan: MCRC Cipher — Verification and Completeness

## Overview

The implementation is complete. These tasks verify that the existing code compiles cleanly, that the verification suite passes, that all 13 correctness properties are explicitly covered, that CLI and demo behaviour matches requirements, and that the README accurately reflects the implementation. Gap-filling tasks add missing assertions to `verify.cpp` where properties are not yet explicitly checked.

---

## Tasks

- [x] 1. Build the project and confirm clean compilation
  - Configure with `cmake -S . -B build` and build with `cmake --build build`
  - Confirm zero warnings and zero errors (all targets: `mcrc_lib`, `mcrc`, `mcrc_demo`, `mcrc_verify`)
  - The build uses `-Wall -Wextra -Wpedantic -Werror`; any diagnostic is a failure
  - _Requirements: all (build is a prerequisite for every other requirement)_

- [x] 2. Run the verification suite and confirm all assertions pass
  - Execute `./build/mcrc_verify` (or `cmake --build build --target verify`)
  - Confirm the final line is `mcrc_verify: all checks passed` and exit code is `0`
  - _Requirements: 14.1, 14.2, 14.3_

- [x] 3. Verify the known-answer test is present and correct
  - Confirm `verify_integration()` in `tests/verify.cpp` asserts `mcrc::encrypt("QUEEN", "3:20") == "22-18-08-08-25"`
  - Confirm the matching decrypt assertion `mcrc::decrypt("22-18-08-08-25", "3:20") == "QUEEN"` is also present
  - _Requirements: 6.7, 14.4_

- [x] 4. Add missing property assertions to `tests/verify.cpp`
  - [x] 4.1 Add P7 assertion — reverse cipher code range
    - In `verify_reverse_cipher_roundtrip()`, after encoding a string, assert every value in the returned vector is `<= 25`
    - Use at least one multi-letter string (e.g. `"ABCDEFGHIJKLMNOPQRSTUVWXYZ"`) to cover all 26 codes
    - **Property 7: Reverse Cipher Code Range**
    - **Validates: Requirements 3.3**
  - [x] 4.2 Add P9 assertion — `format_codes` output length formula
    - In `verify_reverse_cipher_roundtrip()`, after calling `format_codes` on a non-empty vector, assert `result.size() == codes.size() * 3 - 1`
    - Also assert `format_codes({}).empty()` (empty-vector case, Req 4.1)
    - **Property 9: Serialised Output Length**
    - **Validates: Requirements 4.1, 4.2**
  - [x] 4.3 Add P11 assertion — ciphertext group count equals plaintext length
    - In `verify_integration()`, after `encrypt("QUEEN", "3:20")`, count the dash-separated groups in the result and assert the count equals `5` (length of `"QUEEN"`)
    - Add a helper lambda `count_groups(str)` that counts tokens split by `-`
    - **Property 11: Ciphertext Group Count Equals Plaintext Length**
    - **Validates: Requirements 6.8**
  - [x] 4.4 Add P12 assertion — strip policy idempotence
    - In `verify_integration()`, assert that `mcrc::encrypt("Queen!", "3:20")` equals `mcrc::encrypt("QUEEN", "3:20")` (pre-normalised input produces the same output under Strip)
    - **Property 12: Strip Policy Idempotence**
    - **Validates: Requirements 6.3**

- [x] 5. Checkpoint — rebuild and re-run the verification suite
  - Run `cmake --build build` then `./build/mcrc_verify`
  - Confirm `mcrc_verify: all checks passed` and exit code `0`
  - All 13 correctness properties (P1–P13) must now be explicitly covered by at least one assertion
  - _Requirements: 14.1, 14.2, 14.5_

- [x] 6. Verify CLI behaviour matches requirements
  - [x] 6.1 Verify successful encrypt and decrypt (exit code 0, output to stdout)
    - Run `./build/mcrc encrypt --key 3:20 --text QUEEN` and assert stdout is `22-18-08-08-25` and exit code is `0`
    - Run `./build/mcrc decrypt --key 3:20 --cipher 22-18-08-08-25` and assert stdout is `QUEEN` and exit code is `0`
    - _Requirements: 10.1, 11.1_
  - [x] 6.2 Verify exit code 1 for missing arguments and unknown input
    - Run `./build/mcrc` (no args) and assert exit code is `1` and output goes to stdout
    - Run `./build/mcrc encrypt` (missing `--key`/`--text`) and assert exit code is `1` and output goes to stderr
    - Run `./build/mcrc decrypt` (missing `--key`/`--cipher`) and assert exit code is `1` and output goes to stderr
    - Run `./build/mcrc unknowncmd` and assert exit code is `1` and output goes to stderr
    - _Requirements: 10.5, 11.4, 12.1, 12.3_
  - [x] 6.3 Verify exit code 2 for invalid time key
    - Run `./build/mcrc encrypt --key 0:00 --text QUEEN` and assert exit code is `2` and stderr contains `InvalidTimeKey`
    - Run `./build/mcrc decrypt --key 13:00 --cipher 22-18-08-08-25` and assert exit code is `2` and stderr contains `InvalidTimeKey`
    - _Requirements: 10.3, 11.2_
  - [x] 6.4 Verify exit code 3 for invalid ciphertext
    - Run `./build/mcrc decrypt --key 3:20 --cipher 99-00` and assert exit code is `3` and stderr contains `InvalidCiphertext`
    - _Requirements: 11.3_
  - [x] 6.5 Verify exit code 4 for invalid plaintext under `--reject`
    - Run `./build/mcrc encrypt --key 3:20 --text "Queen!" --reject` and assert exit code is `4` and stderr contains `InvalidPlaintext`
    - _Requirements: 10.2, 10.4_
  - [x] 6.6 Verify `--help` / `-h` exits with code 0
    - Run `./build/mcrc --help` and assert exit code is `0`
    - Run `./build/mcrc -h` and assert exit code is `0`
    - _Requirements: 12.2_

- [x] 7. Verify the demo tool works correctly
  - Run `./build/mcrc_demo --key 3:20 --text QUEEN` and confirm:
    - Exit code is `0`
    - stdout contains all five labelled pipeline stages: `Input (normalised)`, `Stage 1 (clockface)`, `Stage 2 (mirror)`, `Stage 3 (reverse num)`, `Stage 4 (mirror)`
    - stdout contains the clock grid header `Clock face for 3:20`
    - The `Stage 4 (mirror)` line shows `22-18-08-08-25`
  - Run `./build/mcrc_demo --key 3:20 --text QUEEN --no-clock` and confirm the clock grid is absent but all five pipeline stage lines are present
  - Run `./build/mcrc_demo` (no args) and confirm exit code is `1` and stderr contains usage information
  - Run `./build/mcrc_demo --key 0:00 --text QUEEN` and confirm exit code is `1` and stderr contains an error message
  - _Requirements: 13.1, 13.2, 13.3, 13.4, 13.5_

- [x] 8. Confirm all 14 requirements are satisfied — traceability review
  - Read through each requirement in `requirements.md` and cross-reference with the source files and verify suite
  - For each acceptance criterion, note which function/assertion covers it
  - Identify any criterion not covered by either `verify.cpp` or the CLI/demo checks above
  - If any gap is found, add a targeted assertion or fix to the appropriate source file
  - _Requirements: 1–14 (all)_

- [x] 9. Update README if gaps are found between documentation and actual behaviour
  - Compare the README's clock-face diagram, worked example, exit-code table, and error-message table against the actual implementation output
  - If any discrepancy is found (e.g. a flag name, exit code, or example output is wrong), update `README.md` to match the implementation
  - If no discrepancies are found, no changes are needed
  - _Requirements: 10, 11, 12, 13 (user-facing documentation accuracy)_

- [x] 10. Final checkpoint — full build and verification pass
  - Run `cmake --build build` and confirm zero errors
  - Run `./build/mcrc_verify` and confirm `mcrc_verify: all checks passed`
  - Ensure all tasks above are complete and no regressions have been introduced

## Notes

- Tasks marked with `*` are optional and can be skipped for a faster pass
- All property assertions added in Task 4 must be inside the existing `verify_*` functions in `tests/verify.cpp` — do not create new test binaries
- The verify suite uses `assert()` with `NDEBUG` forcibly undefined; any new assertion follows the same pattern
- Properties P1, P2, P3, P4, P5, P6, P8, P10, P13 are already covered by the existing verify suite; Tasks 4.1–4.4 add the four missing ones (P7, P9, P11, P12)
- CLI and demo verification in Tasks 6–7 is done by running the compiled binaries and inspecting exit codes and output — no new source files are needed
