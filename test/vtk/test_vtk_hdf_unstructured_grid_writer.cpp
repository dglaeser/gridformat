// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>

#include <gridformat/vtk/hdf_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"
#include "../testing.hpp"

int main() {

    {
        const auto grid = GridFormat::Test::make_unstructured_2d();
        GridFormat::VTKHDFWriter writer{grid};
        GridFormat::Test::write_test_file<2>(writer, "vtk_hdf_unstructured_2d_in_2d");
    }

    {
        const auto grid = GridFormat::Test::make_unstructured_3d();
        GridFormat::VTKHDFWriter writer{grid};
        GridFormat::Test::write_test_file<3>(writer, "vtk_hdf_unstructured_3d_in_3d");
    }

    {  // unit-test the IOContext helper class
        using GridFormat::Testing::throws;
        using GridFormat::Testing::expect;
        using GridFormat::Testing::eq;
        using GridFormat::Testing::operator""_test;

        using GridFormat::VTKHDF::IOContext;
        using Vector = std::vector<std::size_t>;

        "valid_sequential_io_context"_test = [] () {
            IOContext valid{0, 1, Vector{1}, Vector{1}};
            expect(!valid.is_parallel);
            expect(eq(valid.my_rank, 0));
            expect(eq(valid.num_ranks, 1));
        };

        "valid_parallel_io_context"_test = [] () {
            IOContext valid{0, 2, Vector{1, 1}, Vector{1, 1}};
            expect(valid.is_parallel);
            expect(eq(valid.my_rank, 0));
            expect(eq(valid.num_ranks, 2));
        };

        "io_context_invalid_construction"_test = [] () {
            expect(throws([] () { IOContext{0, 0, Vector{}, Vector{}}; }));
            expect(throws([] () { IOContext{2, 1, Vector{1}, Vector{0}}; }));
            expect(throws([] () { IOContext{0, 1, Vector{1, 1}, Vector{0}}; }));
            expect(throws([] () { IOContext{0, 1, Vector{1}, Vector{0, 1}}; }));
            expect(throws([] () { IOContext{0, 1, Vector{1, 1}, Vector{0, 1}}; }));
        };
    }

    return 0;
}
