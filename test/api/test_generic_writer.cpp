// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <iostream>
#include <algorithm>

#include <gridformat/common/ranges.hpp>
#include <gridformat/gridformat.hpp>

#include "../make_test_data.hpp"
#include "../testing.hpp"


template<typename Writer>
void add_fields(Writer& writer) {
    const auto& grid = writer.grid();
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("point_func", [&] (const auto& p) {
        return GridFormat::Test::test_function<double>(grid.position(p));
    });
    writer.set_cell_field("cell_func", [&] (const auto& c) {
        return GridFormat::Test::test_function<double>(grid.center(c));
    });
}


template<typename Writer>
void write(Writer&& writer, std::string suffix = "") {
    add_fields(writer);
    suffix = suffix != "" ? "_" + suffix : "";
    std::cout << "Wrote '" << GridFormat::as_highlight(writer.write("generic_2d_in_2d" + suffix)) << "'" << std::endl;
}

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;

    GridFormat::ImageGrid<2, double> grid{{1.0, 1.0}, {10, 15}};
    write(GridFormat::Writer{GridFormat::vtu({.encoder = GridFormat::Encoding::ascii}), grid});
    write(GridFormat::Writer{GridFormat::vti({.encoder = GridFormat::Encoding::raw}), grid});
    write(GridFormat::Writer{GridFormat::vtr({.data_format = GridFormat::VTK::DataFormat::appended}), grid});
    write(GridFormat::Writer{GridFormat::vts({.compressor = GridFormat::none}), grid});
    write(GridFormat::Writer{GridFormat::vtp({}), grid});
    write(GridFormat::Writer{GridFormat::any, grid}, "any");
    write(GridFormat::Writer{GridFormat::default_for(grid), grid}, "default");
    write(
        GridFormat::Writer{GridFormat::default_for(grid).with({.encoder = GridFormat::Encoding::ascii}), grid},
        "default_with_opts"
    );

#if GRIDFORMAT_HAVE_HIGH_FIVE
    write(GridFormat::Writer{GridFormat::vtk_hdf, grid}, "unstructured");
    {  // include in regression testing once new vtk version is out
        GridFormat::Writer writer{GridFormat::FileFormat::VTKHDFImage{}, grid};
        add_fields(writer);
        std::cout << "Wrote '" << GridFormat::as_highlight(
            writer.write("_ignore_regression_generic_2d_in_2d")
        ) << "'" << std::endl;
    }
#endif

    GridFormat::Writer writer{GridFormat::vtu, grid};
    add_fields(writer);

    "cell_field_iterator"_test = [&] () {
        GridFormat::Writer cpy{GridFormat::vtu, grid};
        writer.copy_fields(cpy);
        expect(GridFormat::Ranges::size(cell_fields(cpy)) == 1);
        expect(std::ranges::all_of(
            cell_fields(cpy),
            [] (auto pair) { return pair.first == "cell_func"; }
        ));
    };

    "point_field_iterator"_test = [&] () {
        GridFormat::Writer cpy{GridFormat::vtu, grid};
        writer.copy_fields(cpy);
        expect(GridFormat::Ranges::size(point_fields(cpy)) == 1);
        expect(std::ranges::all_of(
            point_fields(cpy),
            [] (auto pair) { return pair.first == "point_func"; }
        ));
    };

    "meta_data_iterator"_test = [&] () {
        GridFormat::Writer tmp{GridFormat::vtu, grid};
        tmp.set_meta_data("time", 1.0);
        expect(GridFormat::Ranges::size(meta_data_fields(tmp)) == 1);
        expect(std::ranges::all_of(
            meta_data_fields(tmp),
            [] (auto pair) { return pair.first == "time"; }
        ));
    };

    "field_removal"_test = [&] () {
        GridFormat::Writer cpy{GridFormat::vtu, grid};
        writer.copy_fields(cpy);
        cpy.set_meta_data("time", 1.0);
        [[maybe_unused]] GridFormat::FieldPtr mf = cpy.remove_meta_data("time");
        [[maybe_unused]] GridFormat::FieldPtr cf = cpy.remove_cell_field("cell_func");
        [[maybe_unused]] GridFormat::FieldPtr pf = cpy.remove_point_field("point_func");
        expect(std::ranges::none_of(
            meta_data_fields(cpy),
            [] (auto pair) { return pair.first == "time"; }
        ));
        expect(std::ranges::none_of(
            cell_fields(cpy),
            [] (auto pair) { return pair.first == "cell_func"; }
        ));
        expect(std::ranges::none_of(
            point_fields(cpy),
            [] (auto pair) { return pair.first == "point_func"; }
        ));
    };

    "clear"_test = [&] () {
        GridFormat::Writer cpy{GridFormat::vtu, grid};
        writer.copy_fields(cpy);
        cpy.clear();
        expect(GridFormat::Ranges::size(cell_fields(cpy)) == 0);
        expect(GridFormat::Ranges::size(point_fields(cpy)) == 0);
        expect(GridFormat::Ranges::size(meta_data_fields(cpy)) == 0);
    };

    return 0;
}
