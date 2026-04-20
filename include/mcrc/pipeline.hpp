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
std::string encrypt(std::string_view plaintext,
                    std::string_view time_key,
                    NonLetterPolicy policy = NonLetterPolicy::Strip,
                    Version version = Version::V2);

/// Inverts `encrypt`.
std::string decrypt(std::string_view ciphertext,
                    std::string_view time_key,
                    Version version = Version::V2);

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
                            NonLetterPolicy policy = NonLetterPolicy::Strip,
                            Version version = Version::V2);

} // namespace mcrc
