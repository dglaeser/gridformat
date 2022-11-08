// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_FIELDS_HPP_
#define GRIDFORMAT_COMMON_FIELDS_HPP_

#include <cassert>
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

#ifndef DOXYGEN
namespace Fields::Detail {

int _get_range_size(const std::ranges::range auto& range) {
    return std::ranges::distance(range);
}

int _get_number_of_components(const Concepts::Vector auto& vector) {
    return _get_range_size(vector);
}

int _get_number_of_components(const Concepts::Tensor auto& tensor) {
    const auto nrows = _get_range_size(tensor);
    const auto ncols = _get_number_of_components(*std::ranges::begin(tensor));
    assert(std::ranges::all_of(tensor, [&] (const Concepts::Vector auto& row) {
        return _get_number_of_components(row) == ncols;
    }) && "Cannot process tensors with varying row sizes!");
    return nrows*ncols;
}

template<std::ranges::range R> requires(Concepts::Vectors<R> or Concepts::Tensors<R>)
int _deduce_number_of_components(const R& range) {
    const auto ncomps = _get_number_of_components(*std::ranges::begin(range));
    assert(std::ranges::all_of(range, [&] (const auto& sub_range) {
        return _get_number_of_components(sub_range) == ncomps;
    }) && "Cannot process ranges whose elements have varying number of components");
    return ncomps;
}

}  // namespace Fields::Detail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief TODO: Doc me.
 */
template<Concepts::ScalarsView View,
         typename ValueType = std::ranges::range_value_t<View>> requires(std::ranges::input_range<View>)
class ScalarField : public Field {
    static constexpr int num_components = 1;

public:
    using typename Field::Serialization;

    explicit ScalarField(View&& view, RangeFormatter formatter = RangeFormatter{})
    : Field(num_components, as_dynamic(Precision<ValueType>{}))
    , _view(std::move(view))
    , _formatter(std::move(formatter))
    {}

    ScalarField(View&& view,
                [[maybe_unused]] const Precision<ValueType>& prec,
                RangeFormatter formatter = RangeFormatter{})
    : Field(num_components, as_dynamic(prec))
    , _view(std::move(view))
    , _formatter(std::move(formatter))
    {}

 private:
    View _view;
    RangeFormatter _formatter;

    void _stream(std::ostream& stream) const override {
        _formatter.write(stream, _view);
    }

    Serialization _serialized() const override {
        const auto num_values = std::ranges::distance(_view);
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

template<Concepts::Scalars View>
ScalarField(View&&) -> ScalarField<std::decay_t<View>>;
template<Concepts::Scalars View>
ScalarField(View&&, RangeFormatter) -> ScalarField<std::decay_t<View>>;
template<Concepts::Scalars View, Concepts::Scalar T>
ScalarField(View&&, const Precision<T>& prec) -> ScalarField<std::decay_t<View>, T>;
template<Concepts::Scalars View, Concepts::Scalar T>
ScalarField(View&&, const Precision<T>& prec, RangeFormatter) -> ScalarField<std::decay_t<View>, T>;

template<Concepts::VectorsView View,
         typename ValueType = MDRangeScalar<View>> requires(std::ranges::input_range<View>)
class VectorField : public Field {
public:
    using typename Field::Serialization;

    explicit VectorField(View&& view, RangeFormatter formatter = RangeFormatter{})
    : Field(Fields::Detail::_deduce_number_of_components(view), as_dynamic(Precision<ValueType>{}))
    , _view(std::move(view))
    , _formatter(std::move(formatter))
    {}

    VectorField(View&& view,
                [[maybe_unused]] const Precision<ValueType>& prec,
                RangeFormatter formatter = RangeFormatter{})
    : Field(Fields::Detail::_deduce_number_of_components(view), as_dynamic(prec))
    , _view(std::move(view))
    , _formatter(std::move(formatter))
    {}

 private:
    View _view;
    RangeFormatter _formatter;

    void _stream(std::ostream& stream) const override {
        _formatter.write(stream, _flat_view());
    }

    Serialization _serialized() const override {
        return ScalarField{_flat_view(), Precision<ValueType>{}}.serialized();
    }

    Concepts::Scalars auto _flat_view() const {
        return std::views::join(_view);
    }
};

template<Concepts::Scalars View>
VectorField(View&&) -> VectorField<std::decay_t<View>>;
template<Concepts::Scalars View>
VectorField(View&&, RangeFormatter) -> VectorField<std::decay_t<View>>;
template<Concepts::Scalars View, Concepts::Scalar T>
VectorField(View&&, const Precision<T>& prec) -> VectorField<std::decay_t<View>, T>;
template<Concepts::Scalars View, Concepts::Scalar T>
VectorField(View&&, const Precision<T>& prec, RangeFormatter) -> VectorField<std::decay_t<View>, T>;

template<Concepts::TensorsView View,
         typename ValueType = MDRangeScalar<View>> requires(std::ranges::input_range<View>)
class TensorField : public Field {
public:
    using typename Field::Serialization;

    explicit TensorField(View&& view, RangeFormatter formatter = RangeFormatter{})
    : Field(Fields::Detail::_deduce_number_of_components(view), as_dynamic(Precision<ValueType>{}))
    , _view(std::move(view))
    , _formatter(std::move(formatter))
    {}

    TensorField(View&& view,
                [[maybe_unused]] const Precision<ValueType>& prec,
                RangeFormatter formatter = RangeFormatter{})
    : Field(Fields::Detail::_deduce_number_of_components(view), as_dynamic(prec))
    , _view(std::move(view))
    , _formatter(std::move(formatter))
    {}

 private:
    View _view;
    RangeFormatter _formatter;

    void _stream(std::ostream& stream) const override {
        _formatter.write(stream, _flat_view());
    }

    Serialization _serialized() const override {
        return ScalarField{_flat_view(), Precision<ValueType>{}}.serialized();
    }

    Concepts::Scalars auto _flat_view() const {
        return std::views::join(std::views::join(_view));
    }
};

template<Concepts::Scalars View>
TensorField(View&&) -> TensorField<std::decay_t<View>>;
template<Concepts::Scalars View>
TensorField(View&&, RangeFormatter) -> TensorField<std::decay_t<View>>;
template<Concepts::Scalars View, Concepts::Scalar T>
TensorField(View&&, const Precision<T>& prec) -> TensorField<std::decay_t<View>, T>;
template<Concepts::Scalars View, Concepts::Scalar T>
TensorField(View&&, const Precision<T>& prec, RangeFormatter) -> TensorField<std::decay_t<View>, T>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELDS_HPP_