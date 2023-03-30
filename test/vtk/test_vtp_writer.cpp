// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#include <gridformat/encoding.hpp>
#include <gridformat/compression.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/vtp_writer.hpp>

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
         typename FieldPrec = double>
void write(const GridFormat::VTK::XMLOptions& xml_opts,
           const std::string& filename_prefix,
           const GridFormat::Precision<FieldPrec>& prec = {}) {
    auto grid = GridFormat::Test::make_unstructured<dim, space_dim>();
    GridFormat::VTPWriter writer{grid, xml_opts};
    const auto test_data = GridFormat::Test::make_test_data<space_dim, double>(grid);
    GridFormat::Test::add_test_data(writer, test_data, prec);
    write<dim, space_dim>(writer, filename_prefix);

    // using the with-syntax
    using namespace GridFormat;
    auto writer2 = writer
        .with_encoding(Variant::replace<Automatic>(xml_opts.encoder, Encoding::base64))
        .with_compression(Variant::replace<Automatic>(xml_opts.compressor, none))
        .with_data_format(Variant::replace<Automatic>(
            xml_opts.data_format,
            Variant::is<Encoding::Ascii>(xml_opts.encoder) ? VTK::XML::DataFormat{VTK::DataFormat::inlined}
                                                           : VTK::XML::DataFormat{VTK::DataFormat::appended}
        ))
        .with_header_precision(xml_opts.header_precision)
        .with_coordinate_precision(Variant::replace<Automatic>(xml_opts.coordinate_precision, float64));
    write<dim, space_dim>(writer2, filename_prefix + "_chained");
}

template<std::size_t dim,
         std::size_t space_dim,
         typename FieldPrec = double>
void write_from_abstract_base(const GridFormat::VTK::XMLOptions& xml_opts,
                              const std::string& filename_prefix,
                              const GridFormat::Precision<FieldPrec>& prec = {}) {
    auto grid = GridFormat::Test::make_unstructured<dim, space_dim>();
    GridFormat::VTPWriter writer{grid, xml_opts};
    std::unique_ptr<GridFormat::GridWriter<decltype(grid)>> writer_ptr
        = std::make_unique<decltype(writer)>(std::move(writer));
    const auto test_data = GridFormat::Test::make_test_data<space_dim, double>(grid);
    GridFormat::Test::add_test_data(*writer_ptr, test_data, prec);
    write<dim, space_dim>(*writer_ptr, filename_prefix + "_from_abstract_base");
}

template<std::size_t dim, std::size_t space_dim>
void write_default() {
    write<dim, space_dim>(
        GridFormat::VTK::XMLOptions{},
        "vtp_default"
    );
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
        "vtp_ascii"
    );
    write<2, 2>(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .data_format = GridFormat::VTK::DataFormat::inlined,
            .header_precision = GridFormat::uint32
        },
        "vtp_base64_inlined"
    );
    write_from_abstract_base<2, 2>(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .data_format = GridFormat::VTK::DataFormat::inlined,
            .header_precision = GridFormat::uint32
        },
        "vtp_base64_inlined_from_base_writer"
    );
#if GRIDFORMAT_HAVE_LZMA
    write<2, 2>(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .compressor = GridFormat::Compression::lzma,
            .data_format = GridFormat::VTK::DataFormat::inlined,
            .header_precision = GridFormat::uint32
        },
        "vtp_base64_inlined_lzma_compression_custom_header_precision"
    );
#endif  // GRIDFORMAT_HAVE_LZMA

    return 0;
}
