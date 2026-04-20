# Requirements Document

## Introduction

The Mirror Clock Reverse Cipher (MCRC) is a 4-stage hybrid encryption algorithm implemented as a C++20 static library (`mcrc_lib`) with three companion executables: a CLI tool (`mcrc`), a demonstration tool (`mcrc_demo`), and an assertion-based verification suite (`mcrc_verify`). The pipeline applies, in order: a clockface letter substitution (Stage 1), a mirror permutation (Stage 2), a reverse-alphabet numeric encoding (Stage 3), and a second mirror permutation (Stage 4). Decryption applies the inverse stages in reverse order. The time key is validated at every entry point but is not mixed into the substitution in v1.

---

## Glossary

- **Clockface_Layer**: The component implementing Stage 1 — the ROT13-over-24-letters substitution (`clockface_encode`, `clockface_decode`, `parse_time_key`).
- **Mirror_Layer**: The component implementing Stages 2 and 4 — character-order or code-order reversal (`mirror(string)`, `mirror(vector<uint8_t>)`).
- **Reverse_Cipher_Layer**: The component implementing Stage 3 — reverse-alphabet numeric encoding (`reverse_cipher_encode`, `reverse_cipher_decode`).
- **Serialiser**: The component responsible for converting between code vectors and dash-separated ciphertext strings (`format_codes`, `parse_codes`).
- **Pipeline**: The orchestration layer that chains all four stages for encryption and decryption (`encrypt`, `decrypt`, `encrypt_trace`).
- **Time_Key_Parser**: The function `parse_time_key` that validates and parses a time key string into a `TimeKey` struct.
- **CLI**: The `mcrc` command-line executable exposing `encrypt` and `decrypt` subcommands.
- **Demo**: The `mcrc_demo` executable that renders an ASCII clock face and prints the full pipeline trace.
- **Verification_Suite**: The `mcrc_verify` executable containing assertion-based checks for all components.
- **TimeKey**: A validated struct with `hour ∈ [1, 12]` and `minute ∈ [0, 59]`.
- **Normalised_Plaintext**: The result of uppercasing all characters in the input and removing (Strip) or rejecting (Reject) any non-letter characters.
- **Ciphertext**: A dash-separated sequence of zero-padded two-digit decimal groups, each in the range `[00, 25]`, e.g. `"22-18-08-08-25"`. An empty string is a valid ciphertext representing empty plaintext.
- **NonLetterPolicy**: An enum controlling how non-letter characters in plaintext are handled: `Strip` (silently remove) or `Reject` (throw `InvalidPlaintext`).
- **InvalidTimeKey**: Exception thrown when a time key string fails format or range validation.
- **InvalidCiphertext**: Exception thrown when a ciphertext string fails format or range validation.
- **InvalidPlaintext**: Exception thrown when plaintext contains disallowed characters under the `Reject` policy, or when a non-letter reaches an internal stage.

---

## Requirements

### Requirement 1: Clockface Substitution (Stage 1)

**User Story:** As a developer using `mcrc_lib`, I want a letter-substitution function based on a dual-ring clockface layout, so that Stage 1 of the MCRC pipeline applies a well-defined, invertible character mapping.

#### Acceptance Criteria

1. THE Clockface_Layer SHALL map each letter in `A–L` to its ROT13 partner in `N–Y` (i.e. `A↔N`, `B↔O`, …, `L↔Y`) and SHALL map `M` and `Z` to themselves.
2. WHEN `clockface_encode` is applied twice to the same uppercase string with any valid `TimeKey`, THE Clockface_Layer SHALL return the original string (involution property).
3. WHEN `clockface_encode` is called with any two valid `TimeKey` values on the same uppercase input string, THE Clockface_Layer SHALL produce identical output (v1 key independence).
4. WHEN `clockface_encode` or `clockface_decode` receives a character outside `A–Z`, THE Clockface_Layer SHALL throw `InvalidPlaintext`.
5. THE Clockface_Layer SHALL produce an output string of the same length as the input string.

---

### Requirement 2: Mirror Permutation (Stages 2 and 4)

