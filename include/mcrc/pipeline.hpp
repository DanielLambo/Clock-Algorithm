#pragma once

#include "mcrc/clockface.hpp"

#include <string>
#include <string_view>

namespace mcrc {

/// Policy for handling non-letter characters in plaintext input.
enum class NonLetterPolicy {
    Strip, ///< Silently drop any character outside A\u2013Z (post-uppercase).
    Reject ///< Throw `InvalidPlaintext` on any non-letter character.
};

/// Runs the full 4-stage MCRC pipeline.
///
/// Pipeline: clockface \u2192 mirror \u2192 reverse-cipher \u2192 mirror.
///
/// @param plaintext Input to encrypt. Case is normalised to upper.
/// @param time_key  Required validated key (`H:MM` / `HH:MM`).
/// @param policy    What to do with non-letters before stage 1.
/// @return MCRC ciphertext as `NN-NN-...-NN`.
/// @throws InvalidTimeKey   on malformed key.
/// @throws InvalidPlaintext under `Reject` on non-letter input.
std::string encrypt(std::string_view plaintext,
                    std::string_view time_key,
                    NonLetterPolicy policy = NonLetterPolicy::Strip);

/// Inverts `encrypt`. Runs the pipeline in reverse so that
/// `decrypt(encrypt(p, k), k) == normalised(p)`.
///
/// @param ciphertext MCRC ciphertext (`NN-NN-...-NN`).
/// @param time_key   Same key used for encryption.
/// @return Uppercase plaintext.
/// @throws InvalidTimeKey    on malformed key.
/// @throws InvalidCiphertext on malformed ciphertext.
std::string decrypt(std::string_view ciphertext, std::string_view time_key);

/// Bundle of intermediate pipeline states, for the demo executable.
struct PipelineTrace {
    std::string normalised;           ///< Input after uppercase + filter
    std::string after_clockface;      ///< Stage 1 output
    std::string after_first_mirror;   ///< Stage 2 output
    std::string after_reverse_cipher; ///< Stage 3 output (formatted codes)
    std::string after_second_mirror;  ///< Stage 4 output (final ciphertext)
};

/// Runs `encrypt` while capturing every intermediate stage.
PipelineTrace encrypt_trace(std::string_view plaintext,
                            std::string_view time_key,
                            NonLetterPolicy policy = NonLetterPolicy::Strip);

} // namespace mcrc
