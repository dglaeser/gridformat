#include <vector>
#include <cmath>

#include "../grid/unstructured_grid.hpp"
#include <gridformat/vtk/vtu_writer.hpp>

template<typename T, typename Position>
T evaluate_function(const Position& pos) {
    return 10.0*std::sin(pos[0])*std::cos(pos[1]);
}

template<typename T, typename Grid>
std::vector<T> make_point_data(const Grid& grid) {
    std::vector<T> result;
    result.reserve(GridFormat::Grid::num_points(grid));
    for (const auto& p : GridFormat::Grid::points(grid))
        result.push_back(evaluate_function<T>(
            GridFormat::Grid::coordinates(grid, p)
        ));
    return result;
}

template<typename T, typename Grid>
std::vector<T> make_cell_data(const Grid& grid) {
    std::vector<T> result;
    result.reserve(GridFormat::Grid::num_cells(grid));
    for (const auto& c : GridFormat::Grid::cells(grid))
        result.push_back(evaluate_function<T>(
            GridFormat::Grid::coordinates(
                grid,
                *begin(GridFormat::Grid::corners(grid, c))
            )
        ));
    return result;
}

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();
    GridFormat::VTUWriter writer{grid};

    auto double_point_data = make_point_data<double>(grid);
    auto double_cell_data = make_cell_data<double>(grid);
    writer.set_point_data("double_values", double_point_data);
    writer.set_cell_data("double_values", double_cell_data);

    writer.write("file.vtu");
    return 0;
}