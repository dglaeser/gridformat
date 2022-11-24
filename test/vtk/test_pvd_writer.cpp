#include <cmath>
#include <vector>
#include <ranges>
#include <algorithm>

#include <gridformat/grid.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

template<typename Grid, typename Writer>
void test(const Grid& grid, Writer& pvd_writer) {
    auto point_data = GridFormat::Test::make_point_data<double>(grid);
    auto cell_data = GridFormat::Test::make_cell_data<double>(grid);

    pvd_writer.set_point_field("pdata", [&] (const auto& p) { return point_data[p.id]; });
    pvd_writer.set_cell_field("cdata", [&] (const auto& c) { return cell_data[c.id]; });
    std::string filename = pvd_writer.write(0.0);
    std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
    double sim_time = 0.0;
    const double end_time = 10.0;
    const double timestep_size = 1.0;
    while (sim_time < end_time - 1e-6) {
        std::ranges::for_each(point_data, [] (double& v) { v*= 2.0; });
        std::ranges::for_each(cell_data, [] (double& v) { v*= 2.0; });
        sim_time += timestep_size;
        filename = pvd_writer.write(sim_time);
        std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
    }
}

template<typename Grid>
void test_from_instance(const Grid& grid) {
    GridFormat::PVDWriter pvd_writer{GridFormat::VTUWriter{grid}, "pvd_time_series_2d_in_2d"};
    test(grid, pvd_writer);
}

template<typename Grid>
void test_from_abstract_base_ptr(const Grid& grid) {
    using BaseWriter = GridFormat::TimeSeriesGridWriter<Grid>;
    auto writer = GridFormat::PVDWriter{GridFormat::VTUWriter{grid}, "pvd_time_series_from_base_writer_2d_in_2d"};
    std::unique_ptr<BaseWriter> pvd_writer = std::make_unique<decltype(writer)>(std::move(writer));
    test(grid, *pvd_writer);
}

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();
    test_from_instance(grid);
    test_from_abstract_base_ptr(grid);
    return 0;
}
