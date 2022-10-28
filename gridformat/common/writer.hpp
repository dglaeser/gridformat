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

#include <gridformat/common/field_storage.hpp>
#include <gridformat/common/scalar_field.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/concepts.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
class Writer {
    using FieldStorage = GridFormat::FieldStorage<>;

 public:
    using Field = typename FieldStorage::Field;

    template<Concepts::TensorRange R, typename T = MDRangeScalar<R>>
    void set_point_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        set_point_data(name, std::views::join(range), prec);
    }

    template<Concepts::VectorRange R, typename T = MDRangeScalar<R>>
    void set_point_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        set_point_data(name, std::views::join(range), prec);
    }

    template<Concepts::ScalarRange R, typename T = std::ranges::range_value_t<R>>
    void set_point_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        _set(_point_data, name, std::forward<R>(range), prec);
    }

    template<Concepts::TensorRange R, typename T = MDRangeScalar<R>>
    void set_cell_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        set_cell_data(name, std::views::join(range), prec);
    }

    template<Concepts::VectorRange R, typename T = MDRangeScalar<R>>
    void set_cell_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        set_cell_data(name, std::views::join(range), prec);
    }

    template<Concepts::ScalarRange R, typename T = std::ranges::range_value_t<R>>
    void set_cell_data(const std::string& name, R&& range, const Precision<T>& prec = {}) {
        _set(_cell_data, name, std::forward<R>(range), prec);
    }

 protected:
    const Field& get_point_data(const std::string& name) const {
        return _point_data.get(name);
    }

    const Field& get_cell_data(const std::string& name) const {
        return _cell_data.get(name);
    }

 private:
    template<Concepts::ScalarRange R, typename T> requires(std::is_lvalue_reference_v<R>)
    void _set(FieldStorage& storage,
              const std::string& name,
              R&& input_range,
              const Precision<T>& prec) {
        _set(storage, name, std::ranges::ref_view{input_range}, prec);
    }

    template<Concepts::ScalarView R, typename T> requires(!std::is_lvalue_reference_v<R>)
    void _set(FieldStorage& storage,
              const std::string& name,
              R&& input_range,
              const Precision<T>& prec) {
        storage.set(
            name,
            GridFormat::ScalarField{std::views::transform(
                input_range,
                [&] (const auto& value) { return cast_to(prec, value); }
            )}
        );
    }

    FieldStorage _point_data;
    FieldStorage _cell_data;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_WRITER_HPP_