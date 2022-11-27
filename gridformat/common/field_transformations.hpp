// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief Transformed field implementations
 */
#ifndef GRIDFORMAT_COMMON_FIELD_TRANSFORMATIONS_HPP_
#define GRIDFORMAT_COMMON_FIELD_TRANSFORMATIONS_HPP_

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

/*!
 * \ingroup Common
 * \brief Wraps an underlying field by an identity transformation
 */
class IdentityField : public Field {
 public:
    explicit IdentityField(FieldPtr field)
    : _field(field)
    {}

 private:
    FieldPtr _field;

    MDLayout _layout() const override {
        return _field->layout();
    }

    DynamicPrecision _precision() const override {
        return _field->precision();
    }

    Serialization _serialized() const override {
        return _field->serialized();
    }
};

/*!
 * \ingroup Common
 * \brief Exposes a flat view on a given field
 */
class FlattenedField : public Field {
 public:
    explicit FlattenedField(FieldPtr ptr)
    : _field{ptr}
    {}

 private:
    FieldPtr _field;

    MDLayout _layout() const override {
        return MDLayout{{_field->layout().number_of_entries()}};
    }

    DynamicPrecision _precision() const override {
        return _field->precision();
    }

    Serialization _serialized() const override {
        return _field->serialized();
    }
};

/*!
 * \ingroup Common
 * \brief Extends a given field with zeros up to the given extents
 * \note This takes the extents of the sub-layout as constructor
 *       argument. The extent of the outermost range stays the same.
 */
class ExtendedField : public Field {
 public:
    explicit ExtendedField(FieldPtr f, MDLayout target_sub_layout)
    : _field{f}
    , _target_sub_layout{std::move(target_sub_layout)}
    {}

 private:
    FieldPtr _field;
    MDLayout _target_sub_layout;

    MDLayout _extended_layout() const {
        return _extended_layout(_field->layout());
    }

    MDLayout _extended_layout(const MDLayout& orig_layout) const {
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
        return _extended_layout();
    }

    DynamicPrecision _precision() const override {
        return _field->precision();
    }

    Serialization _serialized() const override {
        const auto orig_layout = _field->layout();
        const auto new_layout = _extended_layout(orig_layout);

        const auto orig_sub_sizes = _sub_sizes(orig_layout);
        const auto new_sub_sizes = _sub_sizes(new_layout);

        auto serialization = _field->serialized();
        _field->precision().visit([&] <typename T> (const Precision<T>&) {
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
        auto operator()(FieldPtr f) const {
            return make_shared(IdentityField{f});
        }
    };

    struct FlattenedFieldAdapter {
        auto operator()(FieldPtr f) const {
            return make_shared(FlattenedField{f});
        }
    };

    class ExtendFieldAdapter {
    public:
        explicit ExtendFieldAdapter(MDLayout sub_layout)
        : _sub_layout{std::move(sub_layout)}
        {}

        auto operator()(FieldPtr f) const {
            if (f->layout().dimension() <= 1)
                throw SizeError("Extension only works for fields with dimension > 1");
            return make_shared(ExtendedField{f, _sub_layout});
        }

    private:
        MDLayout _sub_layout;
    };

    struct ExtendFieldAdapterClosure {
        auto operator()(MDLayout sub_layout) const {
            return ExtendFieldAdapter{std::move(sub_layout)};
        }
    };

    class ExtendAllFieldAdapter {
    public:
        explicit ExtendAllFieldAdapter(std::size_t space_dimension)
        : _space_dim{space_dimension}
        {}

        auto operator()(FieldPtr f) const {
            const std::size_t dim = f->layout().dimension();
            if (dim <= 1)
                throw SizeError("Extension only works for fields with dimension > 1");
            return make_shared(ExtendedField{
                f,
                MDLayout{
                std::views::iota(std::size_t{1}, dim)
                    | std::views::transform([&] (const auto&) { return _space_dim; })
                }
            });
        }

    private:
        std::size_t _space_dim;
    };

    struct ExtendAllFieldAdapterClosure {
        auto operator()(std::size_t space_dimension) const {
            return ExtendAllFieldAdapter{space_dimension};
        }
    };

}  // namespace Detail
#endif  // DOXYGEN

inline constexpr Detail::IdentityFieldAdapter identity;
inline constexpr Detail::FlattenedFieldAdapter flatten;
inline constexpr Detail::ExtendFieldAdapterClosure extend_to;
inline constexpr Detail::ExtendAllFieldAdapterClosure extend_all_to;

}  // namespace FieldTransformation


namespace Concepts {

template<typename T>
concept FieldTransformation
    = std::invocable<T, FieldPtr> and requires(const T& t, FieldPtr f) {
    { t(f) } -> std::convertible_to<FieldPtr>;
};

}  // namespace Concepts


class TransformedField : public Field {
 public:
    template<Concepts::FieldTransformation T>
    TransformedField(FieldPtr field, T&& trafo)
    : _transformed{trafo(field)}
    {}

 private:
    FieldPtr _transformed;

    MDLayout _layout() const override {
        return _transformed->layout();
    }

    DynamicPrecision _precision() const override {
        return _transformed->precision();
    }

    Serialization _serialized() const override {
        return _transformed->serialized();
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FIELD_TRANSFORMATIONS_HPP_
