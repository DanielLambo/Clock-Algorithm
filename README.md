# MCRC — Mirror Clock Reverse Cipher

A custom encryption algorithm that turns plain text into a sequence of numbers — and back again. Created as a cryptography class project, MCRC combines four transformation steps inspired by an analog clock face.

> **Not for real security.** MCRC is a learning cipher designed to be understood and explained. Do not use it to protect sensitive information.

---

## What does it do?

You give MCRC a word (or any text) and a "time key" — a time like `3:20` — and it produces a coded sequence of numbers:

```
QUEEN  →  22-18-08-08-25
```

Give it the same time key and the coded numbers, and it gives you back the original word:

```
22-18-08-08-25  →  QUEEN
```

---

## How the cipher works (plain English)

MCRC applies four steps in order. Each step is reversible, so decryption just runs the steps backwards.

### Step 1 — Clockface substitution

Imagine an analog clock with two rings of letters around the face:

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

- The **inner ring** (positions 1–12) holds the letters: `A B C D E F G H I J K L`
- The **outer ring** (positions 1–12) holds the letters: `N O P Q R S T U V W X Y`
- `M` and `Z` sit at the centre and never change

Each letter swaps with the letter at the same clock position on the other ring:

```
A ↔ N    B ↔ O    C ↔ P    D ↔ Q    E ↔ R    F ↔ S
G ↔ T    H ↔ U    I ↔ V    J ↔ W    K ↔ X    L ↔ Y
M stays M,  Z stays Z
```

So `Q` becomes `D`, `U` becomes `H`, `E` becomes `R`, and so on.

`QUEEN` → `DHRRA`

### Step 2 — Mirror (reverse the letters)

Simply reverse the order of the letters from Step 1.

`DHRRA` → `ARRHD`

### Step 3 — Reverse cipher (letters become numbers)

Replace each letter with a number using this table — the alphabet backwards:

| A  | B  | C  | D  | E  | F  | G  | H  | I  | J  | K  | L  | M  |
|----|----|----|----|----|----|----|----|----|----|----|----|----|
| 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 |

| N  | O  | P  | Q  | R  | S  | T  | U  | V  | W  | X  | Y  | Z  |
|----|----|----|----|----|----|----|----|----|----|----|----|----|
| 12 | 11 | 10 | 09 | 08 | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |

`ARRHD` → `25-08-08-18-22`

### Step 4 — Mirror again (reverse the numbers)

Reverse the order of the number groups.

`25-08-08-18-22` → `22-18-08-08-25`

That's the final ciphertext.

---

### Full worked example

| Step | What happens | Result |
|------|-------------|--------|
| Start | Original text | `QUEEN` |
| Step 1 | Clockface swap | `DHRRA` |
| Step 2 | Reverse letters | `ARRHD` |
| Step 3 | Letters → numbers | `25-08-08-18-22` |
| Step 4 | Reverse numbers | `22-18-08-08-25` |

### Decryption (going backwards)

| Step | What happens | Result |
|------|-------------|--------|
| Start | Ciphertext | `22-18-08-08-25` |
| Undo Step 4 | Reverse numbers | `25-08-08-18-22` |
| Undo Step 3 | Numbers → letters | `ARRHD` |
| Undo Step 2 | Reverse letters | `DHRRA` |
| Undo Step 1 | Clockface swap | `QUEEN` |

---

## The time key

Every encryption and decryption requires a **time key** — a time written as `H:MM` or `HH:MM`, for example `3:20` or `11:45`.

- The hour must be between **1 and 12**
- The minute must be written with **two digits** (e.g. `05`, not `5`)

Valid examples: `1:00`, `3:20`, `12:59`  
Invalid examples: `0:30` (hour 0 not allowed), `3:5` (minute needs two digits), `13:00` (hour too large)

> In v2 (default), the time key drives independent ring rotations that change the output. In v1, the key is validated but does not affect the substitution.

---

## Doing it by hand

You don't need a computer. Here's how to encrypt with pen and paper:

1. Write your message in **capital letters only**. Remove spaces, numbers, and punctuation.
2. For each letter, find its swap partner using the clockface table above. (`M` and `Z` stay the same.)
3. **Reverse** the resulting letters.
4. Look up each letter in the number table above and write down the two-digit number.
5. **Reverse** the list of numbers and join them with dashes.

To decrypt, do the same steps in reverse order.

---

## Installing and building

You need two things installed on your computer:

- **CMake** (version 3.20 or newer) — a build tool
- A **C++ compiler** that supports C++20 — such as GCC 13 or Clang 18

