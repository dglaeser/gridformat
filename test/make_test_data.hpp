// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_
#define GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_

#include <vector>
#include <cmath>

#include <gridformat/grid.hpp>

namespace GridFormat::Test {

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

}  // namespace GridFormat::Test

#endif  // GRIDFORMAT_TEST_MAKE_TEST_DATA_HPP_