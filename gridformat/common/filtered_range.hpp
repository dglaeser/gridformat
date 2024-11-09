// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::FilteredRange
 */
#ifndef GRIDFORMAT_COMMON_FILTERED_RANGE_HPP_
#define GRIDFORMAT_COMMON_FILTERED_RANGE_HPP_

#include <ranges>
#include <cassert>
#include <utility>
#include <concepts>
#include <iterator>

#include <gridformat/common/iterator_facades.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace FilteredRangeDetail {

    template<typename Range, typename Predicate>
    using PredicateResult = std::invoke_result_t<
        const Predicate&,
        std::ranges::range_reference_t<Range>
    >;

    template<typename _It, typename _Sentinel, typename Predicate>
    class Iterator
    : public ForwardIteratorFacade<Iterator<_It, _Sentinel, Predicate>,
                                   typename std::iterator_traits<_It>::value_type,
                                   typename std::iterator_traits<_It>::reference> {
     public:
        Iterator() = default;
        explicit Iterator(_It it, _Sentinel sentinel, const Predicate& pred, bool is_sentinel)
        : _it{it}
        , _end_it{sentinel}
        , _pred{&pred}
        , _is_end_it{is_sentinel} {
            if (_should_increment())
                _increment();
        }

        bool _is_sentinel() const { return _is_end_it; }
        const _It& _current() const { return _it; }
        const _Sentinel& _sentinel() const { return _end_it; }
        const Predicate& _predicate() const { return *_pred; }

     private:
        friend IteratorAccess;

        decltype(auto) _dereference() const {
            assert(!_is_end());
            return *_it;
        }

        void _increment() {
            assert(!_is_end());
            ++_it;
            while (_should_increment())
                ++_it;
        }

        template<typename I, typename S, typename P>
        bool _is_equal(const Iterator<I, S, P>& other) const {
            if (_is_sentinel() && !other._is_sentinel()) return _end_it == other._current();
            if (!_is_sentinel() && other._is_sentinel()) return _it == other._sentinel();
            if (_it != other._it) return false;
            if constexpr (!std::is_same_v<Predicate, P>)
                return false;
            else
                return &_predicate() == &other._predicate();
        }

        bool _is_end() const {
            return _is_end_it || _it == _end_it;
        }

        bool _current_true() const {
            return (*_pred)(*_it);
        }

        bool _should_increment() const {
            if (!_is_end())
                return !_current_true();
            return false;
        }

        _It _it;
        _Sentinel _end_it;
        const Predicate* _pred{nullptr};
        bool _is_end_it{true};
    };

    template<typename I, typename S, typename P>
    Iterator(I&&, S&&, const P&, bool) -> Iterator<std::remove_cvref_t<I>, std::remove_cvref_t<S>, P>;

}  // namespace FilteredRangeDetail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief Filters a range by a given predicate and
 *        only yields those elements that fulfill it.
 * \note We need this because `std::views::filter` yields a view that
 *       exposes `begin()` and `end()` only as non-const (because the
 *       filtered begin iterator is cached in the first call to `begin()`).
 *       However, we need to be able to use const filtered ranges in several contexts,
 *       at the cost of finding the beginning of the range every time the filtered
 *       range is traversed.
 */
template<std::ranges::forward_range R,
         std::invocable<std::ranges::range_reference_t<R>> Predicate>
    requires(std::convertible_to<bool, FilteredRangeDetail::PredicateResult<R, Predicate>>)
class FilteredRange {
 public:
    template<typename _R> requires(std::convertible_to<_R, R>)
    explicit FilteredRange(_R&& range, Predicate&& pred)
    : _range{std::forward<_R>(range)}
    , _pred{std::move(pred)}
    {}

    auto begin() const {
        return FilteredRangeDetail::Iterator{
            std::ranges::begin(_range),
            std::ranges::end(_range),
            _pred,
            false
        };
    }

    auto end() const {
        return FilteredRangeDetail::Iterator{
            std::ranges::begin(_range),
            std::ranges::end(_range),
            _pred,
            true
        };
    }

 private:
    R _range;
    std::remove_cvref_t<Predicate> _pred;
};

template<std::ranges::range R, typename P> requires(!std::is_lvalue_reference_v<R>)
FilteredRange(R&&, P&&) -> FilteredRange<std::remove_cvref_t<R>, std::remove_cvref_t<P>>;
template<std::ranges::range R, typename P> requires(std::is_lvalue_reference_v<R>)
FilteredRange(R&&, P&&) -> FilteredRange<std::remove_reference_t<R>&, std::remove_cvref_t<P>>;

namespace Ranges {

template<typename P, std::ranges::range R>
auto filter_by(P&& predicate, R&& range) {
    return FilteredRange{std::forward<R>(range), std::forward<P>(predicate)};
}

}  // namespace Ranges
}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FILTERED_RANGE_HPP_
