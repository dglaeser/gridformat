// SPDX-FileCopyrightText: 2025 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <type_traits>

#include <gridformat/grid/filtered.hpp>
#include <gridformat/grid/grid.hpp>

#include "structured_grid.hpp"
#include "../testing.hpp"

int main() {
    using namespace GridFormat;
    using namespace GridFormat::Testing;

    const Test::StructuredGrid<3> test_grid{{1.0, 1.0, 1.0}, {4, 5, 6}};
    const FilteredGrid filtered{test_grid, [] (const auto& e) { return e.id == 0; }};

    static_assert(Concepts::UnstructuredGrid<std::remove_cvref_t<decltype(filtered)>>);
    expect(eq(number_of_cells(filtered), std::size_t{1}));

    return 0;
}
