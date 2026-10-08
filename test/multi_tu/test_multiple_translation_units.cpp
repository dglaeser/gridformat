// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT

#include <all_headers.hpp>
#include "other_translation_unit.hpp"

#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;

    "library_usable_from_multiple_translation_units"_test = [] () {
        GridFormat::ImageGrid<2, double> grid{{1.0, 1.0}, {2, 2}};
        GridFormat::Writer writer{GridFormat::vti, grid};
        const auto first = writer.write("multi_tu_first");
        const auto second = write_in_other_translation_unit("multi_tu_second");
        GridFormat::Reader reader;
        reader.open(first);
        expect(reader.number_of_cells() == 4);
        reader.open(second);
        expect(reader.number_of_cells() == 4);
    };

    return 0;
}
