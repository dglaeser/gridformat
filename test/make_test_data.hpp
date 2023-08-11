// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
#ifndef GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_
#define GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>

#include <gridformat/common/range_field.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/grid/type_traits.hpp>
#include <gridformat/grid/writer.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/grid.hpp>

// test grids
#include "grid/unstructured_grid.hpp"
#include "grid/structured_grid.hpp"

namespace GridFormat::Test {

template<Concepts::UnstructuredGrid Grid>
auto evaluation_position(const Grid& g, const GridFormat::Cell<Grid>& cell) {
    std::array<CoordinateType<Grid>, space_dimension<Grid>> center;
    std::ranges::fill(center, CoordinateType<Grid>{0});
    for (const auto& p : points(g, cell)) {
        const auto coords = coordinates(g, p);
        std::transform(
            std::ranges::begin(coords),
            std::ranges::end(coords),
            center.begin(),
            center.begin(),
            std::plus{}
        );
    }
    std::ranges::for_each(center, [n=number_of_points(g, cell)] (auto& c) { c /= n; });
    return center;
}

template<Concepts::UnstructuredGrid Grid>
auto evaluation_position(const Grid& g, const GridFormat::Point<Grid>& point) {
    std::array<CoordinateType<Grid>, space_dimension<Grid>> position;
    std::ranges::fill(position, CoordinateType<Grid>{0});
    std::ranges::copy(coordinates(g, point), position.begin());
    return position;
}

// overload for structured test grids exposing the entity centers
template<Concepts::Grid Grid, typename Entity>
    requires(!Concepts::UnstructuredGrid<Grid> and
             requires(const Grid& g, const Entity& e) {
                { g.center(e) } -> std::ranges::range;
             })
auto evaluation_position(const Grid& g, const Entity& e) {
    std::array<CoordinateType<Grid>, space_dimension<Grid>> center;
    std::ranges::fill(center, CoordinateType<Grid>{0});
    std::ranges::copy(g.center(e), center.begin());
    return center;
}

template<typename T, typename Position, typename T3 = double>
T test_function(const Position& pos, const T3& time_at_step = 1.0) {
    T result = T{10}*static_cast<T>(std::sin(pos[0]));
    if (pos.size() > 1)
        result *= static_cast<T>(std::cos(pos[1]));
    if (pos.size() > 2)
        result *= static_cast<T>(pos[2]) + T{1};
    return result*static_cast<T>(time_at_step);
}

template<typename T, typename Grid, typename T2 = double>
std::vector<T> make_point_data(const Grid& grid, const T2& time_at_step = 1.0) {
    std::vector<T> result(GridFormat::number_of_points(grid), T{0.0});
    for (const auto& p : GridFormat::points(grid))
        result[p.id] = test_function<T>(evaluation_position(grid, p), time_at_step);
    return result;
}

template<typename T, typename Grid, typename T2 = double>
std::vector<T> make_cell_data(const Grid& grid, const T2& time_at_step = 1.0) {
    std::vector<T> result;
    result.resize(GridFormat::number_of_cells(grid), T{0.0});
    for (const auto& c : GridFormat::cells(grid))
        result[c.id] = test_function<T>(evaluation_position(grid, c), time_at_step);
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
TestData<T, dim> make_test_data(const Grid& grid,
                                const Precision<T>&,
                                double time_at_step = 1.0) {
    auto point_data = make_point_data<T>(grid, time_at_step);
    auto cell_data = make_cell_data<T>(grid, time_at_step);

    return {
        .point_scalars = point_data,
        .cell_scalars = cell_data,
        .point_vectors = make_vector_data<dim>(point_data),
        .cell_vectors = make_vector_data<dim>(cell_data),
        .point_tensors = make_tensor_data<dim>(point_data),
        .cell_tensors = make_tensor_data<dim>(cell_data)
    };
}

template<typename Grid, typename T, std::size_t dim, typename T2>
void add_test_point_data(GridWriterBase<Grid>& writer,
                         const TestData<T, dim>& data,
                         const Precision<T2>& custom_precision) {
    writer.set_point_field("pscalar", [&] (const auto& p) { return data.point_scalars[p.id]; });
    writer.set_point_field("pvector", [&] (const auto& p) { return data.point_vectors[p.id]; });
    writer.set_point_field("ptensor", [&] (const auto& p) { return data.point_tensors[p.id]; });

    writer.set_point_field("pscalar_custom_prec", [&] (const auto& p) { return data.point_scalars[p.id]; }, custom_precision);
    writer.set_point_field("pvector_custom_prec", [&] (const auto& p) { return data.point_vectors[p.id]; }, custom_precision);
    writer.set_point_field("ptensor_custom_prec", [&] (const auto& p) { return data.point_tensors[p.id]; }, custom_precision);
}

template<typename Grid, typename T, std::size_t dim, typename T2>
void add_test_cell_data(GridWriterBase<Grid>& writer,
                        const TestData<T, dim>& data,
                        const Precision<T2>& custom_precision) {
    writer.set_cell_field("cscalar", [&] (const auto& c) { return data.cell_scalars[c.id]; });
    writer.set_cell_field("cvector", [&] (const auto& c) { return data.cell_vectors[c.id]; });
    writer.set_cell_field("ctensor", [&] (const auto& c) { return data.cell_tensors[c.id]; });

    writer.set_cell_field("cscalar_custom_prec", [&] (const auto& c) { return data.cell_scalars[c.id]; }, custom_precision);
    writer.set_cell_field("cvector_custom_prec", [&] (const auto& c) { return data.cell_vectors[c.id]; }, custom_precision);
    writer.set_cell_field("ctensor_custom_prec", [&] (const auto& c) { return data.cell_tensors[c.id]; }, custom_precision);
}

template<typename Grid, typename T, std::size_t dim, typename FieldPrec>
void add_test_data(GridWriterBase<Grid>& writer,
                   const TestData<T, dim>& data,
                   const Precision<FieldPrec>& custom_precision) {
    add_test_point_data(writer, data, custom_precision);
    add_test_cell_data(writer, data, custom_precision);
}

template<typename Writer>
void add_meta_data(Writer& w) {
    w.set_meta_data("literal", "some_literal_text");
    w.set_meta_data("string", std::string{"some_string_text"});
    w.set_meta_data("numbers", RangeField{std::vector<int>{1, 2, 3, 4}});
}

template<typename Writer>
void add_discontinuous_point_field(Writer& w) {
    w.set_point_field("cell_index", [] (const auto& p) {
        return p.cell().index();
    });
}


struct TestFileOptions {
    bool write_point_data = true;
    bool write_cell_data = true;
    bool write_meta_data = true;
};


template<std::size_t space_dim, typename Grid, typename T1 = double, typename T2 = float>
std::string write_test_file(GridWriter<Grid>& writer,
                            const std::string& filename,
                            const TestFileOptions& opts = {},
                            const bool verbose = true,
                            const Precision<T1>& main_precision = {},
                            const Precision<T2>& custom_precision = {}) {
    const auto test_data = make_test_data<space_dim>(writer.grid(), main_precision);
    if (opts.write_point_data)
        add_test_point_data(writer, test_data, custom_precision);
    if (opts.write_cell_data)
        add_test_cell_data(writer, test_data, custom_precision);
    if (opts.write_meta_data)
        add_meta_data(writer);
    const auto filename_with_ext = writer.write(filename);
    if (verbose)
        std::cout << "Wrote '" << as_highlight(filename_with_ext) << "'" << std::endl;
    return filename_with_ext;
}

template<std::size_t space_dim, typename Grid, typename T1 = double, typename T2 = float>
std::string write_test_time_series(TimeSeriesGridWriter<Grid>& writer,
                                   const std::size_t num_steps = 5,
                                   const TestFileOptions& opts = {},
                                   const bool verbose = true,
                                   const Precision<T1>& main_precision = {},
                                   const Precision<T2>& custom_precision = {}) {
    if (opts.write_meta_data)
        add_meta_data(writer);

    std::string filename_with_ext;
    std::vector<double> times;
    std::ranges::for_each(
        std::views::iota(std::size_t{0}, num_steps),
        [&, dt=1.0/static_cast<double>(num_steps - 1)] (std::size_t i) {
            times.push_back(static_cast<double>(i)*dt);
    });

    auto test_data = make_test_data<space_dim>(writer.grid(), main_precision, 0.);
    if (opts.write_point_data)
        add_test_point_data(writer, test_data, custom_precision);
    if (opts.write_cell_data)
        add_test_cell_data(writer, test_data, custom_precision);

    std::ranges::for_each(times, [&] (double t) {
        test_data = make_test_data<space_dim>(writer.grid(), main_precision, t);
        filename_with_ext = writer.write(t);
        if (verbose)
            std::cout << "Wrote '" << as_highlight(filename_with_ext) << "' at t = " << t << std::endl;
    });
    return filename_with_ext;
}

}  // namespace GridFormat::Test

#endif  // GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_
