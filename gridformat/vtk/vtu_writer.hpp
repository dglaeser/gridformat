// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_VTU_WRITER_HPP_
#define GRIDFORMAT_VTK_VTU_WRITER_HPP_

#include <ranges>
#include <unordered_map>

#include <gridformat/common/scalar.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief TODO: Doc me (mention max_chars_per_line neglects indentation)
 */
class VTKWriter {
 public:
    template<std::ranges::forward_range R>
    void set_point_data(std::string_view name, R&& range) {
        using T = MDRangeScalarType<R>;
        set_point_data(name, std::forward<R>(range), Scalar<T>{});
    }

    template<std::ranges::forward_range R>
    void set_point_data(std::string_view name, R&& range, const Scalar<T>&) {
        // TODO
    }

    template<std::ranges::forward_range R>
    void set_cell_data(std::string_view name, R&& range) {
        using T = MDRangeScalarType<R>;
        set_cell_data(name, std::forward<R>(range), Scalar<T>{});
    }

    template<std::ranges::forward_range R>
    void set_cell_data(std::string_view name, R&& range, const Scalar<T>&) {
        // TODO
    }

 private:

};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTU_WRITER_HPP_