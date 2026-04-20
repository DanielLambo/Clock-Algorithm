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

} // namespace mcrc
