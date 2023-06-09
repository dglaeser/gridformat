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
#include <gridformat/common/iterator_facades.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace FieldTransformationDetail {
    class BackwardsMDIndexMapWalk {
     public:
        template<std::convertible_to<MDLayout> _L1,
                 std::convertible_to<MDLayout> _L2>
        BackwardsMDIndexMapWalk(_L1&& source_layout,
                                _L2&& target_layout)
        : _source_layout{std::forward<_L1>(source_layout)}
        , _target_layout{std::forward<_L2>(target_layout)} {
            if (_source_layout.dimension() != _target_layout.dimension())
                throw InvalidState("Source and target layout dimensions mismatch");
            if (std::ranges::any_of(
                std::views::iota(std::size_t{0}, _source_layout.dimension()),
                [&] (const std::size_t i) {
                    return _source_layout.extent(i) > _target_layout.extent(i);
                }
            ))
                throw InvalidState("Only mapping into larger layouts supported");

            _compute_target_offsets();
            _current = _make_end_index(_source_layout);
            _current_flat = flat_index(_current, _source_layout);
            _current_target_flat = flat_index(_current, _target_layout);
        }

        void next() {
            _decrement();
        }

        bool is_finished() const {
            return std::ranges::any_of(
                std::views::iota(std::size_t{0}, _source_layout.dimension()),
                [&] (const std::size_t i) {
                    return _current.get(i) >= _source_layout.extent(i);
                }
            );
        }

        const MDIndex& current() const {
            return _current;
        }

        std::size_t source_index_flat() const {
            return _current_flat;
        }

        std::size_t target_index_flat() const {
            return _current_target_flat;
        }

     private:
        void _decrement() {
            _decrement(_source_layout.dimension() - 1);
        }

        void _decrement(std::size_t i) {
            if (_current.get(i) == 0) {
                _current.set(i, _source_layout.extent(i) - 1);
                if (i > 0)
                    _decrement(i-1);
                else {
                    assert(i == 0);
                    _current.set(i, _source_layout.extent(i));
                }
            } else {
                _current.set(i, _current.get(i) - 1);
                _current_flat--;
                _current_target_flat--;
                _current_target_flat -= _target_offsets[i];
            }
        }

        MDIndex _make_begin_index(const MDLayout& layout) const {
            return MDIndex{
                std::views::iota(std::size_t{0}, layout.dimension())
                | std::views::transform([&] (const std::size_t&) {
                    return 0;
                })
            };
        }

        MDIndex _make_end_index(const MDLayout& layout) const {
            return MDIndex{
                std::views::iota(std::size_t{0}, layout.dimension())
                | std::views::transform([&] (const std::size_t i) {
                    return layout.extent(i) - 1;
                })
            };
        }

        void _compute_target_offsets() {
            _target_offsets.reserve(_source_layout.dimension());
            for (std::size_t i = 1; i < _source_layout.dimension(); ++i)
                _target_offsets.push_back(
                    _target_layout.number_of_entries(i)
                    - _source_layout.number_of_entries(i)
                );
            _target_offsets.push_back(0);
            for (std::size_t i = 0; i < _target_offsets.size() - 1; ++i)
                _target_offsets[i] -= (_source_layout.extent(i+1) - 1)*_target_offsets[i+1];
        }

        MDLayout _source_layout;
        MDLayout _target_layout;
        std::vector<std::size_t> _target_offsets;

        MDIndex _current;
        std::size_t _current_flat;
        std::size_t _current_target_flat;
    };
}
#endif  // DOXYGEN


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
 * \note This takes the extents of the desired sub-layout as constructor
 *       argument. The extent of the first dimension stays the same.
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

        std::vector<std::size_t> extents(orig_layout.dimension(), orig_layout.extent(0));
        std::ranges::copy(
            std::views::iota(std::size_t{0}, _target_sub_layout.dimension())
                | std::views::transform([&] (auto i) { return _target_sub_layout.extent(i); }),
            extents.begin() + 1
        );
        _check_valid_layout(orig_layout, extents);
        return MDLayout{std::move(extents)};
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

        auto serialization = _field->serialized();
        if (orig_layout == new_layout)
            return serialization;

        using Walk = FieldTransformationDetail::BackwardsMDIndexMapWalk;
        Walk index_walk{orig_layout, new_layout};

        _field->precision().visit([&] <typename T> (const Precision<T>&) {
            serialization.resize(new_layout.number_of_entries()*sizeof(T), typename Serialization::Byte{0});
            const auto data = serialization.template as_span_of<T>();
            while (!index_walk.is_finished()) {
                assert(index_walk.source_index_flat() < data.size());
                assert(index_walk.target_index_flat() < data.size());
                std::swap(
                    data[index_walk.source_index_flat()],
                    data[index_walk.target_index_flat()]
                );
                index_walk.next();
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
            return make_field_ptr(IdentityField{f});
        }
    };

    struct FlattenedFieldAdapter {
        auto operator()(FieldPtr f) const {
            return make_field_ptr(FlattenedField{f});
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
            return make_field_ptr(ExtendedField{f, _sub_layout});
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
            return make_field_ptr(ExtendedField{f, MDLayout{
                std::views::iota(std::size_t{1}, dim)
                    | std::views::transform([&] (const auto&) { return _space_dim; })
            }});
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
