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
/// Legacy overload retained for direct use on code vectors. The stage-4
/// permutation in the current pipeline is `swap_pairs`, not this full
/// reversal — kept for callers and tests that want a plain reverse.
///
/// @param pairs Sequence of values in the range [0, 25].
/// @return A new vector containing `pairs` in reverse order.
std::vector<std::uint8_t> mirror(const std::vector<std::uint8_t>& pairs);

/// Stage 4 permutation: swap each adjacent pair `(a, b) -> (b, a)`.
///
/// Operates on the code vector produced by the reverse-cipher layer. If
/// the length is odd, the final element stays in place. Self-inverse:
/// `swap_pairs(swap_pairs(v)) == v`, so decrypt reuses it unchanged.
///
/// @param codes Sequence of values in the range [0, 25].
/// @return A new vector with each adjacent pair swapped.
std::vector<std::uint8_t> swap_pairs(const std::vector<std::uint8_t>& codes);

} // namespace mcrc
