// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <unordered_map>
#include <filesystem>
#include <type_traits>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <optional>
#include <utility>
#include <ranges>

#if GRIDFORMAT_HAVE_MPI
#include <mpi.h>
#endif

#include <gridformat/common/string_conversion.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/common/ranges.hpp>

#include <gridformat/gridformat.hpp>

#include "common.hpp"

void throw_missing_dependency(const std::string& class_name, const std::string& dep_name) {
    throw std::runtime_error("'" + class_name + "' unavailable due to missing dependency: " + dep_name);
}

auto make_lzma_compressor() {
#if GRIDFORMAT_HAVE_LZMA
    return GridFormat::Compression::lzma;
#else
    throw_missing_dependency("lzma compressor", "liblzma");
    return GridFormat::none;
#endif
}

auto make_lz4_compressor() {
#if GRIDFORMAT_HAVE_LZ4
    return GridFormat::Compression::lz4;
#else
    throw_missing_dependency("lz4 compressor", "liblz4");
    return GridFormat::none;
#endif
}

auto make_zlib_compressor() {
#if GRIDFORMAT_HAVE_ZLIB
    return GridFormat::Compression::zlib;
#else
    throw_missing_dependency("zlib compressor", "zlib");
    return GridFormat::none;
#endif
}

using OptionsMap = std::unordered_map<std::string, std::string>;

auto split_key_and_value(const std::string& key_value_pair) {
    auto split_pos = key_value_pair.find("=");
    if (split_pos == std::string::npos || split_pos == key_value_pair.size() - 1)
        throw std::runtime_error("Could not parse option (in the form key=value) from string '" + key_value_pair + "'");
    return std::make_pair(key_value_pair.substr(0, split_pos), key_value_pair.substr(split_pos + 1));
}

template<std::ranges::view Opts>
OptionsMap make_options_map(Opts opts) {
    OptionsMap result;
    for (const std::string& option : opts) {
        const auto [key, value] = split_key_and_value(option);
        if (result.contains(key))
            throw std::runtime_error("Option " + key + " appears multiple times");
        result[key] = value;
    }
    return result;
}

void print_options(const std::vector<std::string>& opts) {
    std::ranges::for_each(opts, [] (const std::string& opt) {
        std::cout << " - " << opt << std::endl;
    });
}

void print_available_options(const std::vector<std::string>& opts) {
    std::cout << "Available options:" << std::endl;
    print_options(opts);
}

template<typename T>
concept ExposesOptions = requires { typename T::Options; };

template<typename Options = GridFormat::None>
struct OptsParser {
    static auto parse(const OptionsMap& opts) {
        if (GridFormat::Ranges::size(opts) != 0)
            throw std::runtime_error("Error: chosen format does not take any options");
        return GridFormat::none;
    }

    static std::vector<std::string> get_all_opts() {
        return {};
    }
};

template<>
struct OptsParser<GridFormat::VTK::XMLOptions> {
    static auto parse(const OptionsMap& opts) {
        GridFormat::VTK::XMLOptions result;
        for (const auto& [key, value] : opts) {
            if (key == "encoder") _set_encoder(value, result);
            else if (key == "compressor") _set_compressor(value, result);
            else if (key == "data-format") _set_data_format(value, result);
            else if (key == "coordinate-precision") _set_coord_prec(value, result);
            else if (key == "header-precision") _set_header_prec(value, result);
            else {
                std::cout << "Option '" << key << "' is not supported by vtk-xml formats" << std::endl;
                print_available_options(get_all_opts());
                throw std::runtime_error("Invalid option '" + key + "'");
            }
        }
        return result;
    }

    static std::vector<std::string> get_all_opts() {
        return {
            "encoder (ascii/base64/raw)",
            "compressor (zlib/lz4/lzma/none)",
            "data-format (inlined/appended)",
            "coordinate-precision (float32/float64)",
            "header-precision (uint32/uint64)"
        };
    }

