// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <ranges>
#include <utility>
#include <numbers>

namespace Example {

struct Cartesian {
    double x;
    double y;
    double z;
};

// TODO: Geographic
struct GeoCoordinate {
    double longitude;
    double latitude;
};

struct Patch {
    GeoCoordinate from;
    GeoCoordinate to;
};


class Raster {
    template<int codim>
    struct Entity { std::size_t x; std::size_t y; };

 public:
    using Cell = Entity<0>;
    using Point = Entity<2>;

    struct RasterCoordinate {
        double x;
        double y;
    };

    Raster(Patch patch, std::array<std::size_t, 2> number_of_samples)
    : _lower_left{std::move(patch.from)}
    , _upper_right{std::move(patch.to)}
    , _step{
        .longitude = (_upper_right.longitude - _lower_left.longitude)/static_cast<double>(number_of_samples[0]),
        .latitude = (_upper_right.latitude - _lower_left.latitude)/static_cast<double>(number_of_samples[1])
    }
    , _number_of_samples{std::move(number_of_samples)}
    {}

    std::size_t number_of_cells(int dir) const { return _number_of_samples.at(dir); }
    std::size_t number_of_cells() const { return number_of_cells(0)*number_of_cells(1); }
    std::size_t number_of_points() const { return (number_of_cells(0) + 1)*(number_of_cells(1) + 1); }

    RasterCoordinate center(const Point& p) const {
        return {
            .x = static_cast<double>(p.x),
            .y = static_cast<double>(p.y)
        };
    }

    RasterCoordinate center(const Cell& c) const {
        return {
            .x = static_cast<double>(c.x) + 0.5,
            .y = static_cast<double>(c.y) + 0.5
        };
    }

    friend std::ranges::range auto cells(const Raster& raster) {
        const auto nx = raster._number_of_samples[0];
        const auto ny = raster._number_of_samples[1];
        return std::views::iota(std::size_t{0}, nx*ny) | std::views::transform([&, nx] (const std::size_t i) {
            return Cell{
                .x = i%nx,
                .y = i/nx
            };
        });
    }

    friend std::ranges::range auto points(const Raster& raster) {
        const auto nx = raster._number_of_samples[0] + 1;
        const auto ny = raster._number_of_samples[1] + 1;
        return std::views::iota(std::size_t{0}, nx*ny) | std::views::transform([&, nx] (const std::size_t i) {
            return Point{
                .x = i%nx,
                .y = i/nx
            };
        });
    }

    GeoCoordinate to_map(const RasterCoordinate& r) const {
        return {
            .longitude = r.x*_step.longitude,
            .latitude = r.y*_step.latitude
        };
    }

    RasterCoordinate to_raster(const GeoCoordinate& c) const {
        return {
            .x = number_of_cells(0)*(c.longitude - _lower_left.longitude)
                                   /(_upper_right.longitude - _lower_left.longitude),
            .y = number_of_cells(1)*(c.latitude - _lower_left.latitude)
                                   /(_upper_right.latitude - _lower_left.latitude)
        };
    }

 private:
    GeoCoordinate _lower_left;
    GeoCoordinate _upper_right;
    GeoCoordinate _step;
    std::array<std::size_t, 2> _number_of_samples;
    std::vector<std::vector<double>> _values;
};


class DEM {
    static constexpr double sphere_radius = 1.0;

 public:
    explicit DEM(Raster&& r)
    : _raster{std::move(r)}
    , _values(_raster.number_of_points(), 0.0)
    {}

    void set_elevation_at(const typename Raster::Point& p, double value) {
        _values.at(_index(p)) = value;
    }

    double get_elevation_at(const typename Raster::Point& p) const {
        return _values.at(_index(p));
    }

    Cartesian cartesian(const typename Raster::Point& p) const {
        const auto geo = _raster.to_map(_raster.center(p));
        const auto lon_rad = geo.longitude*std::numbers::pi/180.0;
        const auto lat_rad = geo.latitude*std::numbers::pi/180.0;
        const double x = sphere_radius*std::cos(lat_rad)*std::cos(lon_rad);
        const double y = sphere_radius*std::cos(lat_rad)*std::sin(lon_rad);
        const double z = sphere_radius*std::sin(lat_rad);
        const double length = std::sqrt(x*x + y*y + z*z);
        const double elevation = get_elevation_at(p);
        return {
            .x = x/length*(length + elevation),
            .y = y/length*(length + elevation),
            .z = z/length*(length + elevation),
        };
    }

    const Raster& raster() const {
        return _raster;
    }

 private:
    std::size_t _index(const typename Raster::Point& p) const {
        const auto r = _raster.center(p);
        const std::size_t ix = static_cast<std::size_t>(std::round(r.x));
        const std::size_t iy = static_cast<std::size_t>(std::round(r.y));
        return iy*_raster.number_of_cells(0) + ix;
    }

    Raster _raster;
    std::vector<double> _values;
};

}  // namespace Example
