// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <utility>
#include <fstream>
#include <filesystem>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/encoding/raw.hpp>
#include <gridformat/vtk/xml.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtu_reader.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../testing.hpp"

// read a raw-binary, compressed data array whose header and block sizes are given by the header values
GridFormat::Serialization read_compressed(const std::vector<std::uint64_t>& header, std::size_t payload_bytes = 0) {
    std::stringstream s;
    s.write(reinterpret_cast<const char*>(header.data()), header.size()*sizeof(std::uint64_t));
    s << std::string(payload_bytes, '\0');
    GridFormat::Serialization values;
    GridFormat::VTK::XMLDetail::DataArrayReader<double, std::uint64_t>{s, std::endian::native, "vtkZLibDataCompressor"}
        .read_binary(GridFormat::RawDecoder{}, {}, values);
    return values;
}

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    static constexpr std::uint64_t huge = std::uint64_t{1} << 40;

    "compressed_header_residual_block_without_blocks"_test = [] () {
        expect(throws<GridFormat::IOError>([] () { read_compressed({0, 32, 8}); }));
    };

    "compressed_header_zero_block_size"_test = [] () {
        expect(throws<GridFormat::IOError>([] () { read_compressed({1, 0, 0, 8}, 8); }));
    };

    "compressed_header_residual_larger_than_block"_test = [] () {
        expect(throws<GridFormat::IOError>([] () { read_compressed({1, 32, 64, 8}, 8); }));
    };

    "compressed_header_overflowing_number_of_bytes"_test = [] () {
        expect(throws<GridFormat::IOError>([] () { read_compressed({huge, huge, 0}); }));
    };

    "compressed_header_too_many_blocks"_test = [] () {
        expect(throws<GridFormat::IOError>([] () { read_compressed({huge, 32, 0}); }));
    };

    "compressed_header_too_large_block"_test = [] () {
        expect(throws<GridFormat::IOError>([] () { read_compressed({1, 32, 0, huge}, 8); }));
    };

    "compressed_header_empty_array"_test = [] () {
        expect(eq(read_compressed({0, 32, 0}).size(), std::size_t{0}));
    };

#if GRIDFORMAT_HAVE_ZLIB
    "corrupt_header_error_names_the_data_array"_test = [] () {
        const auto grid = GridFormat::Test::make_unstructured_2d();
        auto writer = GridFormat::VTUWriter{grid}.with_encoding(GridFormat::Encoding::raw)
                                                 .with_compression(GridFormat::Compression::zlib)
                                                 .with_data_format(GridFormat::VTK::DataFormat::appended);
        writer.set_point_field("pressure", [] (const auto&) { return 1.0; });
        const std::string filename = writer.write("test_vtk_xml_corrupt_headers");

        // overwrite the number of blocks in the header of the pressure field
        std::string content;
        {
            std::ifstream in{filename, std::ios::binary};
            std::ostringstream buffer;
            buffer << in.rdbuf();
            content = std::move(buffer).str();
        }
        expect(content.find("header_type=\"UInt64\"") != std::string::npos);
        const auto attribute_pos = content.find("offset=\"", content.find("Name=\"pressure\""));
        const auto offset = std::stoull(content.substr(attribute_pos + 8));
        const auto header_pos = content.find('_', content.find("<AppendedData")) + 1 + offset;
        content.replace(header_pos, sizeof(std::uint64_t), reinterpret_cast<const char*>(&huge), sizeof(std::uint64_t));
        std::ofstream{filename, std::ios::binary} << content;

        bool named_array = false;
        try {
            GridFormat::VTUReader reader;
            reader.open(filename);
            reader.point_field("pressure")->template export_to<std::vector<double>>();
        } catch (const GridFormat::IOError& e) {
            named_array = std::string{e.what()}.find("'pressure'") != std::string::npos;
        }
        expect(named_array);
        std::filesystem::remove(filename);
    };
#endif

    return 0;
}
