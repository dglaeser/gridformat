#include <ranges>
#include <vector>



#include <iostream>



#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/md_index.hpp>
#include "../testing.hpp"



int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;
    using GridFormat::MDIndex;

    "md_index_construct_from_range"_test = [] () {
        MDIndex index{std::views::iota(0, 3)};
        expect(eq(index.size(), std::size_t{3}));
        for (std::size_t i = 0; i < 3; ++i)
            expect(eq(index.get(i), i));
    };

    "md_index_construct_from_md_layout"_test = [] () {
        MDIndex index{GridFormat::MDLayout{std::vector{1, 1, 1}}};
        expect(eq(index.size(), std::size_t{3}));
    };

    "md_index_construct_from_integral"_test = [] () {
        MDIndex index{std::views::iota(0, 3)};
        expect(eq(index.size(), std::size_t{3}));
    };

    "md_index_construct_from_indices"_test = [] () {
        MDIndex index{std::vector<std::size_t>{1, 2, 3, 4}};
        expect(eq(index.size(), std::size_t{4}));
        for (std::size_t i = 0; i < 4; ++i)
            expect(eq(index.get(i), i + 1));
    };

    "md_index_construct_from_initializer_list"_test = [] () {
        MDIndex index{{2, 3, 4, 5}};
        expect(eq(index.size(), std::size_t{4}));
        for (std::size_t i = 0; i < 4; ++i)
            expect(eq(index.get(i), i + 2));
    };

    "md_index_setter"_test = [] () {
        MDIndex index{std::vector<std::size_t>{1, 2, 3, 4}};
        index.set(0, 42);
        expect(eq(index.get(0), std::size_t{42}));
        for (std::size_t i = 1; i < 4; ++i)
            expect(eq(index.get(i), i + 1));
    };

    "md_index_flat"_test = [] () {
        expect(eq(
            GridFormat::flat_index(MDIndex{{{0}}}, GridFormat::MDLayout{std::vector{{1}}}),
            std::size_t{0}
        ));
        expect(eq(
            GridFormat::flat_index(MDIndex{{{1}}}, GridFormat::MDLayout{std::vector{{2}}}),
            std::size_t{1}
        ));
        expect(eq(
            GridFormat::flat_index(MDIndex{{{0, 1}}}, GridFormat::MDLayout{std::vector{{2, 2}}}),
            std::size_t{1}
        ));
        expect(eq(
            GridFormat::flat_index(MDIndex{{{1, 0}}}, GridFormat::MDLayout{std::vector{{2, 2}}}),
            std::size_t{2}
        ));
        expect(eq(
            GridFormat::flat_index(MDIndex{{{1, 1}}}, GridFormat::MDLayout{std::vector{{2, 2}}}),
            std::size_t{3}
        ));
        expect(eq(
            GridFormat::flat_index(MDIndex{{{1, 2, 3}}}, GridFormat::MDLayout{std::vector{{2, 3, 4}}}),
            std::size_t{23}
        ));
    };

    "md_index_range_1d"_test = [] () {
        GridFormat::MDIndexRange range{GridFormat::MDLayout{std::vector<int>{3}}};
        expect(std::ranges::equal(
            range,
            std::vector<MDIndex>{
                MDIndex{{0}},
                MDIndex{{1}},
                MDIndex{{2}}
            }
        ));
    };

    "md_index_range_2d"_test = [] () {
        GridFormat::MDIndexRange range{GridFormat::MDLayout{std::vector<int>{3, 2}}};
        expect(std::ranges::equal(
            range,
            std::vector<MDIndex>{
                MDIndex{{0, 0}},
                MDIndex{{0, 1}},
                MDIndex{{1, 0}},
                MDIndex{{1, 1}},
                MDIndex{{2, 0}},
                MDIndex{{2, 1}}
            }
        ));
    };

    "md_index_range_3d"_test = [] () {
        expect(std::ranges::equal(
            GridFormat::indices(GridFormat::MDLayout{std::vector<int>{3, 2, 3}}),
            std::vector<MDIndex>{
                MDIndex{{0, 0, 0}},
                MDIndex{{0, 0, 1}},
                MDIndex{{0, 0, 2}},
                MDIndex{{0, 1, 0}},
                MDIndex{{0, 1, 1}},
                MDIndex{{0, 1, 2}},

                MDIndex{{1, 0, 0}},
                MDIndex{{1, 0, 1}},
                MDIndex{{1, 0, 2}},
                MDIndex{{1, 1, 0}},
                MDIndex{{1, 1, 1}},
                MDIndex{{1, 1, 2}},

                MDIndex{{2, 0, 0}},
                MDIndex{{2, 0, 1}},
                MDIndex{{2, 0, 2}},
                MDIndex{{2, 1, 0}},
                MDIndex{{2, 1, 1}},
                MDIndex{{2, 1, 2}}
            }
        ));
    };

    "md_index_range_3d_reversed"_test = [] () {
        expect(std::ranges::equal(
            GridFormat::indices(GridFormat::MDLayout{std::vector<int>{3, 2, 3}}).reversed(),
            std::vector<MDIndex>{
                MDIndex{{0, 0, 0}},
                MDIndex{{0, 0, 1}},
                MDIndex{{0, 0, 2}},
                MDIndex{{0, 1, 0}},
                MDIndex{{0, 1, 1}},
                MDIndex{{0, 1, 2}},

                MDIndex{{1, 0, 0}},
                MDIndex{{1, 0, 1}},
                MDIndex{{1, 0, 2}},
                MDIndex{{1, 1, 0}},
                MDIndex{{1, 1, 1}},
                MDIndex{{1, 1, 2}},

                MDIndex{{2, 0, 0}},
                MDIndex{{2, 0, 1}},
                MDIndex{{2, 0, 2}},
                MDIndex{{2, 1, 0}},
                MDIndex{{2, 1, 1}},
                MDIndex{{2, 1, 2}}
            } | std::views::reverse
        ));
    };

    return 0;
}