**User Story:** As a developer using `mcrc_lib`, I want a reversal function for both strings and code vectors, so that Stages 2 and 4 of the MCRC pipeline apply a deterministic, invertible permutation.

#### Acceptance Criteria

1. WHEN `mirror` is applied to a string, THE Mirror_Layer SHALL return a string containing the same characters in reverse order.
2. WHEN `mirror` is applied twice to the same string, THE Mirror_Layer SHALL return the original string (string involution property).
3. WHEN `mirror` is applied twice to the same `vector<uint8_t>`, THE Mirror_Layer SHALL return the original vector (vector involution property).
4. WHEN `mirror` is applied to an empty string, THE Mirror_Layer SHALL return an empty string.
5. WHEN `mirror` is applied to an empty `vector<uint8_t>`, THE Mirror_Layer SHALL return an empty vector.

---

### Requirement 3: Reverse Cipher Encoding (Stage 3)

**User Story:** As a developer using `mcrc_lib`, I want a numeric encoding that maps each letter to a two-digit code using the formula `code = 25 − (letter − 'A')`, so that Stage 3 of the MCRC pipeline converts letters to numeric codes in a well-defined, invertible way.

#### Acceptance Criteria

1. FOR ALL indices `i` in `[0, 25]`, THE Reverse_Cipher_Layer SHALL satisfy `kReverseCipherTable[i] == 25 − i` (i.e. `A=25`, `B=24`, …, `Z=0`).
2. WHEN `reverse_cipher_decode` is applied to the output of `reverse_cipher_encode` on any uppercase string, THE Reverse_Cipher_Layer SHALL return the original string (encode/decode round-trip).
3. FOR ALL outputs of `reverse_cipher_encode`, every code value SHALL be in the range `[0, 25]`.
4. WHEN `reverse_cipher_encode` receives a character outside `A–Z`, THE Reverse_Cipher_Layer SHALL throw `InvalidPlaintext`.
5. WHEN `reverse_cipher_decode` receives a code value greater than `25`, THE Reverse_Cipher_Layer SHALL throw `InvalidCiphertext`.

---

### Requirement 4: Ciphertext Serialisation

**User Story:** As a developer using `mcrc_lib`, I want functions to serialise a code vector to a dash-separated string and deserialise it back, so that MCRC ciphertext can be stored, transmitted, and parsed reliably.

#### Acceptance Criteria

1. WHEN `format_codes` is called on an empty vector, THE Serialiser SHALL return an empty string.
2. WHEN `format_codes` is called on a non-empty code vector, THE Serialiser SHALL return a string of length `codes.size() * 3 − 1` consisting of zero-padded two-digit groups separated by exactly one `-` character.
3. WHEN `parse_codes` is applied to the output of `format_codes` on any valid code vector, THE Serialiser SHALL return the original code vector (serialisation round-trip).
4. WHEN `parse_codes` receives an empty string, THE Serialiser SHALL return an empty vector.
5. IF `parse_codes` receives a string with a group that is not exactly two decimal digits, THEN THE Serialiser SHALL throw `InvalidCiphertext`.
6. IF `parse_codes` receives a string with a group value outside `[0, 25]`, THEN THE Serialiser SHALL throw `InvalidCiphertext`.
7. IF `parse_codes` receives a string with a missing separator, a double separator, or a trailing `-`, THEN THE Serialiser SHALL throw `InvalidCiphertext`.

---

### Requirement 5: Time Key Parsing and Validation

**User Story:** As a developer using `mcrc_lib`, I want a function that parses and validates time key strings, so that all pipeline entry points can enforce a well-formed key before processing.

#### Acceptance Criteria

1. WHEN `parse_time_key` receives a string in `H:MM` or `HH:MM` format with hour in `[1, 12]` and minute in `[0, 59]`, THE Time_Key_Parser SHALL return a `TimeKey` with the correct `hour` and `minute` values.
2. WHEN `parse_time_key` receives an empty string, THE Time_Key_Parser SHALL throw `InvalidTimeKey`.
3. WHEN `parse_time_key` receives a string without a `:` separator, THE Time_Key_Parser SHALL throw `InvalidTimeKey`.
4. WHEN `parse_time_key` receives a string with an hour value outside `[1, 12]` (including `0`), THE Time_Key_Parser SHALL throw `InvalidTimeKey`.
5. WHEN `parse_time_key` receives a string where the minute field is not exactly two digits or has a value outside `[0, 59]`, THE Time_Key_Parser SHALL throw `InvalidTimeKey`.
6. WHEN `parse_time_key` receives a string containing any non-digit character other than the `:` separator, THE Time_Key_Parser SHALL throw `InvalidTimeKey`.

