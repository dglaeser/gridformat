// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>
#include <ranges>

namespace Example {

struct IndexInterval {
    const std::size_t min;
    const std::size_t max;

    std::size_t size() const { return max - min; }

    IndexInterval(std::size_t max) : IndexInterval(std::size_t{0}, max) {}
    IndexInterval(std::size_t min, std::size_t max) : min(min), max(max) {
        if (max < min)
            throw std::runtime_error("It must be max >= min");
    }
};

class Image {
 public:
    struct Location {
        std::size_t x;
        std::size_t y;
    };

    struct Window {
        IndexInterval x_interval;
        IndexInterval y_interval;

        std::size_t size() const {
            return x_interval.size()*y_interval.size();
        }
    };

    explicit Image(std::size_t nx, std::size_t ny) {
        if (ny == 0 || nx == 0)
            throw std::runtime_error("All extents must be > 0");
        _data.resize(ny);
        std::ranges::for_each(_data, [&] (auto& row) { row.resize(nx, 0.0); });
    }

    std::size_t size_x() const { return _data[0].size(); }
    std::size_t size_y() const { return _data.size(); }

    double get(const Location& loc) const {
        return _data.at(loc.y).at(loc.x);
    }

    void set(const Location& loc, double value) {
        _data.at(loc.y).at(loc.x) = value;
    }

    // Note: with cpp23, cartesian_product_view would make constructing index tuple
    // ranges much easier: https://en.cppreference.com/w/cpp/ranges/cartesian_product_view
    // This implementation is not very efficient but suffices for the purposes of the example
    static std::ranges::range auto locations_in(const Window& window) {
        const auto nx = window.x_interval.size();
        const auto ny = window.y_interval.size();
        return std::views::iota(std::size_t{0}, nx*ny) | std::views::transform([=] (const std::size_t i) {
            return Image::Location{
                .x = window.x_interval.min + i%nx,
                .y = window.y_interval.min + i/nx
            };
        });
    }

 private:
    std::vector<std::vector<double>> _data;
};

}  // namespace Example
