// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>
#include <ranges>
#include <algorithm>

#include <gridformat/grid/image_grid.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtu_reader.hpp>

#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    const GridFormat::ImageGrid<2, double> grid{{1.0, 1.0}, {2, 2}};
    const std::string field_name = "p\"<&>'";
    const std::string meta_data_name = "m\"<&>'";
    const std::string meta_data_value = "a\"<&>'b";

    const auto test_round_trip = [&] (const std::string& filename, GridFormat::VTK::XMLOptions opts) {
        GridFormat::VTUWriter writer{grid, opts};
        writer.set_cell_field(field_name, [] (const auto&) { return 1.0; });
        writer.set_meta_data(meta_data_name, meta_data_value);
        const auto written = writer.write(filename);

        GridFormat::VTUReader reader;
        reader.open(written);
        const auto names = cell_field_names(reader);
        expect(std::ranges::find(names, field_name) != std::ranges::end(names));
        expect(eq(
            reader.meta_data_field(meta_data_name)->template export_to<std::string>(),
            meta_data_value
        ));
    };

    "xml_special_characters_in_names_inlined"_test = [&] () {
        test_round_trip("xml_special_characters_inlined", {
            .encoder = GridFormat::Encoding::ascii,
            .data_format = GridFormat::VTK::DataFormat::inlined
        });
    };

    "xml_special_characters_in_names_appended"_test = [&] () {
        test_round_trip("xml_special_characters_appended", {
            .encoder = GridFormat::Encoding::base64,
            .data_format = GridFormat::VTK::DataFormat::appended
        });
    };

    return 0;
}