---

### Requirement 6: Full Encryption Pipeline

**User Story:** As a developer using `mcrc_lib`, I want an `encrypt` function that chains all four pipeline stages, so that I can convert plaintext to MCRC ciphertext with a single call.

#### Acceptance Criteria

1. WHEN `encrypt` is called with a valid uppercase plaintext and a valid time key, THE Pipeline SHALL apply the four stages in order — clockface substitution, mirror, reverse cipher encoding, mirror — and return the formatted ciphertext.
2. WHEN `encrypt` is called with a plaintext containing mixed-case letters, THE Pipeline SHALL uppercase all letters before Stage 1.
3. WHEN `encrypt` is called with `NonLetterPolicy::Strip` and the plaintext contains non-letter characters, THE Pipeline SHALL silently remove those characters before Stage 1.
4. WHEN `encrypt` is called with `NonLetterPolicy::Reject` and the plaintext contains a non-letter character, THE Pipeline SHALL throw `InvalidPlaintext`.
5. WHEN `encrypt` is called with an empty string, THE Pipeline SHALL return an empty string.
6. WHEN `encrypt` is called with a malformed time key, THE Pipeline SHALL throw `InvalidTimeKey`.
7. WHEN `encrypt` is called with plaintext `"QUEEN"` and key `"3:20"`, THE Pipeline SHALL return `"22-18-08-08-25"` (known-answer test).
8. WHEN `encrypt` is called with any valid normalised plaintext and valid key, THE Pipeline SHALL return a ciphertext whose number of dash-separated groups equals the length of the normalised plaintext.

---

### Requirement 7: Full Decryption Pipeline

**User Story:** As a developer using `mcrc_lib`, I want a `decrypt` function that inverts the four pipeline stages, so that I can recover the original plaintext from MCRC ciphertext.

#### Acceptance Criteria

1. WHEN `decrypt` is called with a valid MCRC ciphertext and the same time key used for encryption, THE Pipeline SHALL return the normalised uppercase plaintext.
2. WHEN `decrypt` is called with an empty string, THE Pipeline SHALL return an empty string.
3. WHEN `decrypt` is called with a malformed time key, THE Pipeline SHALL throw `InvalidTimeKey`.
4. WHEN `decrypt` is called with a malformed ciphertext, THE Pipeline SHALL throw `InvalidCiphertext`.
5. FOR ALL valid normalised plaintext strings `p` and valid time keys `k`, `decrypt(encrypt(p, k), k)` SHALL equal `p` (full round-trip property).

---

### Requirement 8: Pipeline Trace

**User Story:** As a developer or educator, I want an `encrypt_trace` function that captures all intermediate pipeline stage outputs, so that I can inspect and display the step-by-step transformation.

#### Acceptance Criteria

1. WHEN `encrypt_trace` is called with a valid plaintext and time key, THE Pipeline SHALL return a `PipelineTrace` struct containing the normalised input, the output after Stage 1 (clockface), the output after Stage 2 (first mirror), the output after Stage 3 (reverse cipher, formatted), and the output after Stage 4 (second mirror, final ciphertext).
2. WHEN `encrypt_trace` is called, THE Pipeline SHALL produce a `PipelineTrace` whose `after_second_mirror` field equals the output of `encrypt` called with the same arguments.
3. WHEN `encrypt_trace` is called with a malformed time key, THE Pipeline SHALL throw `InvalidTimeKey`.

---

### Requirement 9: Exception Types

**User Story:** As a developer using `mcrc_lib`, I want well-typed exceptions for each category of validation failure, so that callers can distinguish and handle time key errors, ciphertext errors, and plaintext errors independently.

#### Acceptance Criteria

