// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_WRITER_HPP_
#define GRIDFORMAT_COMMON_WRITER_HPP_

#include <string>
#include <utility>
#include <ranges>
#include <type_traits>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/streams.hpp>

#include <gridformat/common/field_storage.hpp>
#include <gridformat/common/fields.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace Detail {

template<typename T>
inline constexpr bool is_rvalue_view_or_lvalue_range =
    std::ranges::range<T> and
    (std::ranges::view<T> and !std::is_lvalue_reference_v<T>) or
    (!std::ranges::view<T> and std::is_lvalue_reference_v<T> and std::ranges::viewable_range<T>);

}  // namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
class Writer {
    using FieldStorage = GridFormat::FieldStorage<>;

 public:
    using Field = typename FieldStorage::Field;

    Writer() = default;
    explicit Writer(RangeFormatOptions opts)
    : _format_opts(std::move(opts))
    {}

    // template<Concepts::Tensors R, typename T = MDRangeScalar<R>> requires(Detail::is_rvalue_view_or_lvalue_range<R>)
    // void set_point_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
    //     _point_data.set(name, TensorField{std::views::all(std::forward<R>(range)), prec, _format_opts});
    // }

    template<Concepts::Vectors R, typename T = MDRangeScalar<R>> requires(Detail::is_rvalue_view_or_lvalue_range<R>)
    void set_point_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        _point_data.set(name, VectorField{std::views::all(std::forward<R>(range)), prec, _format_opts});
    }

    template<Concepts::Scalars R, typename T = std::ranges::range_value_t<R>> requires(Detail::is_rvalue_view_or_lvalue_range<R>)
    void set_point_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        _point_data.set(name, ScalarField{std::views::all(std::forward<R>(range)), prec, _format_opts});
    }

//    template<Concepts::Tensors R, typename T = MDRangeScalar<R>> requires(Detail::is_rvalue_view_or_lvalue_range<R>)
//     void set_cell_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
//         _cell_data.set(name, TensorField{std::views::all(std::forward<R>(range)), prec, _format_opts});
//     }

    template<Concepts::Vectors R, typename T = MDRangeScalar<R>> requires(Detail::is_rvalue_view_or_lvalue_range<R>)
    void set_cell_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        _cell_data.set(name, VectorField{std::views::all(std::forward<R>(range)), prec, _format_opts});
    }

    template<Concepts::Scalars R, typename T = std::ranges::range_value_t<R>> requires(Detail::is_rvalue_view_or_lvalue_range<R>)
    void set_cell_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        _cell_data.set(name, ScalarField{std::views::all(std::forward<R>(range)), prec, _format_opts});
    }

 protected:
    std::ranges::range auto _point_data_names() const {
        return _point_data.field_names();
    }

    std::ranges::range auto _cell_data_names() const {
        return _cell_data.field_names();
    }

    const Field& _get_point_data(const std::string& name) const {
        return _point_data.get(name);
    }

    const Field& _get_cell_data(const std::string& name) const {
        return _cell_data.get(name);
    }

    FieldStorage _point_data;
    FieldStorage _cell_data;
    RangeFormatOptions _format_opts;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_WRITER_HPP_