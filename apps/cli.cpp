#include "mcrc/exceptions.hpp"
#include "mcrc/pipeline.hpp"

#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

void print_usage(std::ostream& os) {
    os << "Usage:\n"
       << "  mcrc encrypt --key <H:MM> --text <PLAINTEXT> [--reject]\n"
       << "  mcrc decrypt --key <H:MM> --cipher <NN-NN-...>\n"
       << "\nExamples:\n"
       << "  mcrc encrypt --key 3:20 --text QUEEN\n"
       << "  mcrc decrypt --key 3:20 --cipher 22-18-08-08-25\n";
}

struct Args {
    std::string_view command;
    std::optional<std::string_view> key;
    std::optional<std::string_view> text;
    std::optional<std::string_view> cipher;
    bool reject = false;
};

std::optional<Args> parse_args(int argc, char** argv) {
    if (argc < 2) {
        return std::nullopt;
    }
    // Handle top-level -h / --help before treating argv[1] as a subcommand.
    if (std::string_view(argv[1]) == "-h" || std::string_view(argv[1]) == "--help") {
        return std::nullopt;
    }
    Args a;
    a.command = argv[1];
    for (int i = 2; i < argc; ++i) {
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
        } else if (flag == "--cipher") {
            a.cipher = need_value("--cipher");
        } else if (flag == "--reject") {
            a.reject = true;
        } else if (flag == "-h" || flag == "--help") {
            return std::nullopt;
        } else {
            throw std::runtime_error("unknown flag: " + std::string(flag));
        }
    }
    return a;
}

} // namespace

int main(int argc, char** argv) {
    try {
        auto parsed = parse_args(argc, argv);
        if (!parsed) {
            print_usage(std::cout);
            return argc < 2 ? 1 : 0;
        }
        const Args& a = *parsed;

        if (a.command == "encrypt") {
            if (!a.key || !a.text) {
                print_usage(std::cerr);
                return 1;
            }
            const auto policy = a.reject ? mcrc::NonLetterPolicy::Reject
                                         : mcrc::NonLetterPolicy::Strip;
            std::cout << mcrc::encrypt(*a.text, *a.key, policy) << '\n';
            return 0;
        }
        if (a.command == "decrypt") {
            if (!a.key || !a.cipher) {
                print_usage(std::cerr);
                return 1;
            }
            std::cout << mcrc::decrypt(*a.cipher, *a.key) << '\n';
            return 0;
        }
        print_usage(std::cerr);
        return 1;
    } catch (const mcrc::InvalidTimeKey& e) {
        std::cerr << e.what() << '\n';
        return 2;
    } catch (const mcrc::InvalidCiphertext& e) {
        std::cerr << e.what() << '\n';
        return 3;
    } catch (const mcrc::InvalidPlaintext& e) {
        std::cerr << e.what() << '\n';
        return 4;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
