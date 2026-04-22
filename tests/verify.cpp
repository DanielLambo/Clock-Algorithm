// MCRC verification program. Every check is an `assert()`. Build with
// NDEBUG *off* so assertions actually fire. Returns 0 on success.

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "mcrc/clockface.hpp"
#include "mcrc/exceptions.hpp"
#include "mcrc/mirror.hpp"
#include "mcrc/pipeline.hpp"
#include "mcrc/reverse_cipher.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace {

// ===========================================================================
// V1 tests (unchanged — backward compatibility)
// ===========================================================================

void verify_reverse_cipher_table() {
    assert(mcrc::kReverseCipherTable[0] == 25);   // A
    assert(mcrc::kReverseCipherTable[1] == 24);   // B
    assert(mcrc::kReverseCipherTable[4] == 21);   // E
    assert(mcrc::kReverseCipherTable[13] == 12);  // N
    assert(mcrc::kReverseCipherTable[17] == 8);   // R
    assert(mcrc::kReverseCipherTable[25] == 0);   // Z
}

void verify_mirror_layer() {
    assert(mcrc::mirror(std::string_view("ABCDE")) == "EDCBA");
    assert(mcrc::mirror(std::string_view("")) == "");
    assert(mcrc::mirror(std::string_view("X")) == "X");

    const std::vector<std::uint8_t> codes{1, 2, 3, 4};
    const std::vector<std::uint8_t> expected{4, 3, 2, 1};
    assert(mcrc::mirror(codes) == expected);

    // Mirror is an involution.
    const std::string_view s = "THEQUICKBROWNFOX";
    assert(mcrc::mirror(mcrc::mirror(s)) == std::string(s));
}

void verify_swap_pairs_layer() {
    // Even length: every adjacent pair swaps.
    assert(mcrc::swap_pairs({1, 2, 3, 4}) == (std::vector<std::uint8_t>{2, 1, 4, 3}));
    // Odd length: lone last element stays in place.
    assert(mcrc::swap_pairs({21, 5, 5, 14, 18}) == (std::vector<std::uint8_t>{5, 21, 14, 5, 18}));
    // Edge cases.
    assert(mcrc::swap_pairs({}) == (std::vector<std::uint8_t>{}));
    assert(mcrc::swap_pairs({7}) == (std::vector<std::uint8_t>{7}));
    // Self-inverse property: swap_pairs(swap_pairs(v)) == v.
    const std::vector<std::uint8_t> v{25, 8, 8, 18, 22, 13, 4};
    assert(mcrc::swap_pairs(mcrc::swap_pairs(v)) == v);
}

