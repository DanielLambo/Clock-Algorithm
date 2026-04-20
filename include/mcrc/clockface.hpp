#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace mcrc {

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

/// Applies the MCRC clockface substitution (stage 1 / forward).
///
/// The clockface is a dual 12-position ring:
///   * Inner ring: A B C D E F G H I J K L
///   * Outer ring: N O P Q R S T U V W X Y
/// At each clock position the two letters are ROT13 partners and are
/// swapped. `M` and `Z` sit at the centre and map to themselves.
///
/// @param letters Uppercase A\u2013Z input.
/// @param key     Validated time key (unused in v1 but required).
/// @return Substituted string of the same length.
/// @throws InvalidPlaintext if any non-letter slips through.
std::string clockface_encode(std::string_view letters, const TimeKey& key);

/// Inverse of `clockface_encode`. Because the substitution is a pure
/// involution, `clockface_decode` is implemented by re-applying the
/// same swap table, but is exposed separately for symmetry.
std::string clockface_decode(std::string_view letters, const TimeKey& key);

} // namespace mcrc
