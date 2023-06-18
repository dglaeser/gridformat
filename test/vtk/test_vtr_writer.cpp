// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/vtk/vtr_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "vtk_writer_tester.hpp"

template<typename Grid>
void _test(Grid&& grid, const std::string& suffix = "") {
    GridFormat::Test::VTK::WriterTester tester{std::move(grid), ".vtr", true, suffix};
    tester.test([&] (const auto& grid, const auto& xml_opts) {
        return GridFormat::VTRWriter{grid, xml_opts};
    });
}

int main() {
    for (std::size_t nx : {2})
        for (std::size_t ny : {2, 3}) {
            const auto base_suffix = std::to_string(nx) + "_" + std::to_string(ny);
            _test(
                GridFormat::Test::StructuredGrid<2>{{{1.0, 1.0}}, {{nx, ny}}},
                base_suffix
            );

            _test(
                GridFormat::Test::StructuredGrid<2>{{{1.0, 1.0}}, {{nx, ny}}, {{1.0, 1.0}}},
                base_suffix + "_shifted"
            );

            GridFormat::Test::StructuredGrid<2> inverted{{{1.0, 1.0}}, {{nx, ny}}, {{1.0, 1.0}}};
            inverted.invert();
            _test(std::move(inverted), base_suffix + "_inverted");
        }

    for (std::size_t nx : {2})
        for (std::size_t ny : {2, 3})
            for (std::size_t nz : {2, 4}) {
                const auto base_suffix = std::to_string(nx) + "_" + std::to_string(ny) + "_" + std::to_string(nz);
                _test(
                    GridFormat::Test::StructuredGrid<3>{{{1.0, 1.0, 1.0}}, {{nx, ny, nz}}},
                    base_suffix
                );

                _test(
                    GridFormat::Test::StructuredGrid<3>{{{1.0, 1.0, 1.0}}, {{nx, ny, nz}}, {{1.0, 1.0, 1.0}}},
                    base_suffix + "_shifted"
                );

                GridFormat::Test::StructuredGrid<3> inverted{{{1.0, 1.0, 1.0}}, {{nx, ny, nz}}, {{1.0, 1.0, 1.0}}};
                inverted.invert();
                _test(std::move(inverted), base_suffix + "_inverted");
            }

    return 0;
}
