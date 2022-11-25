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

template<std::size_t dim, typename Grid, typename Writer>
void test(const Grid& grid, Writer& pvd_writer) {
    double sim_time = 0.0;
    auto test_data = GridFormat::Test::make_test_data<dim, double>(grid, sim_time);
    GridFormat::Test::add_test_data(pvd_writer, test_data, GridFormat::Precision<float>{});

    std::string filename = pvd_writer.write(sim_time);
    std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;

    const double end_time = 10.0;
    const double timestep_size = 1.0;
    while (sim_time < end_time - 1e-6) {
        sim_time += timestep_size;

        pvd_writer.clear();
        test_data = GridFormat::Test::make_test_data<dim, double>(grid, sim_time);
        GridFormat::Test::add_test_data(pvd_writer, test_data, GridFormat::Precision<float>{});

        filename = pvd_writer.write(sim_time);
        std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
    }
}

template<std::size_t dim, typename Grid>
void test_from_instance(const Grid& grid) {
    GridFormat::PVDWriter pvd_writer{GridFormat::VTUWriter{grid}, "pvd_time_series_2d_in_2d"};
    test<dim>(grid, pvd_writer);
}

template<std::size_t dim, typename Grid>
void test_from_abstract_base_ptr(const Grid& grid) {
    using BaseWriter = GridFormat::TimeSeriesGridWriter<Grid>;
    auto writer = GridFormat::PVDWriter{GridFormat::VTUWriter{grid}, "pvd_time_series_from_base_writer_2d_in_2d"};
    std::unique_ptr<BaseWriter> pvd_writer = std::make_unique<decltype(writer)>(std::move(writer));
    test<dim>(grid, *pvd_writer);
}

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();
    test_from_instance<2>(grid);
    test_from_abstract_base_ptr<2>(grid);
    return 0;
}
