#include "mcrc/clockface.hpp"
#include "mcrc/exceptions.hpp"

#include <array>
#include <cctype>
#include <string>

namespace mcrc {
namespace {

/// Precomputed ROT13-over-24-letters swap table.
///
/// Layout:
///   inner ring (clock 1..12): A B C D E F G H I J K L
///   outer ring (clock 1..12): N O P Q R S T U V W X Y
/// Letters at the same clock position swap with each other. `M` and
/// `Z` sit at the centre and map to themselves. The resulting table
/// is an involution, so encoding and decoding share it.
constexpr std::array<char, 26> make_swap_table() {
    std::array<char, 26> t{};
    for (int i = 0; i < 26; ++i) {
        char letter = static_cast<char>('A' + i);
        if (letter == 'M' || letter == 'Z') {
            t[static_cast<std::size_t>(i)] = letter;
        } else if (letter >= 'A' && letter <= 'L') {
            t[static_cast<std::size_t>(i)] = static_cast<char>(letter + 13);
        } else {
            // N..Y map to A..L (letter - 13)
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

} // namespace mcrc