1. THE Pipeline SHALL throw `InvalidTimeKey` (and only `InvalidTimeKey`) for all time key format and range violations.
2. THE Pipeline SHALL throw `InvalidCiphertext` (and only `InvalidCiphertext`) for all ciphertext format and range violations.
3. THE Pipeline SHALL throw `InvalidPlaintext` (and only `InvalidPlaintext`) when a non-letter character is encountered under the `Reject` policy or reaches an internal stage.
4. WHEN any exception is thrown, THE Pipeline SHALL include a human-readable description of the failure in the exception message.

---

### Requirement 10: CLI — Encrypt Subcommand

**User Story:** As a user of the `mcrc` tool, I want an `encrypt` subcommand that accepts a time key and plaintext, so that I can encrypt text from the command line.

#### Acceptance Criteria

1. WHEN the CLI is invoked as `mcrc encrypt --key <H:MM> --text <PLAINTEXT>`, THE CLI SHALL print the MCRC ciphertext to stdout followed by a newline and exit with code `0`.
2. WHEN the CLI is invoked with `--reject` in addition to the encrypt flags, THE CLI SHALL apply `NonLetterPolicy::Reject` and exit with code `4` if the plaintext contains non-letter characters.
3. WHEN the CLI is invoked with a malformed `--key` value, THE CLI SHALL print the `InvalidTimeKey` error message to stderr and exit with code `2`.
4. WHEN the CLI is invoked with `--reject` and the plaintext contains non-letter characters, THE CLI SHALL print the `InvalidPlaintext` error message to stderr and exit with code `4`.
5. WHEN the CLI is invoked as `mcrc encrypt` without `--key` or `--text`, THE CLI SHALL print usage information to stderr and exit with code `1`.

---

### Requirement 11: CLI — Decrypt Subcommand

**User Story:** As a user of the `mcrc` tool, I want a `decrypt` subcommand that accepts a time key and ciphertext, so that I can decrypt MCRC ciphertext from the command line.

#### Acceptance Criteria

1. WHEN the CLI is invoked as `mcrc decrypt --key <H:MM> --cipher <NN-NN-...>`, THE CLI SHALL print the uppercase plaintext to stdout followed by a newline and exit with code `0`.
2. WHEN the CLI is invoked with a malformed `--key` value, THE CLI SHALL print the `InvalidTimeKey` error message to stderr and exit with code `2`.
3. WHEN the CLI is invoked with a malformed `--cipher` value, THE CLI SHALL print the `InvalidCiphertext` error message to stderr and exit with code `3`.
4. WHEN the CLI is invoked as `mcrc decrypt` without `--key` or `--cipher`, THE CLI SHALL print usage information to stderr and exit with code `1`.

---

### Requirement 12: CLI — General Behaviour

**User Story:** As a user of the `mcrc` tool, I want consistent usage feedback and exit codes, so that I can integrate the tool into scripts reliably.

#### Acceptance Criteria

1. WHEN the CLI is invoked with no arguments, THE CLI SHALL print usage information to stdout and exit with code `1`.
2. WHEN the CLI is invoked with `-h` or `--help`, THE CLI SHALL print usage information to stdout and exit with code `0`.
3. WHEN the CLI is invoked with an unknown subcommand or an unknown flag, THE CLI SHALL print usage information to stderr and exit with code `1`.

---

### Requirement 13: Demo Tool

**User Story:** As an educator or learner, I want a `mcrc_demo` tool that visualises the time key on an ASCII clock face and prints each pipeline stage, so that I can understand how the MCRC algorithm transforms plaintext step by step.

#### Acceptance Criteria

1. WHEN `mcrc_demo` is invoked with valid `--key` and `--text` flags, THE Demo SHALL print the normalised input and the outputs of all four pipeline stages to stdout.
2. WHEN `mcrc_demo` is invoked without `--no-clock`, THE Demo SHALL render an 11×23 ASCII clock grid with an `H` marker for the hour hand and an `M` marker for the minute hand before the pipeline trace.
3. WHEN `mcrc_demo` is invoked with `--no-clock`, THE Demo SHALL omit the clock rendering and print only the pipeline trace.
4. WHEN `mcrc_demo` is invoked without the required `--key` or `--text` flags, THE Demo SHALL print usage information to stderr and exit with code `1`.
5. WHEN `mcrc_demo` encounters any exception during processing, THE Demo SHALL print the error message to stderr and exit with code `1`.

