#include "mcrc/pipeline.hpp"

#include "mcrc/clockface.hpp"
#include "mcrc/exceptions.hpp"
#include "mcrc/mirror.hpp"
#include "mcrc/reverse_cipher.hpp"

#include <cctype>
#include <string>

namespace mcrc {
namespace {

std::string normalise(std::string_view plaintext, NonLetterPolicy policy) {
    std::string out;
    out.reserve(plaintext.size());
    for (char c : plaintext) {
        char uc = static_cast<char>(
            std::toupper(static_cast<unsigned char>(c)));
        if (uc >= 'A' && uc <= 'Z') {
            out.push_back(uc);
        } else if (policy == NonLetterPolicy::Reject) {
            throw InvalidPlaintext(
                "non-letter character in plaintext under Reject policy");
        }
    }
    return out;
}

} // namespace

std::string encrypt(std::string_view plaintext,
                    std::string_view time_key,
                    NonLetterPolicy policy) {
    const TimeKey key = parse_time_key(time_key);
    const std::string normalised = normalise(plaintext, policy);

    const std::string stage1 = clockface_encode(normalised, key);
    const std::string stage2 = mirror(stage1);
    const std::vector<std::uint8_t> stage3 = reverse_cipher_encode(stage2);
    const std::vector<std::uint8_t> stage4 = mirror(stage3);

    return format_codes(stage4);
}

std::string decrypt(std::string_view ciphertext, std::string_view time_key) {
    const TimeKey key = parse_time_key(time_key);

    const std::vector<std::uint8_t> codes = parse_codes(ciphertext);
    const std::vector<std::uint8_t> un_stage4 = mirror(codes);
    const std::string un_stage3 = reverse_cipher_decode(un_stage4);
    const std::string un_stage2 = mirror(un_stage3);
    return clockface_decode(un_stage2, key);
}

PipelineTrace encrypt_trace(std::string_view plaintext,
                            std::string_view time_key,
                            NonLetterPolicy policy) {
    const TimeKey key = parse_time_key(time_key);

    PipelineTrace t;
    t.normalised = normalise(plaintext, policy);
    t.after_clockface = clockface_encode(t.normalised, key);
    t.after_first_mirror = mirror(t.after_clockface);
    const auto codes = reverse_cipher_encode(t.after_first_mirror);
    t.after_reverse_cipher = format_codes(codes);
    t.after_second_mirror = format_codes(mirror(codes));
    return t;
}

} // namespace mcrc
