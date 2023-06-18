// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::Matrix
 */
#ifndef GRIDFORMAT_COMMON_MATRIX_HPP_
#define GRIDFORMAT_COMMON_MATRIX_HPP_

#include <array>
#include <ranges>
#include <utility>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/type_traits.hpp>

namespace GridFormat {


template<typename T, std::size_t rows, std::size_t cols>
class Matrix {
 public:
    template<Concepts::StaticallySizedMDRange<2> R>
    explicit Matrix(R&& entries) {
        static_assert(static_size<R> == rows);
        static_assert(static_size<std::ranges::range_value_t<R>> == cols);

        int i = 0;
        std::ranges::for_each(entries, [&] (const auto& sub_range) {
            std::ranges::copy(sub_range, _entries[i]);
            i++;
        });
    }

    void transpose() {
        for (std::size_t row = 0; row < rows; ++row)
            for (std::size_t col = row+1; col < cols; ++col)
                std::swap(_entries[row][col], _entries[col][row]);
    }

    Matrix& transposed() {
        transpose();
        return *this;
    }

    auto begin() const { return std::ranges::begin(_entries); }
    auto end() const { return std::ranges::end(_entries); }

 private:
    T _entries[rows][cols];
};


template<Concepts::StaticallySizedMDRange<2> Range>
Matrix(Range&&) -> Matrix<
    MDRangeValueType<Range>,
    static_size<Range>,
    static_size<std::ranges::range_value_t<Range>>
>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_MATRIX_HPP_
