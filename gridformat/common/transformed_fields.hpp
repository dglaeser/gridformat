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
#include <algorithm>
#include <type_traits>

#include <gridformat/common/field.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/md_index.hpp>

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

class FlatMDField : public Field {
 public:
    explicit FlatMDField(const Field& field)
    : _field(field)
    {}

 private:
    const Field& _field;

    MDLayout _layout() const override {
        return MDLayout{{_field.layout().number_of_entries()}};
    }

    DynamicPrecision _precision() const override {
        return _field.precision();
    }

    Serialization _serialized() const override {
        return _field.serialized();
    }
};

class ExtendedMDField : public Field {
 public:
    explicit ExtendedMDField(const Field& f, MDLayout target_sub_layout)
    : _field{f}
    , _target_sub_layout{std::move(target_sub_layout)}
    {}

 private:
    const Field& _field;
    MDLayout _target_sub_layout;

    MDLayout _new_layout() const {
        return _new_layout(_field.layout());
    }

    MDLayout _new_layout(const MDLayout& orig_layout) const {
        if (orig_layout.dimension() <= 1)
            throw SizeError("Can only reshape fields with dimension > 1");
        if (orig_layout.dimension() != _target_sub_layout.dimension() + 1)
            throw SizeError("Field sub-dimension does not match given target layout dimension");
        std::vector<std::size_t> extents(orig_layout.dimension());
        extents[0] = orig_layout.extent(0);
        for (std::size_t i = 1; i < orig_layout.dimension(); ++i)
            extents[i] = _target_sub_layout.extent(i - 1);
        _check_valid_layout(orig_layout, extents);
        return MDLayout{std::move(extents)};
    }

    std::vector<std::size_t> _sub_sizes(const MDLayout& layout) const {
        std::vector<std::size_t> result;
        result.reserve(layout.dimension());
        for (std::size_t dim = 0; dim < layout.dimension() - 1; ++dim)
            result.push_back(layout.number_of_entries(dim + 1));
        result.push_back(1);
        return result;
    }

    MDLayout _layout() const override {
        return _new_layout();
    }

    DynamicPrecision _precision() const override {
        return _field.precision();
    }

    Serialization _serialized() const override {
        const auto orig_layout = _field.layout();
        const auto new_layout = _new_layout(orig_layout);

        const auto orig_sub_sizes = _sub_sizes(orig_layout);
        const auto new_sub_sizes = _sub_sizes(new_layout);

        auto serialization = _field.serialized();
        _field.precision().visit([&] <typename T> (const Precision<T>&) {
            serialization.resize(new_layout.number_of_entries()*sizeof(T), typename Serialization::Byte{0});
            const auto data = serialization.template as_span_of<T>();
            for (const auto& index : reversed_indices(orig_layout)) {
                const auto orig_flat_index = Detail::flat_index_from_sub_sizes(index, orig_sub_sizes);
                const auto new_flat_index = Detail::flat_index_from_sub_sizes(index, new_sub_sizes);
                assert(orig_flat_index < data.size());
                assert(new_flat_index < data.size());
                std::swap(data[orig_flat_index], data[new_flat_index]);
            }
        });
        return serialization;
    }

    void _check_valid_layout(const MDLayout& orig, const std::vector<std::size_t>& extents) const {
        if (std::ranges::any_of(
            std::views::iota(std::size_t{0}, orig.dimension()),
            [&] (const std::size_t i) {
                return orig.extent(i) > extents[i];
            }
        ))
            throw SizeError("Given target extension smaller than original extension.");
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

struct FlatMDFieldAdapter {
    auto operator()(const Field& f) const {
        return FlatMDField{f};
    }
};

class ExtendedMDFieldAdapter {
 public:
    explicit ExtendedMDFieldAdapter(MDLayout sub_layout)
    : _sub_layout{std::move(sub_layout)}
    {}

    auto operator()(const Field& f) const {
        return ExtendedMDField{f, _sub_layout};
    }

 private:
    MDLayout _sub_layout;
};

struct ExtendedMDFieldAdapterClosure {
    auto operator()(MDLayout sub_layout) const {
        return ExtendedMDFieldAdapter{std::move(sub_layout)};
    }
};

class ExtendedFieldAdapter {
 public:
    explicit ExtendedFieldAdapter(std::size_t space_dimension)
    : _space_dim{space_dimension}
    {}

    auto operator()(const Field& f) const {
        const std::size_t dim = f.layout().dimension();
        if (dim <= 1)
            throw SizeError("Extension only works for fields with dimension > 1");
        return ExtendedMDField{
            f,
            MDLayout{
            std::views::iota(std::size_t{1}, dim)
                | std::views::transform([&] (const auto&) { return _space_dim; })
            }
        };
    }

 private:
    std::size_t _space_dim;
};

struct ExtendedFieldAdapterClosure {
    auto operator()(std::size_t space_dimension) const {
        return ExtendedFieldAdapter{space_dimension};
    }
};

}  // namespace Detail
#endif  // DOXYGEN

inline constexpr Detail::IdentityFieldAdapter identity;
inline constexpr Detail::FlatMDFieldAdapter flattened;
inline constexpr Detail::ExtendedMDFieldAdapterClosure extended_md;
inline constexpr Detail::ExtendedFieldAdapterClosure extended;

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
    class TransformedFieldStorage;

    template<typename F> requires(std::is_lvalue_reference_v<F>)
    class TransformedFieldStorage<F> {
    public:
        explicit TransformedFieldStorage(const F& field) : _field{field} {}
        const F& get() const { return _field; }
    private:
        const F& _field;
    };

    template<typename F> requires(!std::is_lvalue_reference_v<F>)
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

    template<typename _F, std::convertible_to<T> _T>
    requires(std::same_as<std::decay_t<_F>, std::decay_t<F>>)
    TransformedField(_F&& field, _T&& trafo)
    : _storage{std::forward<_F>(field)}
    , _transformation{std::forward<_T>(trafo)}
    , _transformed{_transformation(_storage.get())}
    {}

    TransformedField(const TransformedField& other)
    : _storage{other._storage}
    , _transformation{other._transformation}
    , _transformed{_transformation(_storage.get())}
    {}

    TransformedField(TransformedField&& other)
    : _storage{std::move(other._storage)}
    , _transformation{std::move(other._transformation)}
    , _transformed{_transformation(_storage.get())}
    {}

 private:
    Detail::TransformedFieldStorage<F> _storage;
    Transformation _transformation;
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
TransformedField(F&&, T&&) -> TransformedField<std::remove_reference_t<F>&, std::decay_t<T>>;
template<typename F, typename T> requires(!std::is_lvalue_reference_v<F>)
TransformedField(F&&, T&&) -> TransformedField<std::decay_t<F>, std::decay_t<T>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_TRANSFORMED_FIELDS_HPP_
