// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <algorithm>
#include <ranges>

#include <gridformat/parallel/communication.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    const GridFormat::NullCommunicator comm{};
    int some_value = 1;
    "null_communicator_sum"_test = [&] () { expect(eq(GridFormat::Parallel::sum(comm, some_value), int{1})); };
    "null_communicator_min"_test = [&] () { expect(eq(GridFormat::Parallel::min(comm, some_value), int{1})); };
    "null_communicator_max"_test = [&] () { expect(eq(GridFormat::Parallel::max(comm, some_value), some_value)); };
    "null_communicator_broadcast"_test = [&] () {
        expect(eq(GridFormat::Parallel::broadcast(comm, some_value), some_value));
    };
    "null_communicator_gather"_test = [&] () {
        expect(std::ranges::equal(
            GridFormat::Parallel::gather(comm, some_value),
            std::vector<int>{some_value}
        ));
    };
    "null_communicator_gather_vec"_test = [&] () {
        std::vector vals{some_value};
        expect(std::ranges::equal(
            GridFormat::Parallel::gather(comm, vals),
            std::vector<int>{some_value}
        ));
    };
    "null_communicator_gather_range"_test = [&] () {
        expect(std::ranges::equal(
            GridFormat::Parallel::gather(comm, std::vector{some_value} | std::views::all),
            std::vector<int>{some_value}
        ));
    };

    "null_communicator_scatter"_test = [&] () {
        std::vector vals{some_value};
        expect(std::ranges::equal(
            GridFormat::Parallel::scatter(comm, vals),
            std::vector<int>{some_value}
        ));
    };
    "null_communicator_gather_vec"_test = [&] () {
        std::vector vals{some_value};
        expect(std::ranges::equal(
            GridFormat::Parallel::gather(comm, vals | std::views::all),
            std::vector<int>{some_value}
        ));
    };
    "null_communicator_gather_array"_test = [&] () {
        std::array<int, 1> vals{some_value};
        expect(std::ranges::equal(
            GridFormat::Parallel::gather(comm, vals),
            std::vector<int>{some_value}
        ));
    };

    return 0;
}
