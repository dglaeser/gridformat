// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <string>
#include <filesystem>
#include <iterator>
#include <algorithm>
#include <ranges>
#include <fstream>
#include <vector>
#include <regex>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/vtu_reader.hpp>
#include <gridformat/vtk/vtu_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"
#include "../testing.hpp"

#ifndef TEST_DATA_PATH
#define TEST_DATA_PATH ""
#endif


int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "vtu_boolean_field"_test = [&] () {
        GridFormat::VTUWriter bool_writer{grid};
        bool_writer.set_cell_field("true_field", [] (auto&&...) { return true; });
        bool_writer.set_cell_field("false_field", [] (auto&&...) { return false; });
        bool_writer.write("vtu_bool_test");

        GridFormat::VTUReader bool_reader;
        bool_reader.open("vtu_bool_test.vtu");
        const auto true_field = bool_reader.cell_field("true_field")->template export_to<std::vector<bool>>();
        const auto false_field = bool_reader.cell_field("false_field")->template export_to<std::vector<bool>>();
        expect(std::ranges::all_of(true_field, [] (bool value) { return value; }));
        expect(std::ranges::all_of(false_field, [] (bool value) { return not value; }));

        std::filesystem::remove("vtu_bool_test.vtu");
    };

    "vtu_reader_crlf_line_endings"_test = [&] () {
        const auto test = [&] (auto writer, const std::string& name) {
            writer.set_point_field("pfield", [] (const auto& p) { return static_cast<double>(p.id); });
            const std::string filename = writer.write(name);

            // convert the line endings of the xml part, keeping the (possibly raw binary) appended data intact
            std::string content;
            {
                std::ifstream in{filename, std::ios::binary};
                content.assign(std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{});
            }
            const auto appendix_pos = content.find("<AppendedData");
            const auto xml_end = appendix_pos == std::string::npos ? content.size() : content.find('_', appendix_pos);
            const std::string converted = std::regex_replace(content.substr(0, xml_end), std::regex{"\n"}, "\r\n")
                                        + content.substr(xml_end);
            std::ofstream{name + "_crlf.vtu", std::ios::binary} << converted;
            expect(converted.find("\r\n") != std::string::npos);

            GridFormat::VTUReader reader;
            reader.open(filename);
            const auto expected = reader.point_field("pfield")->template export_to<std::vector<double>>();
            reader.open(name + "_crlf.vtu");
            expect(eq(reader.number_of_points(), GridFormat::number_of_points(grid)));
            expect(std::ranges::equal(reader.point_field("pfield")->template export_to<std::vector<double>>(), expected));

            std::filesystem::remove(filename);
            std::filesystem::remove(name + "_crlf.vtu");
        };
        test(GridFormat::VTUWriter{grid}.with_encoding(GridFormat::Encoding::raw)
                                        .with_data_format(GridFormat::VTK::DataFormat::appended),
             "vtu_crlf_appended_raw");
        test(GridFormat::VTUWriter{grid}.with_encoding(GridFormat::Encoding::base64)
                                        .with_data_format(GridFormat::VTK::DataFormat::inlined),
             "vtu_crlf_inlined_base64");
        test(GridFormat::VTUWriter{grid}.with_encoding(GridFormat::Encoding::ascii),
             "vtu_crlf_ascii");
    };

    GridFormat::VTUWriter writer{grid};
    GridFormat::VTUReader reader;
    GridFormat::Test::test_reader<2, 2>(
        writer,
        reader,
        "reader_vtu_test_file_2d_in_2d"
    );

    const std::string test_data_path_name{TEST_DATA_PATH};
    if (test_data_path_name.empty()) {
        std::cout << "No test data folder defined, skipping further tests" << std::endl;
        return 42;
    }

    if (!std::filesystem::directory_entry{std::filesystem::path{test_data_path_name}}.exists()) {
        std::cout << "Test data folder does not exist, skipping further tests" << std::endl;
        return 42;
    }

    std::vector<std::string> vtu_files;
    std::ranges::copy(
        std::filesystem::directory_iterator{std::filesystem::path{test_data_path_name}}
        | std::views::filter([] (const auto& p) { return std::filesystem::is_regular_file(p); })
        | std::views::filter([] (const std::filesystem::path& p) { return p.extension() == ".vtu"; })
        | std::views::transform([] (const std::filesystem::path& p) { return p.string(); }),
        std::back_inserter(vtu_files)
    );
    if (vtu_files.empty()) {
        std::cout << "No test vtu files found in folder " << test_data_path_name << ". Skipping..." << std::endl;
        return 42;
    }

    "vtu_reader_name"_test = [&] () {
        expect(reader.name() == "VTUReader");
    };

    "vtk_written_vtu_files"_test = [&] () {
        for (const std::string& vtu_filepath : vtu_files) {
            std::cout << "Testing '" << GridFormat::as_highlight(vtu_filepath) << "'" << std::endl;
            reader.open(vtu_filepath);

            expect(eq(reader.number_of_pieces(), std::size_t{1}));

            const auto grid = [&] () {
                GridFormat::Test::UnstructuredGridFactory<2, 2> factory;
                reader.export_grid(factory);
                return std::move(factory).grid();
            } ();

            for (const auto& [name, field_ptr] : point_fields(reader))
                expect(GridFormat::Test::test_field_values<2>(name, field_ptr, grid, GridFormat::points(grid)));
            for (const auto& [name, field_ptr] : cell_fields(reader))
                expect(GridFormat::Test::test_field_values<2>(name, field_ptr, grid, GridFormat::cells(grid)));
        }
    };

    return 0;
}
