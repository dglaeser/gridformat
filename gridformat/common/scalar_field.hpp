// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_SCALAR_FIELD_HPP_
#define GRIDFORMAT_COMMON_SCALAR_FIELD_HPP_

#include <utility>
#include <ostream>
#include <ranges>
#include <algorithm>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/range_formatter.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief TODO: Doc me.
 */
template<Concepts::ScalarView View,
         typename ValueType = std::ranges::range_value_t<View>> requires(
            std::ranges::input_range<View>)
class ScalarField : public Field {
public:
    using typename Field::Serialization;

    explicit ScalarField(View&& view, RangeFormatter formatter = RangeFormatter{})
    : _view(std::move(view))
    , _formatter(std::move(formatter))
    {}

    ScalarField(View&& view,
                [[maybe_unused]] const Precision<ValueType>& prec,
                RangeFormatter formatter = RangeFormatter{})
    : _view(std::move(view))
    , _formatter(std::move(formatter))
    {}

 private:
    View _view;
    RangeFormatter _formatter;

    void _stream(std::ostream& stream) const override {
        _formatter.write(stream, _view);
    }

    Serialization _serialized() const override {
        const auto num_values = std::ranges::distance(std::ranges::begin(_view), std::ranges::end(_view));
        const auto num_bytes = num_values*sizeof(ValueType);

        Serialization serialization(num_bytes);
        ValueType* data = reinterpret_cast<ValueType*>(serialization.data());

        if constexpr (std::same_as<std::ranges::range_value_t<View>, ValueType>)
            std::ranges::copy(_view, data);
        else
            std::ranges::copy(
                std::views::transform(_view, [] (const auto& value) {
                    return cast_to(Precision<ValueType>{}, value);
                }),
                data
            );

        return serialization;
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_SCALAR_FIELD_HPP_