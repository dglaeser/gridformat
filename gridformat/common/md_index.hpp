// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::MDIndex
 */
#ifndef GRIDFORMAT_COMMON_MD_INDEX_HPP_
#define GRIDFORMAT_COMMON_MD_INDEX_HPP_

#include <cassert>
#include <vector>
#include <ostream>
#include <utility>
#include <concepts>
#include <iterator>
#include <algorithm>
#include <numeric>
#include <functional>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/iterator_facades.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Represents a multi-dimensional index.
 */
class MDIndex {
 public:
    MDIndex() = default;

    template<std::ranges::forward_range R>
    explicit MDIndex(R&& indices) {
        _indices.reserve(Ranges::size(indices));
        std::ranges::copy(indices, std::back_inserter(_indices));
    }

    explicit MDIndex(std::vector<std::size_t>&& indices)
    : _indices(std::move(indices))
    {}

    explicit MDIndex(std::integral auto size) {
        _indices.resize(size, 0);
    }

    explicit MDIndex(const MDLayout& layout)
    : MDIndex(layout.dimension())
    {}

    std::size_t size() const {
        return _indices.size();
    }

    std::size_t get(std::size_t dim) const {
        return _indices[dim];
    }

    void set(std::size_t dim, std::size_t index) {
        _indices[dim] = index;
    }

    bool operator==(const MDIndex& other) const {
        return std::ranges::equal(_indices, other._indices);
    }

    friend std::ostream& operator<<(std::ostream& s, const MDIndex& i) {
        s << "(";
        if (i._indices.size() > 0) {
            s << i._indices[0];
            std::ranges::for_each(i._indices | std::views::drop(1), [&] (const auto idx) {
                s << "," << idx;
            });
        }
        s << ")";
        return s;
    }

 private:
    std::vector<std::size_t> _indices;
};

class MDIndexRange {
    class Iterator : public BidirectionalIteratorFacade<Iterator, MDIndex, const MDIndex&> {
     public:
        Iterator() = default;
        explicit Iterator(const MDIndexRange& range, bool is_end = false)
        : _range{&range}
        , _current{range._layout.dimension()}
        , _is_end(is_end) {
            if (is_end) {
                _current.set(0, range._layout.extent(0));
            }
        }

     private:
        friend class IteratorAccess;

        const MDIndex& _dereference() const {
            std::cout << "DEREF" << std::endl;
            assert(!_is_end);
            return _current;
        }

        bool _is_equal(const Iterator& other) const {
            std::cout << "EQ" << std::endl;
            if (_range != other._range)
                return false;
            if (_is_end != other._is_end)
                return false;
            if (_is_end && other._is_end)
                return true;
            return _current == other._current;
        }

        void _increment() {
            std::cout << "INC" << std::endl;
            _increment(_range->_layout.dimension() - 1);
        }

        void _increment(std::size_t dim) {
            _current.set(dim, _current.get(dim) + 1);
            if (_current.get(dim) >= _range->_layout.extent(dim)) {
                if (dim == 0) {
                    _is_end = true;
                } else {
                    _current.set(dim, 0);
                    _increment(dim - 1);
                }
            }
        }

        void _decrement() {
            std::cout << "CLL" << std::endl;
            _decrement(_range->_layout.dimension() - 1);
            _is_end = false;
        }

        void _decrement(std::size_t dim) {
            if (_current.get(dim) == 0) {
                assert(dim != 0);
                _current.set(dim, _range->_layout.extent(dim) - 1);
                _decrement(dim - 1);
            } else {
                _current.set(dim, _current.get(dim) - 1);
            }
        }

        const MDIndexRange* _range;
        MDIndex _current;
        bool _is_end = false;
    };

 public:
    explicit MDIndexRange(MDLayout layout)
    : _layout(std::move(layout))
    {}

    auto begin() const { return Iterator{*this}; }
    auto end() const { return Iterator{*this, true}; }

 private:
    friend class Iterator;
    MDLayout _layout;
};

inline auto indices(MDLayout layout) {
    return MDIndexRange{std::move(layout)};
}


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

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_MD_INDEX_HPP_
