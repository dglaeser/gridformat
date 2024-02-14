// SPDX-FileCopyrightText: 2024 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <ranges>
#include <filesystem>

#include <gridformat/gridformat.hpp>
#include "../common.hpp"

template<typename Coordinate>
double test_function(const Coordinate& position) {
    return position[0]*position[1];
}

int main() {
    GridFormat::ImageGrid<2, double> grid{{1.0, 1.0}, {1000, 1000}};
    GridFormat::VTUWriter writer{grid};

    constexpr int num_fields = 3;
    for (int i = 0; i < num_fields; ++i) {
        writer.set_point_field("pf_" + std::to_string(i), [&] (const auto& p) {
            return test_function(grid.position(p));
        });
        writer.set_cell_field("cf_" + std::to_string(i), [&] (const auto& c) {
            return test_function(grid.center(c));
        });
    }

    const auto ascii_results = GridFormat::Benchmark::measure_writer(
        writer.with_encoding(GridFormat::Encoding::ascii),
        "ascii"
    );
    const auto app_raw_results = GridFormat::Benchmark::measure_writer(
        writer.with_encoding(GridFormat::Encoding::raw),
        "appended_raw"
    );
    const auto app_base64_results = GridFormat::Benchmark::measure_writer(
        writer.with_encoding(GridFormat::Encoding::base64)
              .with_data_format(GridFormat::VTK::DataFormat::appended),
        "appended_base64"
    );
    const auto inline_base64_results = GridFormat::Benchmark::measure_writer(
        writer.with_encoding(GridFormat::Encoding::base64)
              .with_data_format(GridFormat::VTK::DataFormat::inlined),
        "inlined_base64"
    );

    GridFormat::Benchmark::write_results_to("benchmark_vtu.csv", {
        {"ascii", ascii_results},
        {"app_raw", app_raw_results},
        {"app_b64", app_base64_results},
        {"inline_b64", inline_base64_results}
    });

    return 0;
}