 private:
    static void _throw_option_value_error(const std::string& key, const std::string& value) {
        std::cout << "Unsupported '" << key << "': " << value << std::endl;
        print_available_options(get_all_opts());
        throw std::runtime_error("Invalid '" + key + "' selected");
    }

    static void _set_encoder(const std::string& enc_str, auto& opts) {
        if (enc_str == "ascii") opts.encoder = GridFormat::Encoding::ascii;
        else if (enc_str == "base64") opts.encoder = GridFormat::Encoding::base64;
        else if (enc_str == "raw") opts.encoder = GridFormat::Encoding::raw;
        else _throw_option_value_error("encoder", enc_str);
    }

    static void _set_compressor(const std::string& comp_str, auto& opts) {
        if (comp_str == "zlib") opts.compressor = make_zlib_compressor();
        else if (comp_str == "lz4") opts.compressor = make_lz4_compressor();
        else if (comp_str == "lzma") opts.compressor = make_lzma_compressor();
        else if (comp_str == "none") opts.compressor = GridFormat::none;
        else _throw_option_value_error("compressor", comp_str);
    }

    static void _set_data_format(const std::string& format_str, auto& opts) {
        if (format_str == "inlined") opts.data_format = GridFormat::VTK::DataFormat::inlined;
        else if (format_str == "appended") opts.data_format = GridFormat::VTK::DataFormat::appended;
        else _throw_option_value_error("data-format", format_str);
    }

    static void _set_coord_prec(const std::string& prec_str, auto& opts) {
        if (prec_str == "float32") opts.coordinate_precision = GridFormat::float32;
        else if (prec_str == "float64") opts.coordinate_precision = GridFormat::float64;
        else _throw_option_value_error("coordinate-precision", prec_str);
    }

    static void _set_header_prec(const std::string& prec_str, auto& opts) {
        if (prec_str == "uint32") opts.header_precision = GridFormat::uint32;
        else if (prec_str == "uint64") opts.header_precision = GridFormat::uint64;
        else _throw_option_value_error("header-precision", prec_str);
    }
};

template<typename Format>
struct OptionsParserSelector : public std::type_identity<OptsParser<>> {};

template<typename Format> requires(ExposesOptions<Format>)
struct OptionsParserSelector<Format> : public std::type_identity<OptsParser<typename Format::Options>> {};

template<typename Format>
using OptionsParser = typename OptionsParserSelector<Format>::type;

struct FormatSelector {
    template<typename Action>
    static void with(const std::string& fmt, const Action& action) {
        if (fmt == "any")
            action(GridFormat::any);
        else if (fmt == "vtu")
            action(GridFormat::vtu);
        else if (fmt == "vti")
            action(GridFormat::vti);
        else if (fmt == "vtr")
            action(GridFormat::vtr);
        else if (fmt == "vts")
            action(GridFormat::vts);
        else if (fmt == "vtk-hdf")
#if GRIDFORMAT_HAVE_HIGH_FIVE
            action(GridFormat::vtk_hdf);
#else
            throw_missing_dependency("vtk-hdf", "HighFive");
#endif
        else
            throw std::runtime_error("Unknown format specifier: " + fmt);
    }

    static std::vector<std::string> supported_formats() {
        return std::vector<std::string>{"vtu", "vti", "vtr", "vts", "vtk-hdf", "any"};
    }
};

