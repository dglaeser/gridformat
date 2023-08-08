// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <utility>
#include <string>

#include <gridformat/common/string_conversion.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/gridformat.hpp>

#include "../make_test_data.hpp"
#include "../reader_tests.hpp"
#include "../testing.hpp"

#ifndef TEST_VTK_DATA_PATH
#define TEST_VTK_DATA_PATH ""
#endif

auto get_grid_and_space_dimension(const std::string& filename) {
    const auto pos = filename.find("d_in_");
    if (pos == std::string::npos || pos == 0 || pos > filename.size() - 7)
        throw GridFormat::ValueError("Could not deduce grid & space dim from filename '" + filename + "'");
    const unsigned int grid_dim = std::stoi(filename.substr(pos-1, 1));
    const unsigned int space_dim = std::stoi(filename.substr(pos+5, 1));
    return std::make_pair(grid_dim, space_dim);
}

std::string get_test_filename(const std::filesystem::path& folder, const std::string& extension) {
    if (!std::filesystem::directory_entry{folder}.exists())
        throw GridFormat::IOError("Test data folder '" + std::string{folder} + "' does not exist");
    for (const std::filesystem::path& file
        : std::filesystem::directory_iterator{folder}
        | std::views::filter([&] (const auto& p) { return std::filesystem::is_regular_file(p); })
        | std::views::filter([&] (const std::filesystem::path& p) { return p.extension() == extension; }))
        return file.string();
    throw GridFormat::IOError("Could not find test file with extension '" + extension + "'");
}

void test_reader(GridFormat::Reader&& reader, const std::string& filename) {
    std::cout << "Testing reader with '" << GridFormat::as_highlight(filename) << "'" << std::endl;

    reader.open(filename);
    const auto points = reader.points()->template export_to<std::vector<std::array<double, 3>>>();
    const auto [_, space_dim] = get_grid_and_space_dimension(filename);
    const auto get_expected_value = [&] (const std::array<double, 3>& position) -> double {
        if (space_dim == 1)
            return GridFormat::Test::test_function<double>(std::array{position[0]});
        if (space_dim == 2)
            return GridFormat::Test::test_function<double>(std::array{position[0], position[1]});
        return GridFormat::Test::test_function<double>(position);
    };

    std::vector<std::string> read_cell_fields;
    std::vector<std::string> read_point_fields;
    std::vector<std::string> read_meta_data_fields;

    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;
    for (const auto [name, fieldptr] : point_fields(reader)) {
        read_point_fields.push_back(name);
        if (fieldptr->layout().dimension() > 2) {
            std::cout << "Skipping point field " << name << ", because it is not a scalar field" << std::endl;
        } else {
            const auto values = fieldptr->template export_to<std::vector<double>>();
            std::size_t p_idx = 0;
            std::ranges::for_each(values, [&] (const auto& value) {
                const auto& point = points[p_idx];
                const auto expected_value = get_expected_value(point);
                expect(GridFormat::Test::equals(expected_value, value));
                p_idx++;
            });
            expect(eq(reader.number_of_points(), p_idx));
        }
    }
    for (const auto [name, fieldptr] : cell_fields(reader)) {
        read_cell_fields.push_back(name);
        if (fieldptr->layout().dimension() > 2) {
            std::cout << "Skipping cell field " << name << ", because it is not a scalar field" << std::endl;
        } else {
            const auto values = fieldptr->template export_to<std::vector<double>>();
            std::size_t c_idx = 0;
            reader.visit_cells([&] (GridFormat::CellType, const std::vector<std::size_t>& corners) {
                std::array<double, 3> center{0., 0., 0.};
                std::ranges::for_each(corners, [&] (const auto corner_idx) {
                    const auto& corner = points.at(corner_idx);
                    center[0] += corner[0];
                    center[1] += corner[1];
                    center[2] += corner[2];
                });
                std::ranges::for_each(center, [&] (auto& c) { c /= static_cast<double>(corners.size()); });
                const auto expected_value = get_expected_value(center);
                expect(GridFormat::Test::equals(expected_value, values[c_idx]));
                c_idx++;
            });
            expect(eq(reader.number_of_cells(), c_idx));
        }
    }

    for (const auto [name, _] : meta_data_fields(reader)) {
        read_meta_data_fields.push_back(name);
        std::cout << "Successfully read meta data " << name << std::endl;
    }

    expect(std::ranges::equal(read_point_fields, point_field_names(reader)));
    expect(std::ranges::equal(read_cell_fields, cell_field_names(reader)));
    expect(std::ranges::equal(read_meta_data_fields, meta_data_field_names(reader)));

    std::cout << "Tested the point fields: " << GridFormat::as_string(read_point_fields) << std::endl;
    std::cout << "Tested the cell fields: " << GridFormat::as_string(read_cell_fields) << std::endl;
}

template<typename VTKFormat>
void test_vtk_read(const VTKFormat& format, const std::string& extension) {
    test_reader(GridFormat::Reader{format}, get_test_filename(std::string{TEST_VTK_DATA_PATH}, extension));
}


int main() {
    test_vtk_read(GridFormat::vtu, ".vtu");
    test_vtk_read(GridFormat::vtp, ".vtp");
    test_vtk_read(GridFormat::vti, ".vti");
    test_vtk_read(GridFormat::vtr, ".vtr");
    test_vtk_read(GridFormat::vts, ".vts");
    return 0;
}
