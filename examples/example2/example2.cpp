// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <cmath>
#include <numbers>

#include <gridformat/gridformat.hpp>

#include "dem.hpp"
#include "traits.hpp"


// some dummy function to represent elevation data
double elevation_at(const Example::Geographic& coord) {
    const auto lon_radians = coord.longitude*std::numbers::pi/180.0;
    const auto lat_radians = coord.latitude*std::numbers::pi/180.0;
    return 0.04 + 0.05*std::sin(std::numbers::pi*5.0*lon_radians)
                      *std::cos(std::numbers::pi*10.0*lat_radians);
}


int main() {
    // Let us check that we implemented all traits for the StructuredGrid concept correctly
    static_assert(GridFormat::Concepts::StructuredGrid<Example::DEM>);

    // let's use a patch of 15 degrees latitude & longitude, starting at zero degrees lat/lon,
    // and discretized into 100 x 100 cells. The spheroid is just the unit sphere here...
    auto dem = Example::DEM::from(
        Example::Raster{
            {
                .lower_left = {0.0, 0.0},
                .upper_right = {15.0, 15.0}
            },
            {100, 100}
        },
        Example::Spheroid{1.0}
    );

    // let's add some artifical elevation data ...
    std::ranges::for_each(points(dem.raster()), [&] (const Example::Raster::Point& p) {
        const auto center = dem.raster().center(p);
        const auto position = dem.raster().to_geographic(center);
        dem.set_elevation_at(p, elevation_at(position));
    });

    // ... and construct a writer for our DEM.
    GridFormat::Writer writer{GridFormat::default_for(dem), dem};

    // let us add the elevation data as point field
    writer.set_point_field("elevation", [&] (const auto& point) {
        return dem.get_elevation_at(point);
    });

    // let's also add a bool field which is true wherever the elevation is positive
    writer.set_point_field("is_above_spheroid", [&] (const auto& point) {
        return dem.get_elevation_at(point) > 0.0;
    });

    // We can also tell GridFormat to write out the field with a specific precision.
    // This can be useful if you want to save some space at the cost of (potentially)
    // loosing some precision.
    writer.set_point_field("elevation_as_float32", [&] (const auto& point) {
        return dem.get_elevation_at(point);
    }, GridFormat::float32);

    std::cout << "Wrote '" << writer.write("dem") << "'" << std::endl;

#if GRIDFORMAT_HAVE_ZLIB
    // zlib has 10 compression levels from 0 to 9
    const auto compressor = GridFormat::Compression::zlib.with({.compression_level = 9});
    const auto format = GridFormat::default_for(dem).with({.compressor = compressor});
    GridFormat::Writer compressed_writer{format, dem};
    compressed_writer.set_point_field("elevation", [&] (const auto& point) {
        return dem.get_elevation_at(point);
    });
    std::cout << "Wrote '" << compressed_writer.write("dem_compressed") << "'" << std::endl;
#endif

    return 0;
}
