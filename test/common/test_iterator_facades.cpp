// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <concepts>
#include <ranges>
#include <vector>
#include <algorithm>
#include <type_traits>

#include <gridformat/common/iterator_facades.hpp>

#include "../testing.hpp"

struct IntWrapper {
    int _value;
    int get() const { return _value; }
    void set(int val) { _value = val; }
    bool operator==(const IntWrapper& w) const { return _value == w.get(); }
    bool operator==(int value) const { return _value == value; }
};

template<typename RandomAccessContainer>
class SubSetIteratorBase {
 public:
    SubSetIteratorBase() = default;
    SubSetIteratorBase(const SubSetIteratorBase&) = default;
    SubSetIteratorBase(RandomAccessContainer* container,
                       const std::vector<std::size_t>& indices,
                       bool is_end_iterator = false)
    : _container(container)
    , _indices(&indices)
    , _it{(is_end_iterator ? _indices->end() : _indices->begin())}
    {}

 protected:
    RandomAccessContainer* _container{nullptr};
    const std::vector<std::size_t>* _indices{nullptr};
    typename std::vector<std::size_t>::const_iterator _it;
};

template<typename RandomAccessContainer,
         typename Reference = typename RandomAccessContainer::value_type&>
class ForwardSubSetIterator
: public SubSetIteratorBase<RandomAccessContainer>
, public GridFormat::ForwardIteratorFacade<
        ForwardSubSetIterator<RandomAccessContainer, Reference>,
        std::decay_t<Reference>,
        Reference> {
    using Base = SubSetIteratorBase<RandomAccessContainer>;

 public:
    using Base::Base;

 private:
    friend class GridFormat::IteratorAccess;

    void _increment() {
        ++(this->_it);
    }

    decltype(auto) _dereference() const {
        return (*this->_container)[*(this->_it)];
    }

    bool _is_equal(const ForwardSubSetIterator& other) const {
        return this->_it == other._it;
    }
};

template<typename RandomAccessContainer,
         typename Reference = typename RandomAccessContainer::value_type&>
class BidirectionalSubSetIterator
: public SubSetIteratorBase<RandomAccessContainer>
, public GridFormat::BidirectionalIteratorFacade<
        BidirectionalSubSetIterator<RandomAccessContainer, Reference>,
        std::decay_t<Reference>,
        Reference> {
    using Base = SubSetIteratorBase<RandomAccessContainer>;

 public:
    using Base::Base;

 private:
    friend class GridFormat::IteratorAccess;

    void _increment() {
        ++(this->_it);
    }

    void _decrement() {
        --(this->_it);
    }

    decltype(auto) _dereference() const {
        return (*this->_container)[*(this->_it)];
    }

    bool _is_equal(const BidirectionalSubSetIterator& other) const {
        return this->_it == other._it;
    }
};

template<typename RandomAccessContainer,
         typename Reference = typename RandomAccessContainer::value_type&>
class RandomAccessSubSetIterator
: public SubSetIteratorBase<RandomAccessContainer>
, public GridFormat::RandomAccessIteratorFacade<
        RandomAccessSubSetIterator<RandomAccessContainer, Reference>,
        std::decay_t<Reference>,
        Reference> {
    using Base = SubSetIteratorBase<RandomAccessContainer>;

 public:
    using Base::Base;

 private:
    friend class GridFormat::IteratorAccess;

    void _increment() {
        ++(this->_it);
    }

    void _decrement() {
        --(this->_it);
    }

    template<std::signed_integral I>
    void _advance(I n) {
        this->_it += n;
    }

    decltype(auto) _dereference() const {
        return (*this->_container)[*(this->_it)];
    }

    int _distance_to(const RandomAccessSubSetIterator& other) const {
        return other._it - this->_it;
    }

    bool _is_equal(const RandomAccessSubSetIterator& other) const {
        return this->_it == other._it;
    }
};

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "forward_iterator"_test = [] () {
        using IntWrapperVector = std::vector<IntWrapper>;
        using Iterator = ForwardSubSetIterator<IntWrapperVector>;
        using ConstIterator = ForwardSubSetIterator<const IntWrapperVector, const IntWrapper&>;
        static_assert(std::forward_iterator<Iterator>);
        static_assert(std::forward_iterator<ConstIterator>);

        IntWrapperVector numbers{{{0}, {1}, {2}, {3}, {4}}};
        std::vector<std::size_t> indices{{0, 1, 2, 3, 4}};

        Iterator it{&numbers, indices};
        expect(eq(*it, 0));
        expect(eq(it->get(), 0));

        Iterator it2{&numbers, indices};
        expect(it == it2);
        expect(++it != it2);

        Iterator it3{&numbers, indices};
        expect(it3 == it2);
        expect(it3++ == it2);
        expect(it3 != it2);
    };

    "forward_iterator_facade_mutable"_test = [] () {
        using IntWrapperVector = std::vector<IntWrapper>;
        IntWrapperVector numbers{{{0}, {1}, {2}, {3}, {4}}};
        std::vector<std::size_t> indices{{0, 1, 2, 3, 4}};

        ForwardSubSetIterator<IntWrapperVector> it{&numbers, indices};
        expect(eq(*it, 0));
        expect(it->get() == 0);

        (*it).set(1);
        expect(eq(*it, 1));
        expect(eq(it->get(), 1));

        it->set(2);
        expect(eq(*it, 2));
        expect(eq(it->get(), 2));
    };

    "forward_iterator_return_by_value"_test = [] () {
        using IntWrapperVector = std::vector<IntWrapper>;
        IntWrapperVector numbers{{{0}, {1}, {2}, {3}, {4}}};
        std::vector<std::size_t> indices{{0, 1, 2, 3, 4}};
        ForwardSubSetIterator<IntWrapperVector, IntWrapper> it{&numbers, indices};
        expect(std::equal(
            numbers.begin(),
            numbers.end(),
            it
        ));
    };

    "bidirectional_iterator"_test = [] () {
        std::vector<int> numbers{{0, 1, 2, 3, 4}};
        std::vector<std::size_t> indices{{0, 1, 2, 3, 4}};
        static_assert(std::bidirectional_iterator<BidirectionalSubSetIterator<std::vector<int>>>);

        BidirectionalSubSetIterator<std::vector<int>> it{&numbers, indices};
        expect(eq(*(++(++it)), 2));
        expect(eq(*(--it), 1));
        expect(eq(*(--it), 0));
    };

    "random_access_iterator"_test = [] () {
        std::vector<int> numbers{{0, 1, 2, 3, 4}};
        std::vector<std::size_t> indices{{0, 1, 2, 3, 4}};
        static_assert(std::random_access_iterator<RandomAccessSubSetIterator<std::vector<int>>>);

        RandomAccessSubSetIterator<std::vector<int>> it{&numbers, indices};
        expect(eq(*it, 0));
        it += 2;
        expect(eq(*it, 2));
        expect(eq(*(it - 2), 0));

        it -= 2;
        expect(eq(*it, 0));
        expect(eq(*(it + 3), 3));
        expect(eq(*(3 + it), 3));
        expect(eq(*it, 0));

        auto it2 = it;
        expect(eq((++(++it2) - it), 2));
        expect(eq((it - it2), -2));

        expect(eq(it[2], 2));
        expect(eq(it2[2], 4));

        expect(it < it2);
        expect(it2 > it);

        expect(it <= it);
        expect(it >= it);
        expect(it2 <= it2);
        expect(it2 >= it2);
    };

    return 0;
}
