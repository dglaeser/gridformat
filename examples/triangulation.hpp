// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <vector>
#include <ranges>
#include <functional>

#include <gridformat/common/concepts.hpp>
#include <gridformat/vtk/vtu_writer.hpp>

namespace GridFormatExamples {

// Exemplary implementation of a triangulation
class Triangulation {
 public:
    class Coordinate {
     public:
        Coordinate(std::array<double, 2>&& c)
        : _c{std::move(c)}
        {}

        double& operator[] (std::integral auto i) { return _c[i]; }
        const double& operator[] (std::integral auto i) const { return _c[i]; }

        Coordinate& operator+=(const Coordinate& other) {
            return _apply(other, std::plus{});
        }

        Coordinate& operator/=(const GridFormat::Concepts::Scalar auto s) {
            return _apply({{s, s}}, std::divides{});
        }

        auto begin() { return _c.begin(); }
        auto begin() const { return _c.begin(); }

        auto end() { return _c.end(); }
        auto end() const { return _c.end(); }

        operator const std::array<double, 2>&() const { return _c; }
        static constexpr std::size_t size() { return 2; }

     private:
        template<typename BinaryOperator>
        Coordinate& _apply(const Coordinate& other, const BinaryOperator& op) {
            std::ranges::for_each(std::array<int, 2>{0, 1}, [&] (auto i) {
                _c[i] = op(_c[i], other[i]);
            });
            return *this;
        }

        std::array<double, 2> _c;
    };

    struct Vertex {
        std::size_t id;
        Coordinate position;
    };

    struct Cell {
        std::size_t id;
        std::array<std::size_t, 3> connectivity;
    };

    Triangulation(const std::vector<Coordinate>& points,
                  const std::vector<std::array<std::size_t, 3>>& cells) {
        _vertices.reserve(points.size());
        for (std::size_t p_id = 0; p_id < points.size(); ++p_id)
            _vertices.push_back(Vertex{.id = p_id, .position = points[p_id]});
        _cells.reserve(cells.size());
        for (std::size_t c_id = 0; c_id < cells.size(); ++c_id)
            _cells.push_back(Cell{.id = c_id, .connectivity = cells[c_id]});
    }

    std::size_t number_of_cells() const { return _vertices.size(); }
    std::size_t number_of_vertices() const { return _cells.size(); }

    const std::vector<Vertex>& vertices() const { return _vertices; }
    const std::vector<Cell>& cells() const { return _cells; }

    Coordinate center(const Cell& c) const {
        Coordinate result{{0.0, 0.0}};
        std::ranges::for_each(c.connectivity, [&] (auto vertex_id) {
            result += _vertices[vertex_id].position;
        });
        result /= static_cast<double>(c.connectivity.size());
        return result;
    }

    std::ranges::range auto corners(const Cell& c) const {
        return c.connectivity | std::views::transform([&] (auto vertex_id) {
            return _vertices[vertex_id];
        });
    }

 private:
    std::vector<Vertex> _vertices;
    std::vector<Cell> _cells;
};

}  // namespace GridFormatExamples

// Let's register Triangulation as UnstructuredGrid
namespace GridFormat::Traits {

template<>
struct Points<GridFormatExamples::Triangulation> {
    static std::ranges::viewable_range auto get(const GridFormatExamples::Triangulation& grid) {
        return grid.vertices();
    }
};

template<>
struct Cells<GridFormatExamples::Triangulation> {
    static std::ranges::viewable_range auto get(const GridFormatExamples::Triangulation& grid) {
        return grid.cells();
    }
};

template<>
struct CellType<GridFormatExamples::Triangulation, typename GridFormatExamples::Triangulation::Cell> {
    static auto get(const GridFormatExamples::Triangulation&,
                    const typename GridFormatExamples::Triangulation::Cell&) {
        return GridFormat::CellType::triangle;
    }
};

template<>
struct CellPoints<GridFormatExamples::Triangulation, typename GridFormatExamples::Triangulation::Cell> {
    static auto get(const GridFormatExamples::Triangulation& grid,
                    const typename GridFormatExamples::Triangulation::Cell& c) {
        return grid.corners(c);
    }
};

template<>
struct PointCoordinates<GridFormatExamples::Triangulation, typename GridFormatExamples::Triangulation::Vertex> {
    static auto get(const GridFormatExamples::Triangulation& grid,
                    const typename GridFormatExamples::Triangulation::Vertex& v) {
        return v.position;
    }
};

template<>
struct PointId<GridFormatExamples::Triangulation, typename GridFormatExamples::Triangulation::Vertex> {
    static auto get(const GridFormatExamples::Triangulation&,
                    const typename GridFormatExamples::Triangulation::Vertex& v) {
        return v.id;
    }
};

}  // namespace GridFormat::Traits
