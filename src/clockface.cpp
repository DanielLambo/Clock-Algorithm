#include "mcrc/clockface.hpp"
#include "mcrc/exceptions.hpp"

#include <array>
#include <cctype>
#include <string>

namespace mcrc {
namespace {

// ---------------------------------------------------------------------------
// V1: Pure ROT13 (compile-time table, key ignored)
// ---------------------------------------------------------------------------

constexpr std::array<char, 26> make_swap_table() {
    std::array<char, 26> t{};
    for (int i = 0; i < 26; ++i) {
        char letter = static_cast<char>('A' + i);
        if (letter == 'M' || letter == 'Z') {
            t[static_cast<std::size_t>(i)] = letter;
        } else if (letter >= 'A' && letter <= 'L') {
            t[static_cast<std::size_t>(i)] = static_cast<char>(letter + 13);
        } else {
            t[static_cast<std::size_t>(i)] = static_cast<char>(letter - 13);
        }
    }
    return t;
}

constexpr auto kSwapTable = make_swap_table();

std::string apply_swap(std::string_view letters) {
    std::string out;
    out.reserve(letters.size());
    for (char c : letters) {
        if (c < 'A' || c > 'Z') {
            throw InvalidPlaintext(
                "clockface layer received non-letter character");
        }
        out.push_back(kSwapTable[static_cast<std::size_t>(c - 'A')]);
    }
    return out;
}

// ---------------------------------------------------------------------------
// V2: Dual-rotation clockface (key-dependent, non-involutory)
//
// Encode: inner letter at pos p → outer at pos (p + hour_offset) % 12
//         outer letter at pos p → inner at pos (p + minute_offset) % 12
// Decode: inner letter at pos q (came from outer) → outer at pos (q - minute_offset + 12) % 12
//         outer letter at pos q (came from inner) → inner at pos (q - hour_offset + 12) % 12
//
// At key 12:00 (offsets both 0), this collapses to v1 ROT13.
// ---------------------------------------------------------------------------

std::string apply_v2_encode(std::string_view letters, const TimeKey& key) {
    const int h = key.hour % 12;
    const int m = key.minute / 5;

    std::string out;
    out.reserve(letters.size());

    for (char c : letters) {
        if (c < 'A' || c > 'Z') {
            throw InvalidPlaintext(
                "clockface layer received non-letter character");
        }
        if (c == 'M' || c == 'Z') { out.push_back(c); continue; }

        if (c >= 'A' && c <= 'L') {
            int p = c - 'A';
            out.push_back(static_cast<char>('N' + (p + h) % 12));
        } else {
            int p = c - 'N';
            out.push_back(static_cast<char>('A' + (p + m) % 12));
        }
    }
    return out;
}

std::string apply_v2_decode(std::string_view letters, const TimeKey& key) {
    const int h = key.hour % 12;
    const int m = key.minute / 5;

    std::string out;
    out.reserve(letters.size());

    for (char c : letters) {
        if (c < 'A' || c > 'Z') {
            throw InvalidPlaintext(
                "clockface layer received non-letter character");
        }
        if (c == 'M' || c == 'Z') { out.push_back(c); continue; }

        if (c >= 'A' && c <= 'L') {
            // Inner letter in ciphertext came from outer letter during encode.
            int q = c - 'A';
            out.push_back(static_cast<char>('N' + (q - m + 12) % 12));
        } else {
            // Outer letter in ciphertext came from inner letter during encode.
            int q = c - 'N';
            out.push_back(static_cast<char>('A' + (q - h + 12) % 12));
        }
    }
    return out;
}

} // namespace

TimeKey parse_time_key(std::string_view text) {
    if (text.empty()) {
        throw InvalidTimeKey("time key is empty");
    }

    const auto colon = text.find(':');
    if (colon == std::string_view::npos) {
        throw InvalidTimeKey("missing ':' separator");
    }

    std::string_view hour_part = text.substr(0, colon);
    std::string_view minute_part = text.substr(colon + 1);

    if (hour_part.empty() || hour_part.size() > 2) {
        throw InvalidTimeKey("hour must be 1 or 2 digits");
    }
    if (minute_part.size() != 2) {
        throw InvalidTimeKey("minute must be exactly 2 digits");
    }

    auto as_int = [](std::string_view s) {
        int v = 0;
        for (char c : s) {
            if (c < '0' || c > '9') {
                throw InvalidTimeKey("time key contains non-digit");
            }
            v = v * 10 + (c - '0');
        }
        return v;
    };

    int hour = as_int(hour_part);
    int minute = as_int(minute_part);

    if (hour < 1 || hour > 12) {
        throw InvalidTimeKey("hour out of range [1, 12]");
    }
    if (minute < 0 || minute > 59) {
        throw InvalidTimeKey("minute out of range [0, 59]");
    }

    return TimeKey{static_cast<std::uint8_t>(hour),
                   static_cast<std::uint8_t>(minute)};
}

std::string clockface_encode(std::string_view letters, const TimeKey& /*key*/) {
    return apply_swap(letters);
}

std::string clockface_decode(std::string_view letters, const TimeKey& /*key*/) {
    return apply_swap(letters);
}

std::string clockface_v2_encode(std::string_view letters, const TimeKey& key) {
    return apply_v2_encode(letters, key);
}

std::string clockface_v2_decode(std::string_view letters, const TimeKey& key) {
    return apply_v2_decode(letters, key);
}

} // namespace mcrc
