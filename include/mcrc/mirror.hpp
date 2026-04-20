#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace mcrc {

/// Reverses the character order of a string.
///
/// This is the layer used for stage 2 of the MCRC pipeline. It is its
/// own inverse: `mirror(mirror(s)) == s`.
///
/// @param input Read-only view of the letters to reverse.
/// @return A newly owned string containing `input` in reverse order.
std::string mirror(std::string_view input);

/// Reverses the order of a sequence of numeric pairs.
///
/// Used for stage 4 of the MCRC pipeline, operating on the vector of
/// two-digit codes produced by the reverse-cipher layer.
///
/// @param pairs Sequence of values in the range [0, 25].
/// @return A new vector containing `pairs` in reverse order.
std::vector<std::uint8_t> mirror(const std::vector<std::uint8_t>& pairs);

} // namespace mcrc
