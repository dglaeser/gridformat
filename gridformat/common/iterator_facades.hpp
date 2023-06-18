// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Iterator facades implemented by means of CRTP.
 */
#ifndef GRIDFORMAT_COMMON_ITERATOR_FACADES_HPP_
#define GRIDFORMAT_COMMON_ITERATOR_FACADES_HPP_

#include <compare>
#include <cstddef>
#include <iterator>
#include <type_traits>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/detail/crtp.hpp>

namespace GridFormat {

//! \addtogroup Common
//! @{

/*!
 * \brief Gateway for granting the iterator facades access to
 *        the iterator implementations. Iterator implementations
 *        are expected to provide a set of functions, depending
 *        on the type of iterator that is modeled. It is recommended
 *        to put these functions in private scope, and make IteratorAccess
 *        a friend of the implementation such that the facades can
 *        request access via this class.
 */
class IteratorAccess {
 public:
    template<typename IteratorImpl>
    static decltype(auto) dereference(IteratorImpl* impl) {
        return impl->_dereference();
    }

    template<typename IteratorImpl>
    static void increment(IteratorImpl* impl) {
        return impl->_increment();
    }

    template<typename IteratorImpl>
    static void decrement(IteratorImpl* impl) {
        return impl->_decrement();
    }

    template<typename IteratorImpl>
    static void advance(IteratorImpl* impl, std::signed_integral auto n) {
        return impl->_advance(n);
    }

    template<std::signed_integral Difference, typename IteratorImpl1, typename IteratorImpl2>
    static Difference distance(IteratorImpl1* lhs, IteratorImpl2* rhs) {
        return lhs->_distance_to(*rhs);
    }

