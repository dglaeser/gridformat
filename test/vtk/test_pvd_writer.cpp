#include <cmath>
#include <vector>
#include <ranges>
#include <algorithm>

#include <gridformat/grid.hpp>
#include <gridformat/encoding.hpp>
#include <gridformat/compression.hpp>

#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include "../grid/unstructured_grid.hpp"

template<typename Grid, typename Writer>
void test(const Grid& grid, Writer& pvd_writer) {
    std::vector<double> point_data(GridFormat::number_of_points(grid));
    std::ranges::copy_if(
        std::views::iota(std::size_t{0}, GridFormat::number_of_points(grid)),
        point_data.begin(),
        [] (const auto&) { return true; },
        [] (const std::integral auto idx) { return std::cos(static_cast<double>(idx)); }
    );

    std::vector<double> cell_data(GridFormat::number_of_cells(grid));
    std::ranges::copy_if(
        std::views::iota(std::size_t{0}, GridFormat::number_of_cells(grid)),
        cell_data.begin(),
        [] (const auto&) { return true; },
        [] (const std::integral auto idx) { return std::cos(static_cast<double>(idx)); }
    );

    pvd_writer.set_point_field("pdata", [&] (const auto& p) { return point_data[p.id]; });
    pvd_writer.set_cell_field("cdata", [&] (const auto& c) { return cell_data[c.id]; });
    pvd_writer.write(0.0);

    double sim_time = 0.0;
    const double end_time = 10.0;
    const double timestep_size = 1.0;
    while (sim_time < end_time - 1e-6) {
        std::ranges::for_each(point_data, [] (double& v) { v*= 2.0; });
        std::ranges::for_each(cell_data, [] (double& v) { v*= 2.0; });
        sim_time += timestep_size;
        pvd_writer.write(sim_time);
    }
}

template<typename Grid>
void test_from_instance(const Grid& grid) {
    GridFormat::PVDWriter pvd_writer{GridFormat::VTUWriter{grid}, "pvd_time_series"};
    test(grid, pvd_writer);
}

template<typename Grid>
void test_from_abstract_base_ptr(const Grid& grid) {
    using BaseWriter = GridFormat::TimeSeriesGridWriter<Grid>;
    auto writer = GridFormat::PVDWriter{GridFormat::VTUWriter{grid}, "pvd_time_series_from_base_writer"};
    std::unique_ptr<BaseWriter> pvd_writer = std::make_unique<decltype(writer)>(std::move(writer));
    test(grid, *pvd_writer);
}

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();
    test_from_instance(grid);
    test_from_abstract_base_ptr(grid);
    return 0;
}