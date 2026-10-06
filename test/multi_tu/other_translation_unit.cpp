// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT

#include <all_headers.hpp>
#include "other_translation_unit.hpp"

std::string write_in_other_translation_unit(const std::string& filename) {
    GridFormat::ImageGrid<2, double> grid{{1.0, 1.0}, {2, 2}};
    GridFormat::Writer writer{GridFormat::vti, grid};
    return writer.write(filename);
}