If you're on macOS, both can be installed via [Homebrew](https://brew.sh):
```bash
brew install cmake gcc
```

On Ubuntu/Debian Linux:
```bash
sudo apt install cmake g++
```

### Build steps

Open a terminal, navigate to this folder, and run:

```bash
cmake -S . -B build
cmake --build build
```

This creates three programs inside the `build/` folder:
- `mcrc` — the main encryption/decryption tool
- `mcrc_demo` — a step-by-step demonstration with a visual clock
- `mcrc_verify` — a self-test that checks everything is working

To confirm everything works:
```bash
./build/mcrc_verify
```

You should see:
```
mcrc_verify: all checks passed
```

---

## Using the `mcrc` tool

### Encrypt a message

```bash
./build/mcrc encrypt --key 3:20 --text QUEEN
```
Output:
```
22-18-08-08-25
```

### Decrypt a message

```bash
./build/mcrc decrypt --key 3:20 --cipher 22-18-08-08-25
```
Output:
```
QUEEN
```

### Spaces and punctuation

By default, spaces, numbers, and punctuation are **silently removed** before encryption:

```bash
./build/mcrc encrypt --key 3:20 --text "Queen of Hearts"
# Encrypts QUEENOFHEARTS (spaces and case ignored)
```

If you want the tool to **refuse** input that contains non-letters, add `--reject`:

```bash
./build/mcrc encrypt --key 3:20 --text "Queen of Hearts" --reject
# Error: InvalidPlaintext: non-letter character in plaintext under Reject policy
```

### What the exit codes mean

If something goes wrong, the tool exits with a number that tells you what happened:

| Exit code | Meaning |
|-----------|---------|
| `0` | Success (also returned by `--help` / `-h`) |
| `1` | Wrong usage (missing flags, unknown command) |
| `2` | Invalid time key |
| `3` | Invalid ciphertext |
| `4` | Invalid plaintext (only with `--reject`) |

To see usage at any time:

```bash
./build/mcrc --help
./build/mcrc -h
```

---

## Using the `mcrc_demo` tool

The demo tool shows you every step of the encryption process, including a visual clock face:

```bash
./build/mcrc_demo --key 3:20 --text QUEEN
```

Example output:
```
Clock face for 3:20 (H = hour hand, M = minute hand):
           12
       11      1
     L            O
  10                 2
  K                    P

9   J        +       Q   3
            M Z
   I                    R
    8                  4
       H            S
         7        5
              G T
                6

Input (normalised):       QUEEN
Stage 1 (clockface):      DHRRA
Stage 2 (mirror):         ARRHD
Stage 3 (reverse num):    25-08-08-18-22
Stage 4 (mirror):         22-18-08-08-25
```

To hide the clock and show only the pipeline steps:

```bash
./build/mcrc_demo --key 3:20 --text QUEEN --no-clock
```

---

## Error messages explained

| Message | What it means | How to fix it |
|---------|--------------|---------------|
| `InvalidTimeKey: hour out of range [1, 12]` | The hour in your key is 0 or greater than 12 | Use a value between 1 and 12 |
| `InvalidTimeKey: minute must be exactly 2 digits` | You wrote `3:5` instead of `3:05` | Always use two digits for minutes |
| `InvalidTimeKey: missing ':' separator` | Your key doesn't have a colon | Write it as `3:20`, not `320` |
| `InvalidCiphertext: code group exceeds 25` | The ciphertext contains a number above 25 | Check you copied the ciphertext correctly |
| `InvalidCiphertext: truncated two-digit group` | The ciphertext is cut off | Make sure the full ciphertext is present |
| `InvalidPlaintext: non-letter character...` | Your text has spaces or punctuation and you used `--reject` | Remove non-letters, or drop `--reject` |

---

## Project layout

```
mcrc/
├── include/mcrc/
│   ├── clockface.hpp        Step 1 — clockface substitution
│   ├── mirror.hpp           Steps 2 & 4 — reversal
│   ├── reverse_cipher.hpp   Step 3 — letter-to-number encoding
│   ├── pipeline.hpp         Full encrypt / decrypt functions
│   └── exceptions.hpp       Error types
│
├── src/
│   ├── clockface.cpp
│   ├── mirror.cpp
│   ├── pipeline.cpp
│   └── reverse_cipher.cpp
│
├── apps/
│   ├── cli.cpp       →  mcrc (the main tool)
│   └── demo.cpp      →  mcrc_demo (the visual demo)
│
├── tests/
│   └── verify.cpp    →  mcrc_verify (self-test suite)
│
└── CMakeLists.txt    Build configuration
```

---

## Running the self-tests

