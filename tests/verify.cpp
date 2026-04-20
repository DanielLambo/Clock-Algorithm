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
#include <string>
#include <vector>

namespace {

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

void verify_clockface_layer() {
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

    // Time key is currently not mixed in - any valid key yields same output.
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

void verify_integration() {
    assert(mcrc::encrypt("QUEEN", "3:20") == "22-18-08-08-25");
    assert(mcrc::decrypt("22-18-08-08-25", "3:20") == "QUEEN");

    // Normalisation: mixed case and Strip policy.
    assert(mcrc::encrypt("Queen!", "3:20") == "22-18-08-08-25");

    // Reject policy throws on non-letters.
    bool threw = false;
    try {
        mcrc::encrypt("Queen!", "3:20", mcrc::NonLetterPolicy::Reject);
    } catch (const mcrc::InvalidPlaintext&) {
        threw = true;
    }
    assert(threw);

    // Empty plaintext is a valid edge case.
    assert(mcrc::encrypt("", "3:20") == "");
    assert(mcrc::decrypt("", "3:20") == "");

    // P11 — Ciphertext group count equals plaintext length.
    {
        const std::string ct = mcrc::encrypt("QUEEN", "3:20");
        auto count_groups = [](const std::string& s) -> std::size_t {
            if (s.empty()) return 0;
            std::size_t n = 1;
            for (char c : s) { if (c == '-') ++n; }
            return n;
        };
        assert(count_groups(ct) == 5); // length of "QUEEN"
        assert(count_groups(mcrc::encrypt("", "3:20")) == 0);
    }

    // P12 — Strip policy idempotence: pre-normalised input == mixed input.
    assert(mcrc::encrypt("Queen!", "3:20") == mcrc::encrypt("QUEEN", "3:20"));
}

void verify_pipeline_trace() {
    // Req 8.1 — encrypt_trace captures all five intermediate stages.
    const auto t = mcrc::encrypt_trace("QUEEN", "3:20");
    assert(t.normalised           == "QUEEN");
    assert(t.after_clockface      == "DHRRA");
    assert(t.after_first_mirror   == "ARRHD");
    assert(t.after_reverse_cipher == "25-08-08-18-22");
    assert(t.after_second_mirror  == "22-18-08-08-25");

    // Req 8.2 — after_second_mirror must equal encrypt() with the same args.
    assert(t.after_second_mirror == mcrc::encrypt("QUEEN", "3:20"));

    // Req 8.3 — malformed key throws InvalidTimeKey.
    bool threw = false;
    try {
        mcrc::encrypt_trace("QUEEN", "0:00");
    } catch (const mcrc::InvalidTimeKey&) {
        threw = true;
    }
    assert(threw);
}

void verify_random_roundtrip() {
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

        const std::string cipher = mcrc::encrypt(plaintext, key_buf);
        const std::string recovered = mcrc::decrypt(cipher, key_buf);
        assert(recovered == plaintext);
    }
}

} // namespace

int main() {
    verify_reverse_cipher_table();
    verify_mirror_layer();
    verify_reverse_cipher_roundtrip();
    verify_clockface_layer();
    verify_time_key_parsing();
    verify_ciphertext_parsing_errors();
    verify_integration();
    verify_pipeline_trace();
    verify_random_roundtrip();

    std::cout << "mcrc_verify: all checks passed\n";
    return 0;
}
