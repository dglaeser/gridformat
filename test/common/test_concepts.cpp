#include <ostream>
#include <vector>
#include <array>

#include <gridformat/common/concepts.hpp>

struct NonStreamable {};
struct Streamable {
    friend std::ostream& operator<<(std::ostream& s, const Streamable&) {
        return s;
    }
};

struct MyStream {
    template<typename T, std::size_t extent>
    void write(std::span<const T, extent>) {

    }
};

struct ConvertibleToDouble {
    operator double() const {
        return 1.0;
    }
};

int main() {

    static_assert(GridFormat::Concepts::StaticallySizedRange<std::array<int, 2>>);
    static_assert(GridFormat::Concepts::StaticallySizedRange<std::span<int, 5>>);
    static_assert(GridFormat::Concepts::StaticallySizedRange<std::array<std::array<int, 2>, 2>>);
    static_assert(!GridFormat::Concepts::StaticallySizedRange<std::vector<int>>);
    static_assert(!GridFormat::Concepts::StaticallySizedRange<std::span<int>>);
    static_assert(!GridFormat::Concepts::StaticallySizedRange<double>);

    static_assert(GridFormat::Concepts::Interoperable<int, double>);
    static_assert(GridFormat::Concepts::Interoperable<double, int>);

    static_assert(GridFormat::Concepts::Streamable<double, std::ostream>);
    static_assert(GridFormat::Concepts::Streamable<Streamable, std::ostream>);
    static_assert(!GridFormat::Concepts::Streamable<NonStreamable, std::ostream>);

    static_assert(GridFormat::Concepts::FormattedOutputStream<std::ostream, double>);
    static_assert(GridFormat::Concepts::FormattedOutputStream<std::ostream, Streamable>);
    static_assert(!GridFormat::Concepts::FormattedOutputStream<std::ostream, NonStreamable>);

    static_assert(!GridFormat::Concepts::OutputStream<std::ostream, int>);
    static_assert(GridFormat::Concepts::OutputStream<MyStream, double>);
    static_assert(GridFormat::Concepts::OutputStream<MyStream, int>);

    static_assert(GridFormat::Concepts::RangeOf<std::vector<int>, int>);
    static_assert(GridFormat::Concepts::RangeOf<std::vector<double>, double>);
    static_assert(GridFormat::Concepts::RangeOf<std::vector<NonStreamable>, NonStreamable>);
    static_assert(!GridFormat::Concepts::RangeOf<std::vector<NonStreamable>, double>);
    static_assert(GridFormat::Concepts::RangeOf<std::vector<ConvertibleToDouble>, double>);

    static_assert(GridFormat::Concepts::MDRange<std::vector<int>, 1>);
    static_assert(!GridFormat::Concepts::MDRange<std::vector<int>, 2>);
    static_assert(GridFormat::Concepts::MDRange<std::vector<std::vector<int>>, 2>);
    static_assert(!GridFormat::Concepts::MDRange<std::vector<std::vector<int>>, 3>);
    static_assert(GridFormat::Concepts::MDRange<std::vector<std::vector<std::vector<int>>>, 3>);

    return 0;
}
