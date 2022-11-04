#include <array>
#include <boost/ut.hpp>
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

}  // namespace GridFormat::Traits

int main() {
    static_assert(GridFormat::Concepts::UnstructuredGrid<MockGrid>);
    return 0;
}