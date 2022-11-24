// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_
#define GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_

#include <vector>
#include <cmath>
#include <algorithm>

#include <gridformat/grid.hpp>

namespace GridFormat::Test {

template<typename Grid, typename Cell>
auto _compute_cell_center(const Grid& g, const Cell& c) {
    std::array<double, Grid::space_dimension> result;
    std::ranges::fill(result, 0.0);
    for (std::size_t i = 0; i < c.corners.size(); ++i)
        for (std::size_t dir = 0; dir < Grid::space_dimension; ++dir)
            result[dir] += g.points()[c.corners[i]].coordinates[dir];
    std::ranges::for_each(result, [&] (auto& value) {
        value /= static_cast<double>(c.corners.size());
    });
    return result;
}

template<typename T, typename Position>
T test_function(const Position& pos) {
    return 10.0*std::sin(pos[0])*std::cos(pos[1]);
}

template<typename T, typename Grid>
std::vector<T> make_point_data(const Grid& grid) {
    std::vector<T> result(GridFormat::number_of_points(grid));
    for (const auto& p : GridFormat::points(grid))
        result[GridFormat::id(grid, p)] = test_function<T>(
            GridFormat::coordinates(grid, p)
        );
    return result;
}

template<typename T, typename Grid>
std::vector<T> make_cell_data(const Grid& grid) {
    std::vector<T> result;
    result.reserve(GridFormat::number_of_cells(grid));
    for (const auto& c : GridFormat::cells(grid))
        result.push_back(test_function<T>(
            _compute_cell_center(grid, c)
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

}  // namespace GridFormat::Test

#endif  // GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_
