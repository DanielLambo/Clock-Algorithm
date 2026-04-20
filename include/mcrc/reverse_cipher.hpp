#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mcrc {

/// Compile-time table mapping `A..Z` to their MCRC reverse-cipher
/// numeric codes. `A = 25`, `B = 24`, ..., `Z = 0`. Index the table
/// with a zero-based letter offset (`letter - 'A'`).
inline constexpr std::array<std::uint8_t, 26> kReverseCipherTable = [] {
    std::array<std::uint8_t, 26> t{};
    for (std::uint8_t i = 0; i < 26; ++i) {
        t[i] = static_cast<std::uint8_t>(25 - i);
    }
    return t;
}();

/// Encodes each uppercase letter of `letters` as its reverse-cipher
/// numeric value.
///
/// Input must contain only `A..Z`; the caller is responsible for
/// normalisation (the pipeline performs this before invocation).
///
/// @param letters Uppercase A\u2013Z string to encode.
/// @return A vector of codes in the range [0, 25], one per input letter.
/// @throws InvalidPlaintext if a non-letter character is encountered.
std::vector<std::uint8_t> reverse_cipher_encode(std::string_view letters);

/// Decodes a sequence of reverse-cipher numeric codes back to letters.
///
/// @param codes Values in the range [0, 25].
/// @return Uppercase letter string of length `codes.size()`.
/// @throws InvalidCiphertext if any code falls outside [0, 25].
std::string reverse_cipher_decode(const std::vector<std::uint8_t>& codes);

/// Formats a code vector as dash-joined zero-padded two-digit groups,
/// e.g. `{25, 8, 8, 18, 22}` \u2192 `"25-08-08-18-22"`.
std::string format_codes(const std::vector<std::uint8_t>& codes);

/// Parses an MCRC ciphertext string into its numeric codes.
///
/// @param ciphertext Dash-separated two-digit groups.
/// @return Vector of parsed codes.
/// @throws InvalidCiphertext on any malformed or out-of-range group.
std::vector<std::uint8_t> parse_codes(std::string_view ciphertext);

} // namespace mcrc
