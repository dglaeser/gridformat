#include <sstream>
#include <string>
#include <ranges>

#include <boost/ut.hpp>

#include <gridformat/common/ascii_range_writer.hpp>

template<std::ranges::range R>
void check(const GridFormat::AsciiRangeWriter& writer,
           R&& input_range,
           std::string expected) {
    using boost::ut::eq;
    using boost::ut::expect;
    std::ostringstream stream;
    writer.write(input_range, stream);
    expect(eq(stream.str(), expected));
}

int main() {
    using boost::ut::operator""_test;

    "ascii_range_default_opts"_test = [] {
        check(
            GridFormat::AsciiRangeWriter{},
            std::views::iota(0, 12),
            "0 1 2 3 4 5 6 7 8 9\n10 11"
        );
    };

    "ascii_range_custom_delimiter"_test = [] {
        check(
            GridFormat::AsciiRangeWriter{{.delimiter = ","}},
            std::views::iota(0,3),
            "0,1,2"
        );
    };

    "ascii_range_custom_line_prefix"_test = [] {
        check(
            GridFormat::AsciiRangeWriter{{.delimiter = ",", .line_prefix = "PRE", .num_entries_per_line = 2}},
            std::views::iota(0, 6),
            "PRE0,1\nPRE2,3\nPRE4,5"
        );
    };

    "ascii_range_zero_entries_per_line_is_set_to_one"_test = [] {
        check(
            GridFormat::AsciiRangeWriter{{.delimiter = ",", .line_prefix = "", .num_entries_per_line = 0}},
            std::views::iota(0, 3),
            "0\n1\n2"
        );
    };

    return 0;
}