#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace mcrc {

/// Algorithm version selector.
enum class Version { V1, V2 };

/// A validated MCRC time key in the form `H:MM` / `HH:MM`.
///
/// Hours are in [1, 12] and minutes in [0, 59]. The time key is
/// carried through the pipeline and validated at each entry point;
/// in the v1 clockface it is not mixed into the substitution (see
/// README) but must still be well-formed.
struct TimeKey {
    std::uint8_t hour;   ///< 1-12 inclusive
    std::uint8_t minute; ///< 0-59 inclusive
};

/// Parses and validates a textual time key.
///
/// @param text Time key to parse (e.g. `"3:20"`, `"11:05"`).
/// @return The parsed `TimeKey`.
/// @throws InvalidTimeKey on any format or range violation.
TimeKey parse_time_key(std::string_view text);

/// V1 clockface substitution (pure ROT13, time key ignored).
std::string clockface_encode(std::string_view letters, const TimeKey& key);
std::string clockface_decode(std::string_view letters, const TimeKey& key);

/// V2 clockface substitution (dual-rotation driven by time key).
///
/// Inner ring letters shift by (hour mod 12) positions onto the outer ring.
/// Outer ring letters shift by (minute / 5) positions onto the inner ring.
/// M and Z are centre specials (map to themselves).
/// NOT an involution — encode and decode are distinct inverse operations.
/// At key 12:00 (both offsets zero), v2 collapses to v1 ROT13.
std::string clockface_v2_encode(std::string_view letters, const TimeKey& key);
std::string clockface_v2_decode(std::string_view letters, const TimeKey& key);

} // namespace mcrc
