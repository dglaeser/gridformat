// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cmath>
#include <numbers>

#include <gridformat/gridformat.hpp>

#include "dem.hpp"
#include "traits.hpp"


double elevation_at(const Example::GeoCoordinate& coord) {
    const auto lon_radians = coord.longitude*std::numbers::pi/180.0;
    const auto lat_radians = coord.latitude*std::numbers::pi/180.0;
    return std::sin(std::numbers::pi*2.0*lon_radians)
        *std::cos(std::numbers::pi*10.0*lat_radians)
        *0.05
        + 0.04;
}


int main() {
    static_assert(GridFormat::Concepts::StructuredGrid<Example::DEM>);
    Example::DEM dem{{
        {
            .from = {0.0, 0.0},
            .to = {35.0, 35.0}
        },
        {50, 50}
    }};

    std::ranges::for_each(points(dem.raster()), [&] (const Example::Raster::Point& p) {
        const auto position = dem.raster().to_map(dem.raster().center(p));
        dem.set_elevation_at(p, elevation_at(position));
    });

    GridFormat::Writer writer{GridFormat::default_for(dem), dem};
    writer.set_point_field("elevation", [&] (const auto& point) {
        return dem.get_elevation_at(point);
    });
    writer.write("dem");

    return 0;
}
