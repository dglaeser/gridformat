// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <unordered_map>
#include <filesystem>
#include <type_traits>
#include <iterator>
#include <iostream>
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
        throw std::runtime_error("Could not parse option from string '" + key_value_pair + "'");
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
            throw std::runtime_error("Format does not take any options");
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
        if (fmt == "vtu")
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
        return std::vector<std::string>{"vtu", "vti", "vtr", "vts", "vtk-hdf"};
    }
};

template<GridFormat::Concepts::Communicator Communicator>
void convert_file(const std::string& in_filename,
                  const std::string& out_fmt,
                  const Communicator& c,
                  std::vector<std::string> opts) {
    std::filesystem::path in_path{in_filename};
    if (!std::filesystem::exists(in_path))
        throw std::runtime_error("Given file '" + in_filename + "' does not exist.");

    std::string out_filename = (in_path.parent_path() / in_path.stem()).string() + "_converted";
    if (auto it = std::ranges::find(opts, "-o"); it != opts.end()) {
        auto out_name_it = it;
        if (++out_name_it; out_name_it == std::ranges::end(opts))
            throw std::runtime_error("No filename given after '-o'");
        out_filename = *out_name_it;
        ++out_name_it;
        opts.erase(it, out_name_it);
    }
    if (auto it = std::ranges::find(opts, "-o"); it != opts.end())
        throw std::runtime_error("Option '-o' given multiple times");

    auto options_map = make_options_map(opts | std::views::all);
    FormatSelector::with(out_fmt, [&] <typename Format> (Format fmt) {
        auto fmt_opts = OptionsParser<Format>::parse(options_map);
        if constexpr (ExposesOptions<Format>)
            fmt.opts = fmt_opts;

        if (std::is_same_v<Communicator, GridFormat::NullCommunicator> || GridFormat::Parallel::size(c) == 1)
            GridFormat::convert(in_filename, out_filename, fmt);
        else
            GridFormat::convert(in_filename, out_filename, fmt, c);
    });
}

void print_help() {
    std::cout << "usage: gridformat-convert FILE TARGET_FORMAT [TARGET_FORMAT_OPTIONS] [-o OUT_FILENAME]\n" << std::endl;
    std::cout << "'TARGET_FORMAT' can be any of: {"
              << GridFormat::as_string(FormatSelector::supported_formats(), ", ")
              << "}" << std::endl
              << "'TARGET_FORMAT_OPTIONS' are pairs of 'key=value'. "
              << "Use gridformat-convert --help-TARGET_FORMAT for more info." << std::endl
              << "'OUT_FILENAME' defaults to FILE_WITHOUT_EXTENSION_converted.NEW_EXTENSION" << std::endl;
}

void print_format_help(const std::string& format) {
    FormatSelector::with(format, [&] <typename Fmt> (const Fmt&) {
        const auto all_opts = OptionsParser<Fmt>::get_all_opts();
        if (all_opts.empty()) {
            std::cout << "Format '" << format << "' takes no options" << std::endl;
        } else {
            std::cout << "Format '" << format << "' accepts the following options:" << std::endl;
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
