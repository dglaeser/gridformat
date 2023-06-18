// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <string>

#include <gridformat/common/indentation.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::eq;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::operator""_test;

    "indentation"_test = [] () {
        for (unsigned width : {0, 1, 2}) {
            for (unsigned level : {0, 1}) {
                std::string initial_indent = std::string(level*width, ' ');
                std::string delta_indent = std::string(width, ' ');
                GridFormat::Indentation ind({.width = width, .level = level});
                expect(eq(ind.get(), initial_indent));
                expect(eq(ind++.get(), initial_indent));
                expect(eq(ind.get(), initial_indent + delta_indent));
                expect(eq((++ind).get(), initial_indent + delta_indent + delta_indent));
                expect(eq(ind--.get(), initial_indent + delta_indent + delta_indent));
                expect(eq(ind.get(), initial_indent + delta_indent));
                expect(eq((--ind).get(), initial_indent));

                std::ostringstream stream;
                stream << ind;
                expect(eq(stream.str(), initial_indent));
            }
        }
    };

    return 0;
}