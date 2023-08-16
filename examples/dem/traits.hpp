// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#pragma once

#include "dem.hpp"

// Here we implement the traits required to fulfill the `StructuredGrid` concept
// for the DEM data structure of this example.
namespace GridFormat::Traits {

template<>
struct Cells<Example::DEM> {
    static std::ranges::range auto get(const Example::DEM& dem) {
        return cells(dem.raster());
    }
};

template<>
struct Points<Example::DEM> {
    static std::ranges::range auto get(const Example::DEM& dem) {
        return points(dem.raster());
    }
};

template<>
struct Extents<Example::DEM> {
    static auto get(const Example::DEM& dem) {
        return std::array{
            dem.raster().number_of_cells(0),
            dem.raster().number_of_cells(1)
        };
    }
};

template<typename Entity>
struct Location<Example::DEM, Entity> {
    static auto get(const Example::DEM&, const Entity& entity) {
        return std::array{entity.x, entity.y};
    }
};

template<>
struct PointCoordinates<Example::DEM, typename Example::Raster::Point> {
    static auto get(const Example::DEM& dem, const typename Example::Raster::Point& point) {
        const auto p = dem(point);
        return std::array{p.x, p.y, p.z};
    }
};

}  // namespace GridFormat::Traits
