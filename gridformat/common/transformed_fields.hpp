// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief Transformed field implementations
 */
#ifndef GRIDFORMAT_COMMON_TRANSFORMED_FIELDS_HPP_
#define GRIDFORMAT_COMMON_TRANSFORMED_FIELDS_HPP_

#include <utility>
#include <concepts>
#include <type_traits>

#include <gridformat/common/field.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>

namespace GridFormat {

class IdentityField : public Field {
 public:
    explicit IdentityField(const Field& field)
    : _field(field)
    {}

 private:
    const Field& _field;

    MDLayout _layout() const override {
        return _field.layout();
    }

    DynamicPrecision _precision() const override {
        return _field.precision();
    }

    Serialization _serialized() const override {
        return _field.serialized();
    }
};

class ReshapedField : public Field {
 public:
    explicit ReshapedField(const Field& f, const MDLayout& target_sub_layout)
    : _field(f)
    , _orig_layout{f.determine_layout()}
    , _new_layout{_make_new_layout(target_sub_layout)}
    , _orig_offsets{_make_offsets(_orig_layout)}
    , _new_offsets{_make_offsets(_new_layout)} {
        if (std::ranges::any_of(
                std::views::iota(std::size_t{0}, _orig_layout.dimension()),
                [&] (const std::integral auto i) {
                    return _new_layout.extent(i) < _orig_layout.extent(i);
                }
            )
        )
            throw SizeError("Reshaping only supported into larger extents");
    }

 private:
    const Field& _field;
    MDLayout _orig_layout;
    MDLayout _new_layout;
    std::vector<std::size_t> _orig_offsets;
    std::vector<std::size_t> _new_offsets;

    MDLayout _make_new_layout(const MDLayout& target_sub_layout) const {
        if (_orig_layout.dimension() != target_sub_layout.dimension() + 1)
            throw SizeError("Field sub-dimension does not match given target layout dimension");
        std::vector<std::size_t> extents{_orig_layout.dimension()};
        extents[0] = _orig_layout.extent(0);
        for (std::size_t i = 1; i < _orig_layout.dimension(); ++i)
            extents[i] = target_sub_layout.extent(i - 1);
        return MDLayout{std::move(extents)};
    }

    std::vector<std::size_t> _make_offsets(const MDLayout& layout) const {
        std::vector<std::size_t> result;
        result.reserve(layout.dimension());
        for (std::size_t dim = 0; dim < layout.dimension() - 1; ++i)
            result.push_back(layout.number_of_entries(dim));
        result.push_back(1);
        return result;
    }

    MDLayout _layout() const override {
        return _new_layout;
    }

    DynamicPrecision _precision() const override {
        return _field.precision();
    }

    Serialization _serialized() const override {
        auto serialization = _field.serialized();
        serialization.resize(this->_size_in_bytes(layout), typename Serialization::Byte{0});
        _field.precision().visit([] <typename T> (const Precision<T>&) {
            for (const auto& index : indices())
        });
        return serialization;
    }
};

namespace FieldTransformation {

#ifndef DOXYGEN
namespace Detail {

struct IdentityFieldAdapter {
    auto operator()(const Field& f) const {
        return IdentityField{f};
    }
};

// class ReshapedFieldAdapter {
//  public:
//     explicit ReshapedFieldAdapter(MDLayout sub_layout)
//     : _sub_layout{std::move(sub_layout)}
//     {}

//     auto operator()(const Field& f) const {
//         return ReshapedField{f, _sub_layout};
//     }

//  private:
//     MDLayout _sub_layout;
// };

// class ReshapedFieldAdapterClosure {
//     auto operator()(MDLayout sub_layout) const {
//         return ReshapedFieldAdapter{std::move(sub_layout)};
//     }
// };

}  // namespace Detail
#endif  // DOXYGEN

inline constexpr Detail::IdentityFieldAdapter identity;
// inline constexpr Detail::ReshapedFieldAdapterClosure reshaped;

}  // namespace FieldTransformation


namespace Concepts {

template<typename T>
concept FieldTransformation
    = std::invocable<T, const Field&> and requires(const T& t, const Field& f) {
    { t(f) } -> std::derived_from<Field>;
};

}  // namespace Concepts


template<typename F, Concepts::FieldTransformation T>
requires(std::derived_from<std::decay_t<F>, Field>)
class TransformedField;


#ifndef DOXYGEN
namespace Detail {

    template<typename T>
    struct IsTransformed : public std::false_type {};

    template<typename... Args>
    struct IsTransformed<TransformedField<Args...>> : public std::true_type {};

    template<typename T>
    class TransformedFieldStorage;

    template<typename F> requires(std::is_lvalue_reference_v<F>)
    class TransformedFieldStorage<F> {
    public:
        explicit TransformedFieldStorage(const F& field) : _field{field} {}
        const F& get() const { return _field; }
    private:
        const F& _field;
    };

    template<typename F>
    requires(!std::is_lvalue_reference_v<F> and IsTransformed<std::decay_t<F>>::value)
    class TransformedFieldStorage<F> {
    public:
        explicit TransformedFieldStorage(F&& field) : _field{std::move(field)} {}
        const F& get() const { return _field; }
    private:
        F _field;
    };

}  // namespace Detail
#endif  // DOXYGEN

template<typename F, Concepts::FieldTransformation T>
requires(std::derived_from<std::decay_t<F>, Field>)
class TransformedField : public Field {
    using Transformed = std::invoke_result_t<T, const Field&>;

 public:
    using Transformation = T;

    template<typename _F> requires(std::same_as<std::decay_t<_F>, std::decay_t<F>>)
    TransformedField(_F&& field, T&& trafo)
    : _storage{std::forward<_F>(field)}
    , _transformed{trafo(_storage.get())}
    {}

 private:
    Detail::TransformedFieldStorage<F> _storage;
    Transformed _transformed;

    MDLayout _layout() const override {
        return _transformed.layout();
    }

    DynamicPrecision _precision() const override {
        return _transformed.precision();
    }

    Serialization _serialized() const override {
        return _transformed.serialized();
    }
};

template<typename F, typename T> requires(std::is_lvalue_reference_v<F>)
TransformedField(F&&, T&&) -> TransformedField<std::remove_reference_t<F>&, T>;
template<typename F, typename T> requires(!std::is_lvalue_reference_v<F>)
TransformedField(F&&, T&&) -> TransformedField<std::decay_t<F>, T>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_TRANSFORMED_FIELDS_HPP_