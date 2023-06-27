// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

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

    static_assert(!GridFormat::Concepts::StaticallySizedMDRange<double>);
    static_assert(GridFormat::Concepts::StaticallySizedMDRange<std::array<int, 2>>);
    static_assert(GridFormat::Concepts::StaticallySizedMDRange<std::array<std::array<int, 2>, 2>>);

    static_assert(GridFormat::Concepts::StaticallySizedMDRange<
        std::array<std::array<int, 2>, 2>,
        2>
    );
    static_assert(!GridFormat::Concepts::StaticallySizedMDRange<
        std::array<std::array<int, 2>, 2>,
        1>
    );
    static_assert(!GridFormat::Concepts::StaticallySizedMDRange<
        std::array<std::array<int, 2>, 2>,
        3>
    );
    static_assert(GridFormat::Concepts::StaticallySizedMDRange<
        std::array<std::array<std::array<int, 2>, 2>, 2>,
        3>
    );
    static_assert(!GridFormat::Concepts::StaticallySizedMDRange<
        std::array<std::array<std::array<int, 2>, 2>, 2>,
        2>
    );
    static_assert(!GridFormat::Concepts::StaticallySizedMDRange<
        std::array<std::vector<std::array<int, 2>>, 2>,
        3>
    );
    static_assert(!GridFormat::Concepts::StaticallySizedMDRange<
        std::array<std::array<std::vector<int>, 2>, 2>,
        3>
    );

    static_assert(GridFormat::Concepts::ResizableMDRange<std::vector<int>>);
    static_assert(GridFormat::Concepts::ResizableMDRange<std::vector<std::vector<double>>>);
    static_assert(GridFormat::Concepts::ResizableMDRange<std::vector<std::array<double, 2>>>);
    static_assert(!GridFormat::Concepts::ResizableMDRange<std::array<double, 2>>);

    static_assert(GridFormat::Concepts::Interoperable<int, double>);
    static_assert(GridFormat::Concepts::Interoperable<double, int>);

    static_assert(GridFormat::Concepts::StreamableWith<double, std::ostream>);
    static_assert(GridFormat::Concepts::StreamableWith<Streamable, std::ostream>);
    static_assert(!GridFormat::Concepts::StreamableWith<NonStreamable, std::ostream>);

    static_assert(!GridFormat::Concepts::WriterFor<std::ostream, std::span<const int>>);
    static_assert(GridFormat::Concepts::WriterFor<MyStream, std::span<const double>>);
    static_assert(GridFormat::Concepts::WriterFor<MyStream, std::span<const int>>);

    static_assert(!GridFormat::Concepts::WritableWith<std::span<const int>, std::ostream>);
    static_assert(GridFormat::Concepts::WritableWith<std::span<const double>, MyStream>);
    static_assert(GridFormat::Concepts::WritableWith<std::span<const int>, MyStream>);

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