template<GridFormat::Concepts::Communicator Communicator>
void convert_file(std::string in_filename,
                  const std::string& out_fmt,
                  const Communicator& c,
                  std::vector<std::string> opts) {

    // (maybe) adjust input file name
    bool rank_specific_files = false;
    if (auto pos = in_filename.find("{RANK}"); pos != std::string::npos) {
        in_filename.replace(pos, 6, std::to_string(GridFormat::Parallel::rank(c)));
        rank_specific_files = true;
    } else if (auto pos = in_filename.find("{RANK:"); pos != std::string::npos) {
        auto width_pos = pos + 6;
        auto end = in_filename.find_first_of("}", width_pos);
        if (end == std::string::npos)
            throw std::runtime_error("Invalid rank placeholder");
        const auto width = GridFormat::from_string<int>(in_filename.substr(width_pos, end-width_pos));
        if (width <= 0)
            throw std::runtime_error("Invalid rank placeholder width");
        auto rank_str = GridFormat::as_string(GridFormat::Parallel::rank(c));
        const int replace_width = width - rank_str.size();
        rank_str.insert(0, (replace_width <= 0 ? 0 : replace_width), '0');
        in_filename.replace(pos, end + 1 - pos, rank_str);
        rank_specific_files = true;
    }

    std::filesystem::path in_path{in_filename};
    if (!std::filesystem::exists(in_path))
        throw std::runtime_error("Given file '" + in_filename + "' does not exist.");

    const auto parse_arg = [&] (const std::string& short_key, const std::string& long_key) -> std::optional<std::string> {
        const auto process = [&] (const std::string& key) -> std::optional<std::string> {
            if (auto it = std::ranges::find(opts, key); it != opts.end()) {
                auto value_it = it;
                if (++value_it; value_it == opts.end())
                    throw std::runtime_error("Missing value for option '" + key + "'.");
                std::string result = *value_it;
                opts.erase(it, ++value_it);
                return result;
            }
            return {};
        };
        if (auto result = process(short_key); result) return result;
        if (auto result = process(long_key); result) return result;
        return {};
    };
    const auto parse_single_arg = [&] (const std::string& short_key, const std::string& long_key) -> std::optional<std::string> {
        if (auto result = parse_arg(short_key, long_key); result) {
            if (std::ranges::find(opts, short_key) != opts.end() || std::ranges::find(opts, long_key) != opts.end())
                throw std::runtime_error("Option '" + short_key + " | " + long_key + "' given multiple times");
            return result;
        }
        return {};
    };

    std::string out_filename = (in_path.parent_path() / in_path.stem()).string() + "_converted";
    if (auto f = parse_single_arg("-o", "--out-filename"); f) out_filename = f.value();

    std::string in_fmt = "any";
    if (auto f = parse_single_arg("-i", "--input-format"); f) in_fmt = f.value();

    bool quiet = false;
    if (auto it = std::ranges::find(opts, "-q"); it != opts.end()) { opts.erase(it); quiet = true; }
    if (auto it = std::ranges::find(opts, "--quiet"); it != opts.end()) { opts.erase(it); quiet = true; }

    bool is_rank_0 = GridFormat::Parallel::rank(c) == 0;
    auto options_map = make_options_map(opts | std::views::all);
    FormatSelector::with(out_fmt, [&] <typename Format> (Format fmt) {
        FormatSelector::with(in_fmt, [&] <typename InFormat> (InFormat) {
            auto fmt_opts = OptionsParser<Format>::parse(options_map);
            if constexpr (ExposesOptions<Format>)
                fmt.opts = fmt_opts;

            const GridFormat::ConversionOptions<Format, InFormat> conversion_opts{
                .out_format = fmt,
                .verbosity = (
                    quiet ? 0 : (
                        is_rank_0 ? 2 : (
                            rank_specific_files ? 1 : 0
                        )
                    )
                )
            };

            const auto written_file = [&] () {
                if (std::is_same_v<Communicator, GridFormat::NullCommunicator> || GridFormat::Parallel::size(c) == 1)
                    return GridFormat::convert(in_filename, out_filename, conversion_opts);
                else
                    return GridFormat::convert(in_filename, out_filename, conversion_opts, c);
            } ();
        });
    });
}