void verify_reverse_cipher_roundtrip() {
    const std::string letters = "QUEEN";
    const auto codes = mcrc::reverse_cipher_encode(letters);
    const std::vector<std::uint8_t> expected{
        25 - ('Q' - 'A'),
        25 - ('U' - 'A'),
        25 - ('E' - 'A'),
        25 - ('E' - 'A'),
        25 - ('N' - 'A'),
    };
    assert(codes == expected);
    assert(mcrc::reverse_cipher_decode(codes) == letters);

    assert(mcrc::format_codes(codes) == "09-05-21-21-12");
    assert(mcrc::parse_codes("09-05-21-21-12") == codes);
    assert(mcrc::parse_codes("") == std::vector<std::uint8_t>{});

    // P7 — Reverse Cipher Code Range: every code must be in [0, 25].
    const auto all_codes = mcrc::reverse_cipher_encode("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    assert(all_codes.size() == 26);
    for (std::uint8_t v : all_codes) {
        assert(v <= 25);
    }

    // P9 — format_codes output length: non-empty → size() == n*3-1; empty → "".
    assert(mcrc::format_codes({}).empty());
    const std::string formatted = mcrc::format_codes(all_codes);
    assert(formatted.size() == all_codes.size() * 3 - 1);
}

void verify_clockface_v1_layer() {
    const auto key = mcrc::parse_time_key("3:20");

    // ROT13 pairs at matching clock positions, M and Z fixed.
    assert(mcrc::clockface_encode("A", key) == "N");
    assert(mcrc::clockface_encode("N", key) == "A");
    assert(mcrc::clockface_encode("L", key) == "Y");
    assert(mcrc::clockface_encode("Y", key) == "L");
    assert(mcrc::clockface_encode("M", key) == "M");
    assert(mcrc::clockface_encode("Z", key) == "Z");
    assert(mcrc::clockface_encode("QUEEN", key) == "DHRRA");

    // Involution.
    const std::string cipher = mcrc::clockface_encode("HELLOWORLD", key);
    assert(mcrc::clockface_decode(cipher, key) == "HELLOWORLD");

    // Time key is not mixed in for v1 — any valid key yields same output.
    const auto key_b = mcrc::parse_time_key("11:59");
    assert(mcrc::clockface_encode("QUEEN", key_b) == "DHRRA");
}

void verify_time_key_parsing() {
    assert(mcrc::parse_time_key("3:20").hour == 3);
    assert(mcrc::parse_time_key("3:20").minute == 20);
    assert(mcrc::parse_time_key("12:00").hour == 12);
    assert(mcrc::parse_time_key("01:05").hour == 1);
    assert(mcrc::parse_time_key("01:05").minute == 5);

    auto throws_time = [](const char* s) {
        try {
            mcrc::parse_time_key(s);
        } catch (const mcrc::InvalidTimeKey&) {
            return true;
        } catch (...) {
            return false;
        }
        return false;
    };
    assert(throws_time(""));
    assert(throws_time("3-20"));
    assert(throws_time("0:20"));
    assert(throws_time("13:00"));
    assert(throws_time("3:60"));
    assert(throws_time("3:5"));
    assert(throws_time("3:XX"));
    assert(throws_time("3:"));
    assert(throws_time(":20"));
}

void verify_ciphertext_parsing_errors() {
    auto throws_cipher = [](const char* s) {
        try {
            mcrc::parse_codes(s);
        } catch (const mcrc::InvalidCiphertext&) {
            return true;
        } catch (...) {
            return false;
        }
        return false;
    };
    assert(throws_cipher("99-00"));
    assert(throws_cipher("26"));
    assert(throws_cipher("1"));
    assert(throws_cipher("1A"));
    assert(throws_cipher("12-"));
    assert(throws_cipher("12--13"));
    assert(throws_cipher("12-3"));
}

void verify_v1_integration() {
    assert(mcrc::encrypt("QUEEN", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V1) == "08-25-18-08-22");
    assert(mcrc::decrypt("08-25-18-08-22", "3:20", mcrc::Version::V1) == "QUEEN");

    // Normalisation: mixed case and Strip policy.
    assert(mcrc::encrypt("Queen!", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V1) == "08-25-18-08-22");

    // Reject policy throws on non-letters.
    bool threw = false;
    try {
        mcrc::encrypt("Queen!", "3:20", mcrc::NonLetterPolicy::Reject, mcrc::Version::V1);
    } catch (const mcrc::InvalidPlaintext&) {
        threw = true;
    }
    assert(threw);

    // Empty plaintext is a valid edge case.
    assert(mcrc::encrypt("", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V1) == "");
    assert(mcrc::decrypt("", "3:20", mcrc::Version::V1) == "");

    // P11 — Ciphertext group count equals plaintext length.
    {
        const std::string ct = mcrc::encrypt("QUEEN", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V1);
        auto count_groups = [](const std::string& s) -> std::size_t {
            if (s.empty()) return 0;
            std::size_t n = 1;
            for (char c : s) { if (c == '-') ++n; }
            return n;
        };
        assert(count_groups(ct) == 5);
        assert(count_groups(mcrc::encrypt("", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V1)) == 0);
    }

    // P12 — Strip policy idempotence.
    assert(mcrc::encrypt("Queen!", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V1)
        == mcrc::encrypt("QUEEN", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V1));
}

void verify_v1_pipeline_trace() {
    const auto t = mcrc::encrypt_trace("QUEEN", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V1);
    assert(t.normalised           == "QUEEN");
    assert(t.after_clockface      == "DHRRA");
    assert(t.after_first_mirror   == "ARRHD");
    assert(t.after_reverse_cipher == "25-08-08-18-22");
    assert(t.after_swap_pairs  == "08-25-18-08-22");

    assert(t.after_swap_pairs == mcrc::encrypt("QUEEN", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V1));

    bool threw = false;
    try {
        mcrc::encrypt_trace("QUEEN", "0:00", mcrc::NonLetterPolicy::Strip, mcrc::Version::V1);
    } catch (const mcrc::InvalidTimeKey&) {
        threw = true;
    }
    assert(threw);
}

void verify_v1_random_roundtrip() {
    std::mt19937 rng(0xC10CCABEu);
    std::uniform_int_distribution<int> len_dist(1, 40);
    std::uniform_int_distribution<int> letter_dist('A', 'Z');
    std::uniform_int_distribution<int> hour_dist(1, 12);
    std::uniform_int_distribution<int> minute_dist(0, 59);

    for (int iter = 0; iter < 100; ++iter) {
        const int len = len_dist(rng);
        std::string plaintext;
        plaintext.reserve(static_cast<std::size_t>(len));
        for (int i = 0; i < len; ++i) {
            plaintext.push_back(static_cast<char>(letter_dist(rng)));
        }

        const int hour = hour_dist(rng);
        const int minute = minute_dist(rng);
        char key_buf[8];
        std::snprintf(key_buf, sizeof(key_buf), "%d:%02d", hour, minute);

        const std::string cipher = mcrc::encrypt(plaintext, key_buf, mcrc::NonLetterPolicy::Strip, mcrc::Version::V1);
        const std::string recovered = mcrc::decrypt(cipher, key_buf, mcrc::Version::V1);
        assert(recovered == plaintext);
    }
}

// ===========================================================================
// V2 tests (dual-rotation clockface)
// ===========================================================================

void verify_clockface_v2_layer() {
    const auto key = mcrc::parse_time_key("3:20");
    // hour_offset = 3 % 12 = 3, minute_offset = 20 / 5 = 4

    // M and Z are centre specials — fixed points (same as v1).
    assert(mcrc::clockface_v2_encode("M", key) == "M");
    assert(mcrc::clockface_v2_encode("Z", key) == "Z");

    // Inner letter E (pos 4): 'N' + (4+3)%12 = 'N'+7 = U
    assert(mcrc::clockface_v2_encode("E", key) == "U");
    // Outer letter Q (pos 3): 'A' + (3+4)%12 = 'A'+7 = H
    assert(mcrc::clockface_v2_encode("Q", key) == "H");
    // Outer letter U (pos 7): 'A' + (7+4)%12 = 'A'+11 = L
    assert(mcrc::clockface_v2_encode("U", key) == "L");
    // Outer letter N (pos 0): 'A' + (0+4)%12 = 'A'+4 = E
    assert(mcrc::clockface_v2_encode("N", key) == "E");

    // Full word.
    assert(mcrc::clockface_v2_encode("QUEEN", key) == "HLUUE");

    // Roundtrip: decode(encode(x)) == x (NOT an involution).
    const std::string cipher = mcrc::clockface_v2_encode("HELLOWORLD", key);
    assert(mcrc::clockface_v2_decode(cipher, key) == "HELLOWORLD");

    // Different key gives different output (key IS mixed in for v2).
    const auto key_b = mcrc::parse_time_key("11:59");
    assert(mcrc::clockface_v2_encode("QUEEN", key_b) != mcrc::clockface_v2_encode("QUEEN", key));
}

void verify_v2_rot13_fallback() {
    // Property 3: at key 12:00 (hour_offset=0, minute_offset=0), v2 == v1.
    const auto key = mcrc::parse_time_key("12:00");
    const std::string plain = "QUEEN";

    const std::string v1_result = mcrc::clockface_encode(plain, key);
    const std::string v2_result = mcrc::clockface_v2_encode(plain, key);
    assert(v1_result == v2_result);

    // Full pipeline at 12:00 must also match.
    assert(mcrc::encrypt(plain, "12:00", mcrc::NonLetterPolicy::Strip, mcrc::Version::V1)
        == mcrc::encrypt(plain, "12:00", mcrc::NonLetterPolicy::Strip, mcrc::Version::V2));

    // Check with more words.
    for (const char* word : {"HELLO", "WORLD", "MCRC", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"}) {
        assert(mcrc::clockface_v2_encode(word, key) == mcrc::clockface_encode(word, key));
    }
}

void verify_v2_key_sensitivity() {
    // Property 2: encrypt(p, k1) != encrypt(p, k2) for ≥90% of random pairs.
    std::mt19937 rng(0xCAFE0201u);
    std::uniform_int_distribution<int> len_dist(3, 20);
    std::uniform_int_distribution<int> letter_dist('A', 'Z');
    std::uniform_int_distribution<int> hour_dist(1, 12);
    std::uniform_int_distribution<int> minute_dist(0, 59);

    int trials = 200;
    int different = 0;

    for (int i = 0; i < trials; ++i) {
        const int len = len_dist(rng);
        std::string plaintext;
        for (int j = 0; j < len; ++j) {
            plaintext.push_back(static_cast<char>(letter_dist(rng)));
        }

        char k1_buf[8], k2_buf[8];
        int h1 = hour_dist(rng), m1 = minute_dist(rng);
        int h2 = hour_dist(rng), m2 = minute_dist(rng);
        // Ensure k1 != k2
        while (h1 == h2 && m1 == m2) {
            h2 = hour_dist(rng);
            m2 = minute_dist(rng);
        }
        std::snprintf(k1_buf, sizeof(k1_buf), "%d:%02d", h1, m1);
        std::snprintf(k2_buf, sizeof(k2_buf), "%d:%02d", h2, m2);

        const std::string c1 = mcrc::encrypt(plaintext, k1_buf, mcrc::NonLetterPolicy::Strip, mcrc::Version::V2);
        const std::string c2 = mcrc::encrypt(plaintext, k2_buf, mcrc::NonLetterPolicy::Strip, mcrc::Version::V2);
        if (c1 != c2) ++different;
    }

    // At least 90% should differ.
    assert(different >= trials * 9 / 10);
}

void verify_v2_distinct_key_count() {
    // Property 4: {encrypt("HELLO", k) for k in all 144 distinct rotation pairs} has ≥100 distinct values.
    std::set<std::string> outputs;

    for (int h = 1; h <= 12; ++h) {
        for (int m_slot = 0; m_slot < 12; ++m_slot) {
            char key_buf[8];
            std::snprintf(key_buf, sizeof(key_buf), "%d:%02d", h, m_slot * 5);
            outputs.insert(mcrc::encrypt("HELLO", key_buf, mcrc::NonLetterPolicy::Strip, mcrc::Version::V2));
        }
    }

    assert(outputs.size() >= 100);
}

void verify_v2_integration() {
    // Locked test vector: QUEEN with key 3:20 under v2.
    // Clockface: QUEEN → HLUUE
    // Mirror: HLUUE → EUULH
    // Reverse cipher: E=21, U=05, U=05, L=14, H=18 → "21-05-05-14-18"
    // Swap pairs:  (21,05)→(05,21), (05,14)→(14,05), lone 18 → "05-21-14-05-18"
    assert(mcrc::encrypt("QUEEN", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V2) == "05-21-14-05-18");
    assert(mcrc::decrypt("05-21-14-05-18", "3:20", mcrc::Version::V2) == "QUEEN");

    // Normalisation still works.
    assert(mcrc::encrypt("Queen!", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V2) == "05-21-14-05-18");

    // Empty plaintext.
    assert(mcrc::encrypt("", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V2) == "");
    assert(mcrc::decrypt("", "3:20", mcrc::Version::V2) == "");
}

void verify_v2_pipeline_trace() {
    const auto t = mcrc::encrypt_trace("QUEEN", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V2);
    assert(t.normalised           == "QUEEN");
    assert(t.after_clockface      == "HLUUE");
    assert(t.after_first_mirror   == "EUULH");
    assert(t.after_reverse_cipher == "21-05-05-14-18");
    assert(t.after_swap_pairs  == "05-21-14-05-18");

    assert(t.after_swap_pairs == mcrc::encrypt("QUEEN", "3:20", mcrc::NonLetterPolicy::Strip, mcrc::Version::V2));
}

void verify_v2_random_roundtrip() {
    // Property 1: decrypt(encrypt(p, k), k) == p for all p, k.
    std::mt19937 rng(0xBEEF0202u);
    std::uniform_int_distribution<int> len_dist(1, 40);
    std::uniform_int_distribution<int> letter_dist('A', 'Z');
    std::uniform_int_distribution<int> hour_dist(1, 12);
    std::uniform_int_distribution<int> minute_dist(0, 59);

    for (int iter = 0; iter < 200; ++iter) {
        const int len = len_dist(rng);
        std::string plaintext;
        plaintext.reserve(static_cast<std::size_t>(len));
        for (int i = 0; i < len; ++i) {
            plaintext.push_back(static_cast<char>(letter_dist(rng)));
        }

        const int hour = hour_dist(rng);
        const int minute = minute_dist(rng);
        char key_buf[8];
        std::snprintf(key_buf, sizeof(key_buf), "%d:%02d", hour, minute);

        const std::string cipher = mcrc::encrypt(plaintext, key_buf, mcrc::NonLetterPolicy::Strip, mcrc::Version::V2);
        const std::string recovered = mcrc::decrypt(cipher, key_buf, mcrc::Version::V2);
        assert(recovered == plaintext);
    }
}

} // namespace

int main() {
    // Shared layers (unchanged).
    verify_reverse_cipher_table();
    verify_mirror_layer();
    verify_swap_pairs_layer();
    verify_reverse_cipher_roundtrip();
    verify_time_key_parsing();
    verify_ciphertext_parsing_errors();

    // V1 suite.
    verify_clockface_v1_layer();
    verify_v1_integration();
    verify_v1_pipeline_trace();
    verify_v1_random_roundtrip();

    // V2 suite.
    verify_clockface_v2_layer();
    verify_v2_rot13_fallback();
    verify_v2_key_sensitivity();
    verify_v2_distinct_key_count();
    verify_v2_integration();
    verify_v2_pipeline_trace();
    verify_v2_random_roundtrip();

    std::cout << "mcrc_verify: all checks passed\n";
    return 0;
}
