#include <vector>
#include <cmath>
#include <fstream>
#include <chrono>

#include "../grid/unstructured_grid.hpp"
#include <gridformat/vtk/vtu_writer.hpp>

template<typename T, typename Position>
T evaluate_function(const Position& pos) {
    return 10.0*std::sin(pos[0])*std::cos(pos[1]);
}

template<typename T, typename Grid>
std::vector<T> make_point_data(const Grid& grid) {
    std::vector<T> result;
    result.reserve(GridFormat::number_of_points(grid));
    for (const auto& p : GridFormat::points(grid))
        result.push_back(evaluate_function<T>(
            GridFormat::coordinates(grid, p)
        ));
    return result;
}

template<typename T, typename Grid>
std::vector<T> make_cell_data(const Grid& grid) {
    std::vector<T> result;
    result.reserve(GridFormat::number_of_cells(grid));
    for (const auto& c : GridFormat::cells(grid))
        result.push_back(evaluate_function<T>(
            GridFormat::coordinates(
                grid,
                *begin(GridFormat::points(grid, c))
            )
        ));
    return result;
}

template<typename T>
auto make_vector_data(const std::vector<T>& scalars) {
    using Vector = std::array<T, 2>;
    std::vector<Vector> result(scalars.size());
    std::transform(scalars.begin(), scalars.end(), result.begin(), [] (T value) {
        return Vector{value, value};
    });
    return result;
}

template<typename T>
auto make_tensor_data(const std::vector<T>& scalars) {
    using Vector = std::array<T, 2>;
    using Tensor = std::array<Vector, 2>;
    std::vector<Tensor> result(scalars.size());
    std::transform(scalars.begin(), scalars.end(), result.begin(), [] (T value) {
        return Tensor{
            Vector{value, value},
            Vector{value, value}
        };
    });
    return result;
}

int main() {
    using Grid = GridFormat::Test::UnstructuredGrid<2>;
    using Point = typename Grid::Point;
    using Cell = typename Grid::Cell;

    std::vector<Point> points;
    std::vector<Cell> cells;

    const std::size_t num_cells = 1023;
    const double dx = 1.0/static_cast<double>(num_cells);
    for (std::size_t i = 0; i < num_cells + 1; ++i)
        for (std::size_t j = 0; j < num_cells + 1; ++j)
            points.emplace_back(Point{
                {static_cast<double>(i)*dx, static_cast<double>(j)*dx},
                i*(num_cells+1) + j
            });
    for (std::size_t i = 0; i < num_cells; ++i)
        for (std::size_t j = 0; j < num_cells; ++j) {
            const std::size_t p0 = i*(num_cells+1) + j;
            cells.emplace_back(Cell{
                {p0, p0 + 1, p0 + num_cells + 2, p0 + num_cells + 1},
                GridFormat::CellType::quadrilateral,
                i*num_cells + j
            });
        }

    Grid grid{std::move(points), std::move(cells)};
    auto point_scalars = make_point_data<double>(grid);
    auto cell_scalars = make_cell_data<double>(grid);

    GridFormat::VTUWriter writer{
        grid,
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::raw_binary,
            .compression = GridFormat::none,
            .format = GridFormat::VTK::DataFormat::appended
        },
        GridFormat::VTK::PrecisionOptions{
            .coordinate_precision = GridFormat::float64,
            .header_precision = GridFormat::uint32
        }
    };

    for (int i = 0; i < 5; ++i) {
        writer.set_point_field("pscalar_" + std::to_string(i), [&] (const auto& p) { return point_scalars[p.id]; }, GridFormat::float64);
        writer.set_cell_field("cscalar_" + std::to_string(i), [&] (const auto& c) { return cell_scalars[c.id]; }, GridFormat::float64);
    }

    auto t1 = std::chrono::steady_clock::now();
    writer.write("file");
    auto t2 = std::chrono::steady_clock::now();
    std::cout << "Write took " << std::chrono::duration<double>(t2-t1).count() << std::endl;

    return 0;
}