    template<typename IteratorImpl1, typename IteratorImpl2>
    static bool equal(IteratorImpl1* lhs, IteratorImpl2* rhs) {
        return lhs->_is_equal(*rhs);
    }
};

/*!
 * \brief Base class for all iterator facades.
 * \tparam Impl The derived (implementation) type
 * \tparam IteratorTag The iterator category tag
 * \tparam ValueType The type over which it is iterated
 * \tparam Reference The reference type returned after dereferencing
 * \tparam Difference The type to express differences between two iterators
 */
template<typename Impl,
         typename IteratorTag,
         typename ValueType,
         typename Reference = ValueType&,
         typename Pointer = std::remove_reference_t<Reference>*,
         typename Difference = std::ptrdiff_t>
class IteratorFacade
: public Detail::CRTPBase<Impl> {
    static constexpr bool is_bidirectional = std::derived_from<IteratorTag, std::bidirectional_iterator_tag>;
    static constexpr bool is_random_access = std::derived_from<IteratorTag, std::random_access_iterator_tag>;

 public:
    using iterator_category = IteratorTag;
    using value_type = ValueType;
    using difference_type = Difference;
    using pointer = Pointer;
    using reference = Reference;

    template<Concepts::Interoperable<Impl> I, typename V, typename R, typename D>
    friend bool operator==(const IteratorFacade& lhs,
                           const IteratorFacade<I, V, R, D>& rhs) {
        if constexpr (std::is_convertible_v<I, Impl>)
            return IteratorAccess::equal(
                lhs._pimpl(),
                Detail::cast_to_impl_ptr<const Impl>(&rhs)
            );
        else
            return IteratorAccess::equal(
                Detail::cast_to_impl_ptr<const I>(&lhs),
                rhs._pimpl()
            );
    }

    template<Concepts::Interoperable<Impl> I, typename V, typename R, typename D>
    friend bool operator!=(const IteratorFacade& lhs,
                           const IteratorFacade<I, V, R, D>& rhs) {
        return !(lhs == rhs);
    }

    Reference operator*() const {
        return IteratorAccess::dereference(this->_pimpl());
    }

    Pointer operator->() const {
        static_assert(std::is_lvalue_reference_v<Reference>);
        return &(IteratorAccess::dereference(this->_pimpl()));
    }

    Impl& operator++() {
        IteratorAccess::increment(this->_pimpl());
        return this->_impl();
    }

    Impl operator++(int) {
        auto cpy(this->_impl());
        IteratorAccess::increment(this->_pimpl());
        return cpy;
    }

    Impl& operator--() requires(is_bidirectional) {
        IteratorAccess::decrement(this->_pimpl());
        return this->_impl();
    }

    Impl operator--(int) requires(is_bidirectional) {
        auto cpy(this->_impl());
        IteratorAccess::decrement(this->_pimpl());
        return cpy;
    }

    Reference operator[](Difference n) const requires(is_random_access) {
        auto cpy(this->_impl());
        cpy += n;
        return *cpy;
    }

    Impl& operator+=(Difference n) requires(is_random_access) {
        IteratorAccess::advance(this->_pimpl(), n);
        return this->_impl();
    }

    Impl& operator-=(Difference n) requires(is_random_access) {
        IteratorAccess::advance(this->_pimpl(), -n);
        return this->_impl();
    }

    Impl operator+(Difference n) const requires(is_random_access) {
        auto cpy(this->_impl());
        cpy += n;
        return cpy;
    }

    Impl operator-(Difference n) const requires(is_random_access) {
        auto cpy(this->_impl());
        cpy -= n;
        return cpy;
    }

    friend Impl operator+(Difference n, const IteratorFacade& it) requires(is_random_access) {
        return it + n;
    }

    template<Concepts::Interoperable<Impl> I, typename V, typename R, typename D>
    friend auto operator-(const IteratorFacade& lhs,
                          const IteratorFacade<I, V, R, D>& rhs) requires(is_random_access) {
        if constexpr (std::is_convertible_v<I, Impl>)
            return IteratorAccess::distance<Difference>(
                Detail::cast_to_impl_ptr<const Impl>(&rhs),
                lhs._pimpl()
            );
        else
            return IteratorAccess::distance<D>(
                rhs._pimpl(),
                Detail::cast_to_impl_ptr<const I>(&lhs)
            );
    }

    template<Concepts::Interoperable<Impl> I, typename V, typename R, typename D>
    friend std::strong_ordering operator<=>(const IteratorFacade& lhs,
                                            const IteratorFacade<I, V, R, D>& rhs) requires(is_random_access) {
        const auto d = lhs - rhs;
        return d <=> decltype(d){0};
    }
};

/*!
 * \brief Base class to turn a type into a forward iterator by means of CRTP.
 * \tparam Impl The derived (implementation) type
 * \tparam ValueType The type over which it is iterated
 * \tparam Reference The reference type returned after dereferencing
 * \tparam Difference The type to express differences between two iterators
 */
template<typename Impl,
         typename ValueType,
         typename Reference = ValueType&,
         typename Pointer = std::remove_reference_t<Reference>*,
         typename Difference = std::ptrdiff_t>
using ForwardIteratorFacade = IteratorFacade<Impl,
                                             std::forward_iterator_tag,
                                             ValueType,
                                             Reference,
                                             Pointer,
                                             Difference>;

/*!
 * \brief Base class to turn a type into a bidirectional iterator by means of CRTP.
 * \tparam Impl The derived (implementation) type
 * \tparam ValueType The type over which it is iterated
 * \tparam Reference The reference type returned after dereferencing
 * \tparam Difference The type to express differences between two iterators
 */
template<typename Impl,
         typename ValueType,
         typename Reference = ValueType&,
         typename Pointer = std::remove_reference_t<Reference>*,
         typename Difference = std::ptrdiff_t>
using BidirectionalIteratorFacade = IteratorFacade<Impl,
                                                   std::bidirectional_iterator_tag,
                                                   ValueType,
                                                   Reference,
                                                   Pointer,
                                                   Difference>;

/*!
 * \brief Base class to turn a type into a random-access iterator by means of CRTP.
 * \tparam Impl The derived (implementation) type
 * \tparam ValueType The type over which it is iterated
 * \tparam Reference The reference type returned after dereferencing
 * \tparam Difference The type to express differences between two iterators
 */
template<typename Impl,
         typename ValueType,
         typename Reference = ValueType&,
         typename Pointer = std::remove_reference_t<Reference>*,
         typename Difference = std::ptrdiff_t>
using RandomAccessIteratorFacade = IteratorFacade<Impl,
                                                  std::random_access_iterator_tag,
                                                  ValueType,
                                                  Reference,
                                                  Pointer,
                                                  Difference>;

//! @} end Common group

}  // end namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ITERATOR_FACADES_HPP_
