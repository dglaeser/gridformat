#include <span>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>

#include <gridformat/common/range_field.hpp>
#include <gridformat/encoding/encoded_field.hpp>
#include <gridformat/encoding/ascii.hpp>
#include <gridformat/encoding/base64.hpp>
#include <gridformat/encoding/raw.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "encoded_field_ascii"_test = [] () {
        std::vector<int> v{1, 2, 3, 4, 5};
        const auto field = GridFormat::RangeField{v};
        std::ostringstream s;
        s << GridFormat::EncodedField{field, GridFormat::Encoding::ascii};
        expect(eq(s.str(), std::string{"12345"}));
    };

    "encoded_field_base64"_test = [] () {
        std::vector<char> v{1, 2, 3, 4, 5};
        const auto field = GridFormat::RangeField{v};
        std::ostringstream s;
        s << GridFormat::EncodedField{field, GridFormat::Encoding::base64};
        expect(eq(s.str(), std::string{"AQIDBAU="}));
    };

    "encoded_field_raw"_test = [] () {
        std::vector<char> v{1, 2, 3, 4, 5};
        const auto field = GridFormat::RangeField{v};
        std::ostringstream s;
        s << GridFormat::EncodedField{field, GridFormat::Encoding::raw};
        expect(std::ranges::equal(
            std::span{v},
            std::span{s.str().data(), s.str().size()}
        ));
    };

    return 0;
}