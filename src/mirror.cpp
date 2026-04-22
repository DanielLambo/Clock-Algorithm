#include "mcrc/mirror.hpp"

#include <algorithm>

namespace mcrc {

std::string mirror(std::string_view input) {
    std::string out(input.rbegin(), input.rend());
    return out;
}

std::vector<std::uint8_t> mirror(const std::vector<std::uint8_t>& pairs) {
    std::vector<std::uint8_t> out(pairs.rbegin(), pairs.rend());
    return out;
}

std::vector<std::uint8_t> swap_pairs(const std::vector<std::uint8_t>& codes) {
    std::vector<std::uint8_t> out = codes;
    for (std::size_t i = 0; i + 1 < out.size(); i += 2) {
        std::uint8_t tmp = out[i];
        out[i] = out[i + 1];
        out[i + 1] = tmp;
    }
    return out;
}

} // namespace mcrc
