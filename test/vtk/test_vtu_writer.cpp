#include <vector>
#include <cmath>
#include <fstream>

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
                *begin(GridFormat::corners(grid, c))
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
    const auto grid = GridFormat::Test::make_unstructured_2d();
    auto point_scalars = make_point_data<double>(grid);
    auto point_vectors = make_vector_data(point_scalars);
    auto point_tensors = make_tensor_data(point_scalars);
    auto cell_scalars = make_cell_data<double>(grid);
    auto cell_vectors = make_vector_data(cell_scalars);
    auto cell_tensors = make_tensor_data(cell_scalars);

    GridFormat::VTK::XMLOptions xml_opts{
        .encoder = GridFormat::Encoding::base64,
        .compression = GridFormat::Compression::lzma,
        .format = GridFormat::VTK::DataFormat::inlined
    };

    GridFormat::VTK::PrecisionOptions prec_opts{
        GridFormat::default_precision,
        GridFormat::default_precision
    };

    GridFormat::VTUWriter writer{grid, xml_opts, prec_opts};
    writer.set_point_field("pscalar", point_scalars);
    writer.set_point_field("pvector", point_vectors);
    writer.set_point_field("ptensor", point_tensors);
    writer.set_cell_field("cscalar", cell_scalars);
    writer.set_cell_field("cvector", cell_vectors);
    writer.set_cell_field("ctensor", cell_tensors);

    writer.write("file");

    return 0;
}