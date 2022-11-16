#include <vector>
#include <cmath>
#include <string>

#include <gridformat/compression/lzma.hpp>
#include <gridformat/encoding/base64.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include "../grid/unstructured_grid.hpp"

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

template<typename XMLOptions,
         typename PrecisionOptions,
         typename FieldPrecision = GridFormat::Precision<double>>
void write(const XMLOptions& xml_opts,
           const PrecisionOptions& prec_opts,
           const std::string& filename,
           const FieldPrecision& prec = {}) {
    const auto grid = GridFormat::Test::make_unstructured_2d();
    auto point_scalars = make_point_data<double>(grid);
    auto point_vectors = make_vector_data(point_scalars);
    auto point_tensors = make_tensor_data(point_scalars);
    auto cell_scalars = make_cell_data<double>(grid);
    auto cell_vectors = make_vector_data(cell_scalars);
    auto cell_tensors = make_tensor_data(cell_scalars);

    GridFormat::VTUWriter writer{grid, xml_opts, prec_opts};
    writer.set_point_field("pscalar", [&] (const auto& p) { return point_scalars[p.id]; });
    writer.set_point_field("pvector", [&] (const auto& p) { return point_vectors[p.id]; });
    writer.set_point_field("ptensor", [&] (const auto& p) { return point_tensors[p.id]; });
    writer.set_cell_field("cscalar", [&] (const auto& c) { return cell_scalars[c.id]; });
    writer.set_cell_field("cvector", [&] (const auto& c) { return cell_vectors[c.id]; });
    writer.set_cell_field("ctensor", [&] (const auto& c) { return cell_tensors[c.id]; });

    writer.set_point_field("pscalar_custom_prec", [&] (const auto& p) { return point_scalars[p.id]; }, prec);
    writer.set_point_field("pvector_custom_prec", [&] (const auto& p) { return point_vectors[p.id]; }, prec);
    writer.set_point_field("ptensor_custom_prec", [&] (const auto& p) { return point_tensors[p.id]; }, prec);
    writer.set_cell_field("cscalar_custom_prec", [&] (const auto& c) { return cell_scalars[c.id]; }, prec);
    writer.set_cell_field("cvector_custom_prec", [&] (const auto& c) { return cell_vectors[c.id]; }, prec);
    writer.set_cell_field("ctensor_custom_prec", [&] (const auto& c) { return cell_tensors[c.id]; }, prec);

    std::cout << "Writing " << filename << ".vtu" << std::endl;
    writer.write(filename);
}

int main() {
    write(GridFormat::VTK::XMLOptions{}, GridFormat::VTK::PrecisionOptions{}, "vtu_default");
    write(
        GridFormat::VTK::XMLOptions{.encoder = GridFormat::Encoding::ascii},
        GridFormat::VTK::PrecisionOptions{},
        "vtu_ascii"
    );
    write(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .data_format = GridFormat::VTK::DataFormat::inlined
        },
        GridFormat::VTK::PrecisionOptions{},
        "vtu_base64_inlined"
    );
    write(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .compression = GridFormat::Compression::lzma,
            .data_format = GridFormat::VTK::DataFormat::inlined
        },
        GridFormat::VTK::PrecisionOptions{},
        "vtu_base64_inlined_lzma_compression"
    );
    write(
        GridFormat::VTK::XMLOptions{
            .encoder = GridFormat::Encoding::base64,
            .compression = GridFormat::Compression::lzma,
            .data_format = GridFormat::VTK::DataFormat::inlined
        },
        GridFormat::VTK::PrecisionOptions{},
        "vtu_base64_inlined_lzma_compression_custom_field_precision",
        GridFormat::Precision<float>{}
    );

    return 0;
}