The verification suite checks every part of the cipher automatically:

```bash
cmake --build build --target verify
```

It tests:
- Every letter in the clockface swap table
- The mirror reversal (both letters and numbers)
- The reverse cipher encoding and decoding
- Time key validation (valid and invalid formats)
- Ciphertext parsing (valid and malformed inputs)
- The known-answer test: `QUEEN` with key `3:20` → `22-18-08-08-25`
- 100 randomly generated messages, each encrypted and then decrypted to confirm the original is recovered

---

## Frequently asked questions

**Can I use any word?**  
Yes. Letters only — spaces and punctuation are stripped automatically (or rejected if you use `--reject`). The output is always uppercase.

**Does the time key change the result?**  
In v2 (default), yes — the hour and minute drive independent ring rotations, so different keys produce different ciphertext. In v1, the key is validated but does not affect output.

**Can I encrypt numbers or emoji?**  
No. MCRC only works with the 26 letters of the English alphabet. Everything else is either removed or causes an error.

**Is this secure?**  
No. MCRC is a teaching cipher. It uses fixed, publicly known substitution tables and is vulnerable to simple analysis. Never use it to protect real secrets.

**What if I lose the time key?**  
In v2, you need the exact key to decrypt. In v1, any valid key works (the key doesn't affect output).

---

## Algorithm versions

| Version | Clockface behaviour | Time key role |
|---------|-------------------|---------------|
| **v1** | Pure ROT13 — each letter swaps with the letter at the same clock position on the opposite ring. The substitution is identical for every time key. | Validated (`H:MM`, hour 1–12, minute 0–59) but **does not affect output**. |
| **v2** (default) | Dual-rotation — inner letters shift forward, outer letters shift forward (see rules below). Different keys produce different ciphertext. Not an involution (encode ≠ decode). | `hour_offset = H mod 12`, `minute_offset = MM / 5`. Both actively mixed into the substitution. |

Use `--version v1` or `--version v2` on the CLI to select. Default is v2.

### Why A↔N, B↔O, … specifically?

The inner ring (A–L) and outer ring (N–Y) are positioned so that each inner letter is exactly 13 apart from its outer partner — standard ROT13. This is the **design anchor**: at key `12:00` (both offsets zero), v2's shift-based rule collapses to v1's pure ROT13 swap. The ring layout guarantees backward compatibility by construction.

### v2 hand-encoding rules

Compute two offsets from the time key:
- `hour_offset = H mod 12` (e.g. `3:20` → 3; `12:00` → 0)
- `minute_offset = MM / 5` (integer division, e.g. `3:20` → 4; `12:00` → 0)

Then for each letter:

| Input letter's ring | Rule | Formula |
|---|---|---|
| **Inner** (A–L, position `p` = letter − 'A') | Shift forward by `hour_offset` onto the outer ring | output = outer letter at position `(p + hour_offset) mod 12`, i.e. `'N' + (p + hour_offset) % 12` |
| **Outer** (N–Y, position `p` = letter − 'N') | Shift forward by `minute_offset` onto the inner ring | output = inner letter at position `(p + minute_offset) mod 12`, i.e. `'A' + (p + minute_offset) % 12` |
| **Centre** (M or Z) | Fixed point | output = itself |

To **decode**, reverse the shifts: inner letters shift back by `minute_offset` to recover the outer original, outer letters shift back by `hour_offset` to recover the inner original.

### v2 worked example (`QUEEN`, key `3:20`)

Offsets: `hour_offset = 3`, `minute_offset = 4`

| Letter | Ring | Position | Shift | Output position | Output |
|--------|------|----------|-------|-----------------|--------|
| Q | outer | 3 | +4 (minute) | 7 | H |
| U | outer | 7 | +4 (minute) | 11 | L |
| E | inner | 4 | +3 (hour) | 7 | U |
| E | inner | 4 | +3 (hour) | 7 | U |
| N | outer | 0 | +4 (minute) | 4 | E |

```
QUEEN
 → HLUUE              (clockface v2)
 → EUULH              (first mirror)
 → 21-05-05-14-18     (reverse cipher)
 → 18-14-05-05-21     (second mirror — final ciphertext)
```

At key `12:00` (both offsets zero), every inner letter shifts by 0 onto the outer ring at the same position — which is exactly the v1 ROT13 swap.

---

## Note on the original specification document

The original class assignment document had `Q` incorrectly mapped to `E` (position 5) instead of `D` (position 4) in the clockface step, yielding a wrong intermediate. This has been corrected in the v1.0 implementation: `QUEEN → DHRRA`, final ciphertext `22-18-08-08-25`.
