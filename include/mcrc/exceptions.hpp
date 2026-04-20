#pragma once

#include <stdexcept>
#include <string>

namespace mcrc {

/// Thrown when a time key fails parse or range validation.
///
/// Accepted format is `H:MM` or `HH:MM` with hour in [1, 12] and
/// minute in [0, 59]. Any deviation (non-digits, missing colon,
/// out-of-range values) produces this exception.
class InvalidTimeKey : public std::runtime_error {
public:
    /// @param what Human-readable description of the validation failure.
    explicit InvalidTimeKey(const std::string& what)
        : std::runtime_error("InvalidTimeKey: " + what) {}
};

/// Thrown when ciphertext cannot be parsed as MCRC output.
///
/// MCRC ciphertext is a sequence of dash-separated two-digit groups,
/// each group in the range [00, 25]. Malformed groups, out-of-range
/// values, and stray characters all raise this exception.
class InvalidCiphertext : public std::runtime_error {
public:
    /// @param what Human-readable description of the parse failure.
    explicit InvalidCiphertext(const std::string& what)
        : std::runtime_error("InvalidCiphertext: " + what) {}
};

/// Thrown when plaintext contains disallowed characters under the
/// `reject` non-letter policy, or is otherwise unacceptable.
class InvalidPlaintext : public std::runtime_error {
public:
    /// @param what Human-readable description of the plaintext issue.
    explicit InvalidPlaintext(const std::string& what)
        : std::runtime_error("InvalidPlaintext: " + what) {}
};

} // namespace mcrc
