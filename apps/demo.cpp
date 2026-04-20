#include "mcrc/clockface.hpp"
#include "mcrc/exceptions.hpp"
#include "mcrc/pipeline.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

void print_usage(std::ostream& os) {
    os << "Usage: mcrc_demo --key <H:MM> --text <PLAINTEXT> [--no-clock] [--version v1|v2]\n";
}

struct DemoArgs {
    std::optional<std::string_view> key;
    std::optional<std::string_view> text;
    bool clock = true;
    mcrc::Version version = mcrc::Version::V2;
};

std::optional<DemoArgs> parse_args(int argc, char** argv) {
    DemoArgs a;
    for (int i = 1; i < argc; ++i) {
        std::string_view flag = argv[i];
        auto need_value = [&](const char* name) -> std::string_view {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string("missing value for ") + name);
            }
            return argv[++i];
        };
        if (flag == "--key") {
            a.key = need_value("--key");
        } else if (flag == "--text") {
            a.text = need_value("--text");
        } else if (flag == "--no-clock") {
            a.clock = false;
        } else if (flag == "--version") {
            auto val = need_value("--version");
            if (val == "v1") {
                a.version = mcrc::Version::V1;
            } else if (val == "v2") {
                a.version = mcrc::Version::V2;
            } else {
                throw std::runtime_error("--version must be v1 or v2");
            }
        } else if (flag == "-h" || flag == "--help") {
            return std::nullopt;
        } else {
            throw std::runtime_error("unknown flag: " + std::string(flag));
        }
    }
    if (!a.key || !a.text) {
        return std::nullopt;
    }
    return a;
}

/// Renders an ASCII analog clock with hour and minute hands marked.
///
/// Positions follow a standard clock face: 12 at the top, 3 on the
/// right, 6 at the bottom, 9 on the left.
void render_clock(const mcrc::TimeKey& key, std::ostream& os) {
    constexpr int rows = 11;
    constexpr int cols = 23;
    std::array<std::array<char, cols>, rows> grid{};
    for (auto& row : grid) {
        row.fill(' ');
    }

    const double cx = (cols - 1) / 2.0;
    const double cy = (rows - 1) / 2.0;
    const double radius_x = cx - 1;
    const double radius_y = cy - 1;

    auto plot = [&](int r, int c, char ch) {
        if (r >= 0 && r < rows && c >= 0 && c < cols) {
            grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = ch;
        }
    };

    // Hour-number labels around the rim.
    for (int h = 1; h <= 12; ++h) {
        const double theta = (h * 30.0 - 90.0) * M_PI / 180.0;
        const int r = static_cast<int>(std::round(cy + radius_y * std::sin(theta)));
        const int c = static_cast<int>(std::round(cx + radius_x * std::cos(theta)));
        if (h >= 10) {
            plot(r, c - 1, static_cast<char>('0' + (h / 10)));
            plot(r, c,     static_cast<char>('0' + (h % 10)));
        } else {
            plot(r, c, static_cast<char>('0' + h));
        }
    }

    // Hour hand: `H` marker near the rim at the hour-hand tip.
    {
        const double theta = (key.hour * 30.0 - 90.0) * M_PI / 180.0;
        const int r = static_cast<int>(std::round(cy + (radius_y - 2) * std::sin(theta)));
        const int c = static_cast<int>(std::round(cx + (radius_x - 3) * std::cos(theta)));
        plot(r, c, 'H');
    }

    // Minute hand: `M` marker further out.
    {
        const double theta = (key.minute * 6.0 - 90.0) * M_PI / 180.0;
        const int r = static_cast<int>(std::round(cy + (radius_y - 1) * std::sin(theta)));
        const int c = static_cast<int>(std::round(cx + (radius_x - 1) * std::cos(theta)));
        plot(r, c, 'M');
    }

    plot(static_cast<int>(cy), static_cast<int>(cx), '+');

    for (const auto& row : grid) {
        os.write(row.data(), cols);
        os << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        auto parsed = parse_args(argc, argv);
        if (!parsed) {
            print_usage(std::cerr);
            return 1;
        }
        const DemoArgs& a = *parsed;

        const auto trace = mcrc::encrypt_trace(*a.text, *a.key,
                                               mcrc::NonLetterPolicy::Strip, a.version);

        if (a.clock) {
            const auto key = mcrc::parse_time_key(*a.key);
            std::cout << "Clock face for " << *a.key
                      << " (H = hour hand, M = minute hand):\n";
            render_clock(key, std::cout);
            std::cout << '\n';
        }

        std::cout << "Input (normalised):       " << trace.normalised << '\n';
        std::cout << "Stage 1 (clockface):      " << trace.after_clockface << '\n';
        std::cout << "Stage 2 (mirror):         " << trace.after_first_mirror << '\n';
        std::cout << "Stage 3 (reverse num):    " << trace.after_reverse_cipher << '\n';
        std::cout << "Stage 4 (mirror):         " << trace.after_second_mirror << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
