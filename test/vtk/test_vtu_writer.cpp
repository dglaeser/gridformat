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

template<typename T>
std::vector<std::array<T, 2>> make_vector_data(const std::vector<T>& scalars) {
    std::vector<std::array<T, 2>> result(scalars.size());
    std::transform(scalars.begin(), scalars.end(), result.begin(), [] (T value) {
        return std::array<T, 2>{value, value};
    });
    return result;
}

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();
    auto point_scalars = make_point_data<double>(grid);
    auto point_vectors = make_vector_data(point_scalars);
    auto cell_scalars = make_cell_data<double>(grid);
    auto cell_vectors = make_vector_data(cell_scalars);

    GridFormat::VTUWriter writer{grid};
    writer.set_point_field("pscalar", point_scalars);
    writer.set_cell_field("cscalar", cell_scalars);
    writer.set_point_field("pvector", point_vectors);
    writer.set_cell_field("cvector", cell_vectors);

    writer.write(std::cout);

    return 0;
}