#include "mcrc/reverse_cipher.hpp"
#include "mcrc/exceptions.hpp"

#include <cctype>
#include <string>

namespace mcrc {

std::vector<std::uint8_t> reverse_cipher_encode(std::string_view letters) {
    std::vector<std::uint8_t> out;
    out.reserve(letters.size());
    for (char c : letters) {
        if (c < 'A' || c > 'Z') {
            throw InvalidPlaintext(
                "reverse_cipher_encode received non-letter character");
        }
        out.push_back(kReverseCipherTable[static_cast<std::size_t>(c - 'A')]);
    }
    return out;
}

std::string reverse_cipher_decode(const std::vector<std::uint8_t>& codes) {
    std::string out;
    out.reserve(codes.size());
    for (std::uint8_t code : codes) {
        if (code > 25) {
            throw InvalidCiphertext(
                "reverse-cipher code out of range [0, 25]");
        }
        out.push_back(static_cast<char>('A' + (25 - code)));
    }
    return out;
}

std::string format_codes(const std::vector<std::uint8_t>& codes) {
    std::string out;
    if (codes.empty()) {
        return out;
    }
    out.reserve(codes.size() * 3 - 1);
    for (std::size_t i = 0; i < codes.size(); ++i) {
        if (i > 0) {
            out.push_back('-');
        }
        std::uint8_t v = codes[i];
        out.push_back(static_cast<char>('0' + (v / 10)));
        out.push_back(static_cast<char>('0' + (v % 10)));
    }
    return out;
}

std::vector<std::uint8_t> parse_codes(std::string_view ciphertext) {
    if (ciphertext.empty()) {
        return {};
    }

    std::vector<std::uint8_t> out;
    std::size_t i = 0;
    while (i < ciphertext.size()) {
        if (i + 1 >= ciphertext.size()) {
            throw InvalidCiphertext("truncated two-digit group");
        }
        char c0 = ciphertext[i];
        char c1 = ciphertext[i + 1];
        if (c0 < '0' || c0 > '9' || c1 < '0' || c1 > '9') {
            throw InvalidCiphertext(
                "expected two decimal digits for code group");
        }
        int value = (c0 - '0') * 10 + (c1 - '0');
        if (value > 25) {
            throw InvalidCiphertext("code group exceeds 25");
        }
        out.push_back(static_cast<std::uint8_t>(value));
        i += 2;
        if (i == ciphertext.size()) {
            break;
        }
        if (ciphertext[i] != '-') {
            throw InvalidCiphertext(
                "expected '-' separator between code groups");
        }
        ++i;
        if (i == ciphertext.size()) {
            throw InvalidCiphertext("trailing '-' with no following group");
        }
    }
    return out;
}

} // namespace mcrc
