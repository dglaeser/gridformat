// Copyright [2022] <Dennis Gläser>

#include <concepts>
#include <ranges>
#include <vector>
#include <algorithm>
#include <type_traits>

#include "suite.hpp"

#include <gridformat/common/iterator_range.hpp>
#include <gridformat/common/iterator_facades.hpp>

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


void test_iterator_range_construction(GridFormat::TestSuite& suite) {
    std::vector<int> raw{{1, 2, 3, 4, 5}};
    auto wrapped = GridFormat::make_range(raw.begin(), raw.end());
    suite.check([&](){ return std::ranges::distance(wrapped) == 5; });
    suite.check([&](){ return std::ranges::size(wrapped) ==5; });
}


void test_iterator_range_equality(GridFormat::TestSuite& suite) {
    std::vector<int> raw{{1, 2, 3, 4, 5}};
    auto wrapped = GridFormat::make_range(raw.begin(), raw.end());
    auto wrapped_sub = GridFormat::make_range(raw.begin()+1, raw.end()-1);
    suite.check([&](){ return std::ranges::equal(wrapped, raw); });
    suite.check([&](){ return std::ranges::equal(wrapped_sub, std::vector<int>{{2, 3, 4}}); });
}


void test_iterator_range_mutable(GridFormat::TestSuite& suite) {
    std::vector<int> modifyable{{1, 2, 3}};
    std::ranges::for_each(modifyable, [] (auto& v) { v *= 2; });
    suite.check([&](){
        return std::ranges::equal(modifyable, std::vector<int>{{2, 4, 6}});
    });
}


void test_forward_iterator_facade_operations(GridFormat::TestSuite& suite) {
    using IntWrapperVector = std::vector<IntWrapper>;
    using Iterator = ForwardSubSetIterator<IntWrapperVector>;
    using ConstIterator = ForwardSubSetIterator<const IntWrapperVector, const IntWrapper&>;
    static_assert(std::forward_iterator<Iterator>);
    static_assert(std::forward_iterator<ConstIterator>);

    IntWrapperVector numbers{{{0}, {1}, {2}, {3}, {4}}};
    std::vector<std::size_t> indices{{0, 1, 2, 3, 4}};

    Iterator it{&numbers, indices};
    suite.check([&](){ return *it == 0; });
    suite.check([&](){ return it->get() == 0; });

    Iterator it2{&numbers, indices};
    suite.check([&](){ return it == it2; });
    suite.check([&](){ return ++it != it2; });

    Iterator it3{&numbers, indices};
    suite.check([&](){ return it3 == it2; });
    suite.check([&](){ return it3++ == it2; });
    suite.check([&](){ return it3 != it2; });

    auto all_numbers = GridFormat::make_range(
        Iterator{&numbers, indices},
        Iterator{&numbers, indices, true}
    );
    suite.check([&](){ return std::ranges::equal(all_numbers, numbers); });
    auto all_numbers_const = GridFormat::make_range(
        ConstIterator{&numbers, indices},
        ConstIterator{&numbers, indices, true}
    );
    suite.check([&](){
        return std::ranges::equal(all_numbers_const, numbers);
    });
}


void test_forward_iterator_facade_mutable(GridFormat::TestSuite&  suite) {
    using IntWrapperVector = std::vector<IntWrapper>;
    IntWrapperVector numbers{{{0}, {1}, {2}, {3}, {4}}};
    std::vector<std::size_t> indices{{0, 1, 2, 3, 4}};

    ForwardSubSetIterator<IntWrapperVector> it{&numbers, indices};
    suite.check([&](){ return *it == 0; });
    suite.check([&](){ return it->get() == 0; });

    (*it).set(1);
    suite.check([&](){ return *it == 1; });
    suite.check([&](){ return it->get() == 1; });

    it->set(2);
    suite.check([&](){ return *it == 2; });
    suite.check([&](){ return it->get() == 2; });
}


void test_forward_iterator_facade_return_by_value(GridFormat::TestSuite& suite) {
    using IntWrapperVector = std::vector<IntWrapper>;
    IntWrapperVector numbers{{{0}, {1}, {2}, {3}, {4}}};
    std::vector<std::size_t> indices{{0, 1, 2, 3, 4}};

    auto all_numbers = GridFormat::make_range(
        ForwardSubSetIterator<IntWrapperVector, IntWrapper>{&numbers, indices},
        ForwardSubSetIterator<IntWrapperVector, IntWrapper>{&numbers, indices, true}
    );
    suite.check([&](){
        return std::ranges::equal(all_numbers, numbers);
    });
}


void test_bidirectional_iterator_facade_operations(GridFormat::TestSuite& suite) {
    std::vector<int> numbers{{0, 1, 2, 3, 4}};
    std::vector<std::size_t> indices{{0, 1, 2, 3, 4}};
    static_assert(std::bidirectional_iterator<BidirectionalSubSetIterator<std::vector<int>>>);

    BidirectionalSubSetIterator<std::vector<int>> it{&numbers, indices};
    suite.check([&](){ return*(++(++it)) == 2; });
    suite.check([&](){ return*(--it) == 1; });
    suite.check([&](){ return*(--it) == 0; });
}


void test_random_access_iterator_facade_operations(GridFormat::TestSuite& suite) {
    std::vector<int> numbers{{0, 1, 2, 3, 4}};
    std::vector<std::size_t> indices{{0, 1, 2, 3, 4}};
    static_assert(std::random_access_iterator<RandomAccessSubSetIterator<std::vector<int>>>);

    RandomAccessSubSetIterator<std::vector<int>> it{&numbers, indices};
    suite.check([&](){ return *it == 0; });
    it += 2;
    suite.check([&](){ return *it == 2; });
    suite.check([&](){ return *(it - 2) == 0; });

    it -= 2;
    suite.check([&](){ return *it == 0; });
    suite.check([&](){ return *(it + 3) == 3; });
    suite.check([&](){ return *(3 + it) == 3; });
    suite.check([&](){ return *it == 0; });

    auto it2 = it;
    suite.check([&](){ return (++(++it2) - it) == 2; });
    suite.check([&](){ return (it - it2) == -2; });

    suite.check([&](){ return it[2] == 2; });
    suite.check([&](){ return it2[2] == 4; });

    suite.check([&](){ return it < it2; });
    suite.check([&](){ return it2 > it; });

    suite.check([&](){ return it <= it; });
    suite.check([&](){ return it >= it; });
    suite.check([&](){ return it2 <= it2; });
    suite.check([&](){ return it2 >= it2; });
}


int main(int argc, char** argv) {
    GridFormat::TestSuite suite("iterator_facades", argc, argv);

    test_iterator_range_construction(suite);
    test_iterator_range_equality(suite);
    test_iterator_range_mutable(suite);
    test_forward_iterator_facade_operations(suite);
    test_forward_iterator_facade_mutable(suite);
    test_forward_iterator_facade_return_by_value(suite);
    test_bidirectional_iterator_facade_operations(suite);
    test_random_access_iterator_facade_operations(suite);

    suite.log_summary();
    return suite.failed();
}