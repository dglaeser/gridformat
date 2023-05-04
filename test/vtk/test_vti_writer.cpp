// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <numbers>

#include <gridformat/vtk/vti_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "vtk_writer_tester.hpp"

template<typename Grid>
void _test(Grid&& grid, std::string suffix = "") {
    GridFormat::Test::VTK::WriterTester tester{std::move(grid), ".vti", true, suffix};
    tester.test([&] (const auto& grid, const auto& xml_opts) {
        return GridFormat::VTIWriter{grid, xml_opts};
    });
}

int main() {
    _test(
        GridFormat::Test::StructuredGrid<2>{
            {{1.0, 1.0}},
            {{10, 10}}
        }
    );

    _test(
        GridFormat::Test::StructuredGrid<3>{
            {{1.0, 1.0, 1.0}},
            {{10, 10, 10}}
        }
    );

    _test(
        GridFormat::Test::StructuredGrid<3>{
            {{1.0, 1.0, 1.0}},
            {{10, 10, 10}},
            {{1.0, 1.0, 1.0}}
        },
        "shifted"
    );

    GridFormat::Test::StructuredGrid<3> inverted{
        {{1.0, 1.0, 1.0}},
        {{10, 10, 10}}
    };
    inverted.invert();
    _test(std::move(inverted), "inverted");

    constexpr auto sqrt2_half = 1.0/std::numbers::sqrt2;
    _test(
        GridFormat::Test::OrientedStructuredGrid<2>{
            {
                std::array<double, 2>{sqrt2_half, sqrt2_half},
                std::array<double, 2>{-sqrt2_half, sqrt2_half}
            },
            {{1.0, 1.0}},
            {{10, 10}}
        },
        "oriented"
    );

    _test(
        GridFormat::Test::OrientedStructuredGrid<3>{
            {
                std::array<double, 3>{sqrt2_half, sqrt2_half, 0.0},
                std::array<double, 3>{-sqrt2_half, sqrt2_half, 0.0},
                std::array<double, 3>{0.0, 0.0, 1.0}
            },
            {{1.0, 1.0, 1.0}},
            {{10, 10, 10}}
        },
        "oriented"
    );

    return 0;
}
