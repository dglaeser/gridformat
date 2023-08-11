// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::MDIndex
 */
#ifndef GRIDFORMAT_COMMON_MD_INDEX_HPP_
#define GRIDFORMAT_COMMON_MD_INDEX_HPP_

#include <cassert>
#include <ostream>
#include <utility>
#include <concepts>
#include <iterator>
#include <algorithm>
#include <initializer_list>
#include <functional>
#include <numeric>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/reserved_vector.hpp>
#include <gridformat/common/iterator_facades.hpp>
#include <gridformat/common/md_layout.hpp>

namespace GridFormat {

//! \addtogroup Common
//! \{

//! Represents a multi-dimensional index.
class MDIndex {
    static constexpr std::size_t buffered_dimensions = 5;
    using Indices = ReservedVector<std::size_t, buffered_dimensions>;

 public:
    MDIndex() = default;

    //! Construct from a range of indices
    template<Concepts::MDRange<1> R>
    explicit MDIndex(R&& indices) {
        _indices.reserve(Ranges::size(indices));
        std::ranges::copy(indices, std::back_inserter(_indices));
    }

    //! Construct from a vector of indices
    template<std::integral T>
    explicit MDIndex(const std::initializer_list<T>& indices) {
        _indices.reserve(Ranges::size(indices));
        std::ranges::copy(indices, std::back_inserter(_indices));
    }

    //! Zero-initialize a md-index with a given size
    explicit MDIndex(std::integral auto size) {
        _indices.resize(size, std::size_t{0});
    }

    //! Zero-initialize a md-index with a given layout
    explicit MDIndex(const MDLayout& layout)
    : MDIndex(layout.dimension())
    {}

    auto begin() const { return _indices.begin(); }
    auto begin() { return _indices.begin(); }

    auto end() const { return _indices.end(); }
    auto end() { return _indices.end(); }

    std::size_t size() const {
        return _indices.size();
    }

    std::size_t get(unsigned int codim) const {
        return _indices[codim];
    }

    void set(unsigned int codim, std::size_t index) {
        _indices[codim] = index;
    }

    bool operator==(const MDIndex& other) const {
        return std::ranges::equal(_indices, other._indices);
    }

    MDIndex& operator+=(const MDIndex& other) {
        if (size() != other.size())
            throw ValueError("MDIndex size mismatch");
        std::transform(
            begin(),
            end(),
            other.begin(),
            begin(),
            std::plus<std::size_t>{}
        );
        return *this;
    }

    MDIndex operator+(const MDIndex& other) const {
        MDIndex result(*this);
        result += other;
        return result;
    }

    friend std::ostream& operator<<(std::ostream& s, const MDIndex& md_index) {
        s << "(";
        if (md_index._indices.size() > 0) {
            s << md_index._indices[0];
            std::ranges::for_each(md_index._indices | std::views::drop(1), [&] (const auto idx) {
                s << "," << idx;
            });
        }
        s << ")";
        return s;
    }

 private:
    Indices _indices;
};


//! A range over multi-dimensional indices within a given layout
class MDIndexRange {
    class Iterator
    : public ForwardIteratorFacade<Iterator, MDIndex, const MDIndex&> {
     public:
        Iterator() = default;
        Iterator(const MDLayout& layout, bool is_end = false)
        : _layout{&layout}
        , _current{layout}
        , _last_dim{layout.dimension() - 1} {
            if (is_end)
                _current.set(_last_dim, layout.extent(_last_dim));
        }

     private:
        friend IteratorAccess;

        const MDIndex& _dereference() const {
            return _current;
        }

        bool _is_equal(const Iterator& other) const {
            return _layout == other._layout && _current == other._current;
        }

        void _increment() {
            assert(!_is_end());
            unsigned int codim = 0;
            while (true) {
                _increment_at(codim);
                if (_current.get(codim) >= _layout->extent(codim)) {
                    if (codim == _last_dim)
                        break;
                    _current.set(codim, 0);
                    codim++;
                } else {
                    break;
                }
            }
        }

        void _increment_at(unsigned int codim) {
            _current.set(codim, _current.get(codim) + 1);
        }

        bool _is_end() const {
            return _current.get(_last_dim) >= _layout->extent(_last_dim);
        }

        const MDLayout* _layout;
        MDIndex _current;
        std::size_t _last_dim;
    };

 public:
    explicit MDIndexRange(MDLayout layout)
    : _layout{std::move(layout)}
    {}

    explicit MDIndexRange(const std::vector<std::size_t>& dimensions)
    : MDIndexRange(MDLayout{dimensions})
    {}

    explicit MDIndexRange(const std::initializer_list<std::size_t>& dimensions)
    : MDIndexRange(MDLayout{dimensions})
    {}

    auto begin() const { return Iterator{_layout}; }
    auto end() const { return Iterator{_layout, true}; }

    const MDLayout& layout() const {
        return _layout;
    }

    std::size_t size() const {
        return _layout.number_of_entries();
    }

    std::size_t size(unsigned int codim) const {
        return _layout.extent(codim);
    }

 private:
    MDLayout _layout;
};


#ifndef DOXYGEN
namespace Detail {

    template<std::ranges::range R>
    std::size_t flat_index_from_sub_sizes(const MDIndex& index, R&& sub_sizes) {
        assert(index.size() == Ranges::size(sub_sizes));
        auto offsets = std::views::iota(std::size_t{0}, index.size())
            | std::views::transform([&] (const std::integral auto dim) {
                return index.get(dim)*sub_sizes[dim];
        });
        return std::accumulate(
            std::ranges::begin(offsets),
            std::ranges::end(offsets),
            0
        );
    }

}  // namespace Detail
#endif  // DOXYGEN

//! Compute the flat index from a multidimensional index and layout
inline std::size_t flat_index(const MDIndex& index, const MDLayout& layout) {
    assert(index.size() == layout.dimension());
    auto sub_sizes = std::views::iota(std::size_t{0}, index.size())
        | std::views::transform([&] (const std::integral auto dim) {
            if (dim < layout.dimension() - 1)
                return layout.number_of_entries(dim + 1);
            return std::size_t{1};
    });
    return Detail::flat_index_from_sub_sizes(index, sub_sizes);
}

//! \} group Common

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_MD_INDEX_HPP_