void print_help() {
    const auto print_arg_line = [] (std::string arg, std::string description) {
        static constexpr std::size_t arg_width = 25;
        static constexpr std::size_t indentation = 25 + 4;
        std::cout << GridFormat::Apps::as_cell(std::move(arg), arg_width) << std::string(4, ' ');
        int i = 0;
        for (const auto& description_line : description | std::views::split('\n')) {
            std::cout << (i++ > 0 ? std::string(indentation, ' ') : std::string{})
                      << std::string{std::ranges::begin(description_line), std::ranges::end(description_line)}
                      << std::endl;
        }
        std::cout << std::endl;
    };

    std::cout << "usage: [mpirun -n NUM_RANKS] gridformat-convert FILE TARGET_FORMAT [TARGET_FORMAT_OPTIONS] "
              << "[-o | --out-filename OUT_FILENAME] [-q --quiet] [-i --input-format]" << std::endl;
    std::cout << std::endl;
    print_arg_line(
        "FILE",
        "The file to be converted. May contain '{RANK}', a placeholder that is substituted\n"
        "by the process rank and which allows you to read different files per process (e.g. to\n"
        "merge them into one parallel file). Use '{RANK:N}' in order to specify a fixed width\n"
        "that is filled with leading zeros. For instance: '{RANK:3}' will yield 001 on rank 0."
    );
    print_arg_line(
        "TARGET_FORMAT",
        "Specify the format into which to convert. Can be any of {"
        + GridFormat::as_string(FormatSelector::supported_formats(), ", ")
        + "}.\nNote: if 'any' is selected, gridformat will select a default format."
    );
    print_arg_line(
        "TARGET_FORMAT_OPTIONS",
        "Specify further options for the chosen TARGET_FORMAT as pairs of 'key=value'.\n"
        "Use 'gridformat-convert --help-TARGET_FORMAT for more info."
    );
    print_arg_line(
        "-o | --out-filename",
        "The name of the file to be written (without extension).\n"
        "Defaults to '${FILE*}_converted.NEW_EXTENSION'., where FILE* is the name of\n"
        "the given file without the extension."
    );
    print_arg_line("-q | --quiet", "Use this flag to suppress progress output.");
    print_arg_line(
        "-i | --input-format",
        "Specify the format of FILE. If unspecified, it is deduced from its extension.\n"
        "See 'TARGET_FORMAT' for the available format specifiers."
    );
    std::cout << "Important: input & output filenames cannot be the same since data is read/written lazily to reduce memory usage." << std::endl;
}

void print_format_help(const std::string& format) {
    FormatSelector::with(format, [&] <typename Fmt> (const Fmt&) {
        const auto all_opts = OptionsParser<Fmt>::get_all_opts();
        if (all_opts.empty()) {
            std::cout << "[gridformat-convert]: Format '" << format << "' takes no options" << std::endl;
        } else {
            std::cout << "[gridformat-convert]: Format '" << format << "' accepts the following options:" << std::endl;
            print_options(all_opts);
        }
    });
}

void run(int argc, char** argv, const auto& comm) {
    if (GridFormat::Apps::args_ask_for_help(argc, argv)) {
        print_help();
        return;
    }

    const std::string format_help_signal = "--help-";
    for (int i = 1; i < argc; ++i) {
        const std::string arg{argv[i]};
        if (arg.starts_with(format_help_signal)) {
            const auto format = arg.substr(format_help_signal.size());
            print_format_help(format);
            return;
        }
    }

    if (argc < 3) {
        print_help();
        throw std::runtime_error("Invalid number of arguments");
    }

    std::vector<std::string> options;
    std::ranges::copy(std::views::iota(3, argc) | std::views::transform([&] (int arg_i) {
        return std::string{argv[arg_i]};
    }), std::back_inserter(options));
    convert_file(argv[1], argv[2], comm, std::move(options));
}

int main(int argc, char** argv) {

#if GRIDFORMAT_HAVE_MPI
    MPI_Init(&argc, &argv);
    const auto comm = MPI_COMM_WORLD;
#else
    const GridFormat::NullCommunicator comm{};
#endif

    int ret_code = EXIT_SUCCESS;
    try {
        run(argc, argv, comm);
    } catch (const std::exception& e) {
        std::cout << GridFormat::as_error(e.what()) << std::endl;
        ret_code = EXIT_FAILURE;
    }

#if GRIDFORMAT_HAVE_MPI
    MPI_Finalize();
#endif

    return ret_code;
}
