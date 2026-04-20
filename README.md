# MCRC — Mirror Clock Reverse Cipher

A C++20 implementation of the Mirror Clock Reverse Cipher (MCRC), a
four-stage hybrid cipher designed for a cryptography class assignment.
Given a plaintext and a time key such as `3:20`, MCRC produces a
dash-separated sequence of two-digit numeric codes. Each stage is an
involution, so decryption is just the pipeline run backward.

## Pipeline

```
plaintext -> [clockface] -> [mirror] -> [reverse cipher] -> [mirror] -> ciphertext
```

1. **Clockface substitution** — a letter-for-letter substitution keyed
   by a clock-face layout (see below).
2. **Mirror permutation** — reverse the character order of the string.
3. **Reverse cipher substitution** — replace each letter with a
   two-digit number using the fixed table `A=25, B=24, …, Z=00`.
4. **Mirror permutation** — reverse the resulting sequence of numeric
   pairs. Joining those pairs with `-` produces the final ciphertext.

### Worked example — `QUEEN` with key `3:20`

| Stage                 | Output              |
| --------------------- | ------------------- |
| Plaintext             | `QUEEN`             |
| 1. Clockface          | `DHRRA`             |
| 2. Mirror             | `ARRHD`             |
| 3. Reverse cipher     | `25-08-08-18-22`    |
| 4. Mirror (final)     | `22-18-08-08-25`    |

Hand-decrypting is the same sequence in reverse.

## Clockface substitution

Picture an analog clock with two concentric rings of letters. The 24
positions (12 on each ring) hold the 24 letters that are not `M` or
`Z`. `M` and `Z` sit at the centre and are never moved.

```
                    12
                A        N
         11                    1
       L                          O
    10                              2
   K                                  P

 9    J              +              Q    3
                    M Z
   I                                  R
    8                                 4
       H                          S
         7                     5
                G        T
                     6
```

- **Inner ring (clock 1..12):** `A B C D E F G H I J K L`
- **Outer ring (clock 1..12):** `N O P Q R S T U V W X Y`
- **Centre (fixed):** `M`, `Z`

Encryption swaps the letter at each clock position with its partner on
the opposite ring, i.e. a plain ROT13 across the 24 non-centre letters:

```
A<->N  B<->O  C<->P  D<->Q  E<->R  F<->S
G<->T  H<->U  I<->V  J<->W  K<->X  L<->Y
M, Z  -> themselves
```

Because the mapping is an involution, a single swap table handles both
directions. The mapping is verified by `QUEEN` → `DHRRA`:

```
Q -> D   U -> H   E -> R   E -> R   N -> A
```

### Time key

A time key in the form `H:MM` or `HH:MM` (hour `1..12`, minute
`0..59`) is a required, validated input to the pipeline, but **v1
does not mix the time into the substitution**. The key is reserved
for future variants of the cipher where the hour and minute hands
will drive ring rotations; see the writeup for the class assignment
for details. The CLI and library still reject invalid keys so the
interface is stable across future variants.

## Reverse cipher table

```
A=25  B=24  C=23  D=22  E=21  F=20  G=19  H=18  I=17
J=16  K=15  L=14  M=13  N=12  O=11  P=10  Q=09  R=08
S=07  T=06  U=05  V=04  W=03  X=02  Y=01  Z=00
```

Defined as a `constexpr` table in
[`include/mcrc/reverse_cipher.hpp`](include/mcrc/reverse_cipher.hpp).

## Build

Requires CMake ≥ 3.20 and a C++20-capable compiler. Tested clean on
`g++` 13 and `clang++` 18 with `-Wall -Wextra -Wpedantic -Werror`. The
code uses only the standard library.

```bash
cmake -S . -B build
cmake --build build
./build/mcrc_verify      # run the assertion-based test suite
```

The `verify` custom target builds and runs the verifier in one go:

```bash
cmake --build build --target verify
```

### C++17 fallback

The code targets C++20 for `constexpr` lambdas, `std::string_view`
conveniences, and `constexpr std::array` initialisation. If your
compiler lacks C++20 support, lowering the standard to C++17 works
after one small change: replace the immediately-invoked `constexpr`
lambda in `include/mcrc/reverse_cipher.hpp` with a named
`constexpr` function (the lambda form requires C++20). No other
source changes are required.

## CLI

```bash
./build/mcrc encrypt --key 3:20 --text QUEEN
# 22-18-08-08-25

./build/mcrc decrypt --key 3:20 --cipher 22-18-08-08-25
# QUEEN

./build/mcrc encrypt --key 3:20 --text "Queen of hearts" --reject
# InvalidPlaintext: non-letter character in plaintext under Reject policy

./build/mcrc encrypt --key 3:20 --text "Queen of hearts"
# default is Strip; non-letters are silently dropped
```

### Demo walkthrough

```bash
./build/mcrc_demo --key 3:20 --text QUEEN
```

Shows an ASCII clock face with the hour hand (`H`) and minute hand
(`M`), followed by the output of each pipeline stage. Use
`--no-clock` to suppress the clock rendering.

## Library layout

```
include/mcrc/
├── clockface.hpp        stage 1 + TimeKey parser
├── mirror.hpp           stage 2 / 4
├── reverse_cipher.hpp   stage 3 + code formatting/parsing
├── pipeline.hpp         encrypt / decrypt / encrypt_trace
└── exceptions.hpp       InvalidTimeKey / InvalidCiphertext / InvalidPlaintext

src/
├── clockface.cpp
├── mirror.cpp
├── pipeline.cpp
└── reverse_cipher.cpp

apps/
├── cli.cpp       -> mcrc
└── demo.cpp      -> mcrc_demo

tests/
└── verify.cpp    -> mcrc_verify
```

All public declarations carry Doxygen-style comments.

## Error handling

Three custom exception types, all deriving from `std::runtime_error`:

| Type                  | Raised by                                                         |
| --------------------- | ----------------------------------------------------------------- |
| `InvalidTimeKey`      | `parse_time_key` on malformed or out-of-range keys                |
| `InvalidCiphertext`   | `parse_codes` / `reverse_cipher_decode` on malformed cipher input |
| `InvalidPlaintext`    | pipeline under `Reject` policy when plaintext has non-letters     |

## Verification

`mcrc_verify` is a single executable packed with `assert()` calls:

- Individual layer round-trips (clockface, mirror, reverse cipher).
- The integration assertion `encrypt("QUEEN", "3:20") == "22-18-08-08-25"`.
- 100 pseudo-random A–Z plaintexts × random valid time keys, each
  asserting `decrypt(encrypt(p, k), k) == p`.
- Error-path assertions for each custom exception type.

The verifier unconditionally undefines `NDEBUG` at the top of its
translation unit so assertions fire regardless of build type.

## Hand-encrypting

A classmate can reproduce the cipher with pen and paper:

1. Strip non-letters and upper-case the plaintext.
2. For each letter, apply the ROT13 partner swap (leave `M`, `Z` alone).
3. Reverse the string.
4. Look up each letter in the reverse-cipher table above to get a
   two-digit number; join the numbers with `-`.
5. Reverse the list of numbers. That is the ciphertext.

Decryption is the same steps in reverse.
