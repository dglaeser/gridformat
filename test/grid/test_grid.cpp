#include <array>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/grid/grid.hpp>

class MockGrid {
 public:
    using Point = std::array<double, 2>;
    struct Cell {};

    const auto& points() const { return _points; }
    const auto& cells() const { return _cells; }

    std::size_t id(const Point& p) const {
        return (p[0] == 0.0) ? 0 : 1;
    }

 private:
    std::array<Point, 2> _points{{
        {{0.0, 0.0}}, {{1.0, 1.0}}
    }};
    std::array<Cell, 1> _cells{};
};

namespace GridFormat::Traits {

template<>
struct Points<MockGrid> {
    static decltype(auto) get(const MockGrid& grid) {
        return grid.points();
    }
};

template<>
struct Cells<MockGrid> {
    static decltype(auto) get(const MockGrid& grid) {
        return grid.cells();
    }
};

template<>
struct PointCoordinates<MockGrid, typename MockGrid::Point> {
    static decltype(auto) get([[maybe_unused]] const MockGrid& grid, const typename MockGrid::Point& p) {
        return p;
    }
};

template<>
struct PointId<MockGrid, typename MockGrid::Point> {
    static decltype(auto) get(const MockGrid& grid, const typename MockGrid::Point& p) {
        return grid.id(p);
    }
};

template<>
struct CellType<MockGrid, typename MockGrid::Cell> {
    static auto get([[maybe_unused]] const MockGrid& grid,
                    [[maybe_unused]] const typename MockGrid::Cell& cell) {
        return GridFormat::CellType::segment;
    }
};

template<>
struct CellCornerPoints<MockGrid, typename MockGrid::Cell> {
    static auto get(const MockGrid& grid, [[maybe_unused]] const typename MockGrid::Cell& cell) {
        return grid.points();
    }
};

}  // namespace GridFormat::Traits

int main() {
    static_assert(GridFormat::Concepts::UnstructuredGrid<MockGrid>);

    std::size_t cell_count = 0;
    std::size_t point_count = 0;

    MockGrid grid;
    for ([[maybe_unused]] const auto& point : GridFormat::Traits::Points<MockGrid>::get(grid)) point_count++;
    for ([[maybe_unused]] const auto& cell : GridFormat::Traits::Cells<MockGrid>::get(grid)) cell_count++;
    if (point_count != 2) throw GridFormat::InvalidState("Unexpected point count");
    if (cell_count != 1) throw GridFormat::InvalidState("Unexpected cell count");

    return 0;
}