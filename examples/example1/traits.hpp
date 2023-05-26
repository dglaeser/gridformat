// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "image.hpp"

// Our image does not really carry a notion of cells/points.
// We simply use Image::Location as both cell & point type and
// return ranges over locations as cell/point ranges.

namespace GridFormat::Traits {

template<>
struct Cells<Example::Image> {
    static std::ranges::range auto get(const Example::Image& grid) {
        return Example::Image::locations_in({grid.size_x(), grid.size_y()});
    }
};

template<>
struct Points<Example::Image> {
    static std::ranges::range auto get(const Example::Image& grid) {
        return Example::Image::locations_in({grid.size_x() + 1, grid.size_y() + 1});
    }
};

template<>
struct Extents<Example::Image> {
    static auto get(const Example::Image& grid) {
        return std::array{grid.size_x(), grid.size_y()};
    }
};

template<>
struct Origin<Example::Image> {
    static auto get(const Example::Image& grid) {
        return std::array{0.0, 0.0};
    }
};

template<>
struct Spacing<Example::Image> {
    static auto get(const Example::Image& grid) {
        return std::array{1, 1};
    }
};

template<>
struct Location<Example::Image, Example::Image::Location> {
    static auto get(const Example::Image&, const Example::Image::Location& ituple) {
        return std::array{ituple.x, ituple.y};
    }
};

}  // namespace GridFormat::Traits
