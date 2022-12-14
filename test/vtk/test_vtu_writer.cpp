// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <cmath>
#include <string>

#include <gridformat/common/logging.hpp>
#include <gridformat/encoding/base64.hpp>
#include <gridformat/encoding/raw.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/compression.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

template<std::size_t dim, std::size_t space_dim, typename Writer>
void write(const Writer& writer, const std::string& file_prefix) {
    std::string filename = file_prefix + "_" +
                           std::to_string(dim) + "d_in_" +
                           std::to_string(space_dim) + "d";
    filename = writer.write(filename);
    std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
}

template<std::size_t dim,
         std::size_t space_dim,
         typename XMLOptions,
         typename PrecisionOptions,
         typename FieldPrec = double>
void write(const XMLOptions& xml_opts,
           const PrecisionOptions& prec_opts,
           const std::string& filename_prefix,
           const GridFormat::Precision<FieldPrec>& prec = {}) {
    auto grid = GridFormat::Test::make_unstructured<dim, space_dim>();
    GridFormat::VTUWriter writer{grid, xml_opts, prec_opts};
    const auto test_data = GridFormat::Test::make_test_data<space_dim, double>(grid);
    GridFormat::Test::add_test_data(writer, test_data, prec);
    write<dim, space_dim>(writer, filename_prefix);
}

template<std::size_t dim,
         std::size_t space_dim,
         typename XMLOptions,
         typename PrecisionOptions,
         typename FieldPrec = double>
void write_from_abstract_base(const XMLOptions& xml_opts,
                              const PrecisionOptions& prec_opts,
                              const std::string& filename_prefix,
                              const GridFormat::Precision<FieldPrec>& prec = {}) {
    auto grid = GridFormat::Test::make_unstructured<dim, space_dim>();
    GridFormat::VTUWriter writer{grid, xml_opts, prec_opts};
    std::unique_ptr<GridFormat::GridWriter<decltype(grid)>> writer_ptr
        = std::make_unique<decltype(writer)>(std::move(writer));
    const auto test_data = GridFormat::Test::make_test_data<space_dim, double>(grid);
    GridFormat::Test::add_test_data(*writer_ptr, test_data, prec);
    write<dim, space_dim>(*writer_ptr, filename_prefix);
}

template<std::size_t dim, std::size_t space_dim>
void write_default() {
    write<dim, space_dim>(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .compressor = GridFormat::none,
            .data_format = GridFormat::VTK::DataFormat::appended
        },
        GridFormat::VTK::PrecisionOptions{},
        std::string{"vtu_base64_appended"}
    );
}

std::string data_format_name(const GridFormat::VTK::DataFormat::Inlined&) { return "inlined"; }
std::string data_format_name(const GridFormat::VTK::DataFormat::Appended&) { return "appended"; }

std::string encoder_name(const GridFormat::Encoding::Base64&) { return "base64"; }
std::string encoder_name(const GridFormat::Encoding::RawBinary&) { return "raw"; }


template<typename DataFormat, typename Encoder = GridFormat::Encoding::Base64>
void write_with(const DataFormat& format, const Encoder& enc = GridFormat::Encoding::base64) {
    const auto _filename = [&] (const std::string& postfix) {
        return "vtu_" + encoder_name(enc) + "_" + data_format_name(format) + (
            postfix != "" ? "_" + postfix : ""
        );
    };

    write<2, 2>(
        GridFormat::VTK::XMLOptions{
            .encoder = enc,
            .compressor = GridFormat::none,
            .data_format = format
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        _filename("")
    );
    write<3, 3>(
        GridFormat::VTK::XMLOptions{
            .encoder = enc,
            .compressor = GridFormat::none,
            .data_format = format
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        _filename("custom_field_precision"),
        GridFormat::Precision<float>{}
    );
    write_from_abstract_base<2, 2>(
        GridFormat::VTK::XMLOptions{
            .encoder = enc,
            .compressor = GridFormat::none,
            .data_format = format
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        _filename("from_base_writer")
    );
#if GRIDFORMAT_HAVE_LZMA
    write<1, 3>(
        GridFormat::VTK::XMLOptions{
            .encoder = enc,
            .compressor = GridFormat::Compression::lzma,
            .data_format = format
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        _filename("lzma_compression_custom_header_precision")
    );
    write<2, 2>(
        GridFormat::VTK::XMLOptions{
            .encoder = enc,
            .compressor = GridFormat::Compression::lzma,
            .data_format = format
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        _filename("lzma_compression_custom_header_precision")
    );
    write<3, 3>(
        GridFormat::VTK::XMLOptions{
            .encoder = enc,
            .compressor = GridFormat::Compression::lzma,
            .data_format = format
        },
        GridFormat::VTK::PrecisionOptions{},
        _filename("lzma_compression_custom_field_precision"),
        GridFormat::Precision<float>{}
    );
#endif  // GRIDFORMAT_HAVE_LZMA

#if GRIDFORMAT_HAVE_ZLIB
    write<1, 3>(
        GridFormat::VTK::XMLOptions{
            .encoder = enc,
            .compressor = GridFormat::Compression::zlib,
            .data_format = format
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        _filename("zlib_compression_custom_header_precision")
    );
    write<2, 2>(
        GridFormat::VTK::XMLOptions{
            .encoder = enc,
            .compressor = GridFormat::Compression::zlib,
            .data_format = format
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        _filename("zlib_compression_custom_header_precision")
    );
#endif  // GRIDFORMAT_HAVE_ZLIB

#if GRIDFORMAT_HAVE_LZ4
    write<1, 3>(
        GridFormat::VTK::XMLOptions{
            .encoder = enc,
            .compressor = GridFormat::Compression::lz4,
            .data_format = format
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        _filename("lz4_compression_custom_header_precision")
    );
    write<2, 2>(
        GridFormat::VTK::XMLOptions{
            .encoder = enc,
            .compressor = GridFormat::Compression::lz4,
            .data_format = format
        },
        GridFormat::VTK::PrecisionOptions{.header_precision = GridFormat::uint32},
        _filename("lz4_compression_custom_header_precision")
    );
#endif  // GRIDFORMAT_HAVE_LZ4
}

int main() {
    write_default<0, 1>();
    write_default<0, 2>();
    write_default<0, 3>();
    write_default<1, 1>();
    write_default<1, 2>();
    write_default<1, 3>();
    write_default<2, 2>();
    write_default<2, 3>();
    write_default<3, 3>();
    write<2, 2>(
        GridFormat::VTK::XMLOptions{.encoder = GridFormat::Encoding::ascii},
        GridFormat::VTK::PrecisionOptions{},
        "vtu_ascii"
    );

    write_with(GridFormat::VTK::DataFormat::inlined);
    write_with(GridFormat::VTK::DataFormat::appended);
    write_with(
        GridFormat::VTK::DataFormat::appended,
        GridFormat::Encoding::raw
    );

    return 0;
}
