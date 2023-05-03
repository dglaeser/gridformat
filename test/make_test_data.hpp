// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_
#define GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_

#include <vector>
#include <cmath>
#include <algorithm>

#include <gridformat/grid/writer.hpp>
#include <gridformat/grid.hpp>

// test grids
#include "grid/unstructured_grid.hpp"
#include "grid/structured_grid.hpp"

namespace GridFormat::Test {

template<int space_dim, typename Cell>
auto _compute_cell_center(const GridFormat::Test::StructuredGrid<space_dim>& g, const Cell& c) {
    return g.center(c);
}

template<int dim, int space_dim, typename Cell>
auto _compute_cell_center(const GridFormat::Test::UnstructuredGrid<dim, space_dim>& g, const Cell& c) {
    std::array<double, space_dim> result;
    std::ranges::fill(result, 0.0);
    for (std::size_t i = 0; i < c.corners.size(); ++i)
        for (std::size_t dir = 0; dir < space_dim; ++dir)
            result[dir] += g.points()[c.corners[i]].coordinates[dir];
    std::ranges::for_each(result, [&] (auto& value) {
        value /= static_cast<double>(c.corners.size());
    });
    return result;
}

template<int dim, typename Point>
auto _get_point_coordinates(const GridFormat::Test::StructuredGrid<dim>& g, const Point& p) {
    return g.center(p);
}

template<int dim, int space_dim, typename Point>
auto _get_point_coordinates(const GridFormat::Test::UnstructuredGrid<dim, space_dim>&, const Point& p) {
    return p.coordinates;
}

template<typename T, typename Position>
T test_function(const Position& pos) {
    T result = 10.0*std::sin(pos[0]);
    if (pos.size() > 1)
        result *= std::cos(pos[1]);
    if (pos.size() > 2)
        result *= pos[2] + 1.0;
    return result;
}

template<typename T, typename Grid>
std::vector<T> make_point_data(const Grid& grid) {
    std::vector<T> result(GridFormat::number_of_points(grid), T{0.0});
    for (const auto& p : GridFormat::points(grid))
        result[p.id] = test_function<T>(_get_point_coordinates(grid, p));
    return result;
}

template<typename T, typename Grid>
std::vector<T> make_cell_data(const Grid& grid) {
    std::vector<T> result;
    result.resize(GridFormat::number_of_cells(grid), T{0.0});
    for (const auto& c : GridFormat::cells(grid))
        result[c.id] = test_function<T>(
            _compute_cell_center(grid, c)
        );
    return result;
}

template<std::size_t dim, typename T>
auto make_vector_data(const std::vector<T>& scalars) {
    using Vector = std::array<T, dim>;
    std::vector<Vector> result(scalars.size());
    std::transform(scalars.begin(), scalars.end(), result.begin(), [] (T value) {
        Vector v;
        std::ranges::fill(v, value);
        return v;
    });
    return result;
}

template<std::size_t dim, typename T>
auto make_tensor_data(const std::vector<T>& scalars) {
    using Vector = std::array<T, dim>;
    using Tensor = std::array<Vector, dim>;
    std::vector<Tensor> result(scalars.size());
    std::transform(scalars.begin(), scalars.end(), result.begin(), [] (T value) {
        Vector v;
        Tensor t;
        std::ranges::fill(v, value);
        std::ranges::fill(t, v);
        return t;
    });
    return result;
}

template<typename T, std::size_t dim>
struct TestData {
    std::vector<T> point_scalars;
    std::vector<T> cell_scalars;
    std::vector<std::array<T, dim>> point_vectors;
    std::vector<std::array<T, dim>> cell_vectors;
    std::vector<std::array<std::array<T, dim>, dim>> point_tensors;
    std::vector<std::array<std::array<T, dim>, dim>> cell_tensors;
};

template<std::size_t dim, typename T, typename Grid>
TestData<T, dim> make_test_data(const Grid& grid, double timestep = 1.0) {
    auto point_data = make_point_data<double>(grid);
    auto cell_data = make_cell_data<double>(grid);

    // scale with time
    std::ranges::for_each(point_data, [&] (auto& value) { value *= timestep; });
    std::ranges::for_each(cell_data, [&] (auto& value) { value *= timestep; });

    return {
        .point_scalars = point_data,
        .cell_scalars = cell_data,
        .point_vectors = make_vector_data<dim>(point_data),
        .cell_vectors = make_vector_data<dim>(cell_data),
        .point_tensors = make_tensor_data<dim>(point_data),
        .cell_tensors = make_tensor_data<dim>(cell_data)
    };
}

template<typename Grid, typename T, std::size_t dim, typename FieldPrec>
void add_test_data(GridWriterBase<Grid>& writer,
                   const TestData<T, dim>& data,
                   const Precision<FieldPrec>& prec) {
    writer.set_point_field("pscalar", [&] (const auto& p) { return data.point_scalars[p.id]; });
    writer.set_point_field("pvector", [&] (const auto& p) { return data.point_vectors[p.id]; });
    writer.set_point_field("ptensor", [&] (const auto& p) { return data.point_tensors[p.id]; });

    writer.set_cell_field("cscalar", [&] (const auto& c) { return data.cell_scalars[c.id]; });
    writer.set_cell_field("cvector", [&] (const auto& c) { return data.cell_vectors[c.id]; });
    writer.set_cell_field("ctensor", [&] (const auto& c) { return data.cell_tensors[c.id]; });

    writer.set_point_field("pscalar_custom_prec", [&] (const auto& p) { return data.point_scalars[p.id]; }, prec);
    writer.set_point_field("pvector_custom_prec", [&] (const auto& p) { return data.point_vectors[p.id]; }, prec);
    writer.set_point_field("ptensor_custom_prec", [&] (const auto& p) { return data.point_tensors[p.id]; }, prec);

    writer.set_cell_field("cscalar_custom_prec", [&] (const auto& c) { return data.cell_scalars[c.id]; }, prec);
    writer.set_cell_field("cvector_custom_prec", [&] (const auto& c) { return data.cell_vectors[c.id]; }, prec);
    writer.set_cell_field("ctensor_custom_prec", [&] (const auto& c) { return data.cell_tensors[c.id]; }, prec);
}

}  // namespace GridFormat::Test

#endif  // GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_