---

### Requirement 14: Verification Suite

**User Story:** As a developer maintaining `mcrc_lib`, I want an assertion-based verification suite that exercises all components and a 100-iteration random round-trip, so that regressions are caught at build time.

#### Acceptance Criteria

1. WHEN `mcrc_verify` is executed, THE Verification_Suite SHALL run checks for the reverse cipher table, mirror layer, reverse cipher round-trip, clockface layer, time key parsing, ciphertext parsing errors, integration tests, and the 100-iteration random round-trip.
2. WHEN all checks pass, THE Verification_Suite SHALL print `"mcrc_verify: all checks passed"` to stdout and exit with code `0`.
3. WHEN any assertion fails, THE Verification_Suite SHALL abort execution and exit with a non-zero exit code.
4. THE Verification_Suite SHALL include the known-answer test: `encrypt("QUEEN", "3:20") == "22-18-08-08-25"`.
5. THE Verification_Suite SHALL include a random round-trip test that generates at least 100 random uppercase strings of length 1–40 with random valid time keys and asserts `decrypt(encrypt(p, k), k) == p` for each.

---

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system — essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Clockface Involution

*For any* uppercase string `s` and any valid `TimeKey` `k`, applying `clockface_encode` twice must return the original string: `clockface_encode(clockface_encode(s, k), k) == s`.

**Validates: Requirements 1.2**

---

### Property 2: Clockface Key Independence (v1)

*For any* uppercase string `s` and any two valid `TimeKey` values `k1` and `k2`, `clockface_encode(s, k1) == clockface_encode(s, k2)`.

**Validates: Requirements 1.3**

---

### Property 3: Mirror Involution (string)

*For any* string `s`, `mirror(mirror(s)) == s`.

**Validates: Requirements 2.2**

---

### Property 4: Mirror Involution (vector)

*For any* `vector<uint8_t>` `v`, `mirror(mirror(v)) == v`.

**Validates: Requirements 2.3**

---

### Property 5: Reverse Cipher Table Formula

*For all* indices `i` in `[0, 25]`, `kReverseCipherTable[i] == 25 − i`.

**Validates: Requirements 3.1**

---

### Property 6: Reverse Cipher Encode/Decode Round-Trip

*For any* uppercase string `s`, `reverse_cipher_decode(reverse_cipher_encode(s)) == s`.

**Validates: Requirements 3.2**

---

### Property 7: Reverse Cipher Code Range

*For any* uppercase string `s`, every value in `reverse_cipher_encode(s)` is in `[0, 25]`.

**Validates: Requirements 3.3**

---

### Property 8: Serialisation Round-Trip

*For any* valid code vector `v` (all values in `[0, 25]`), `parse_codes(format_codes(v)) == v`.

**Validates: Requirements 4.3**

---

### Property 9: Serialised Output Length

*For any* non-empty valid code vector `v`, `format_codes(v).size() == v.size() * 3 − 1`.

**Validates: Requirements 4.2**

---

### Property 10: Full Encrypt/Decrypt Round-Trip

*For any* normalised uppercase plaintext string `p` and any valid time key `k`, `decrypt(encrypt(p, k), k) == p`.

**Validates: Requirements 7.5**

---

### Property 11: Ciphertext Group Count Equals Plaintext Length

*For any* normalised uppercase plaintext string `p` and any valid time key `k`, the number of dash-separated groups in `encrypt(p, k)` equals `p.size()`.

**Validates: Requirements 6.8**

---

### Property 12: Strip Policy Idempotence

*For any* string `p` and any valid time key `k`, `encrypt(p, k, Strip) == encrypt(normalise(p), k, Strip)`, where `normalise` uppercases and removes all non-letter characters.

**Validates: Requirements 6.3**

---

### Property 13: Empty Input Identity

*For any* valid time key `k`, `encrypt("", k) == ""` and `decrypt("", k) == ""`.

**Validates: Requirements 6.5, 7.2**
