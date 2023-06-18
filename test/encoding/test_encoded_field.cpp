// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <span>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>

#include <gridformat/common/serialization.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/field.hpp>

#include <gridformat/encoding/encoded_field.hpp>
#include <gridformat/encoding/ascii.hpp>
#include <gridformat/encoding/base64.hpp>
#include <gridformat/encoding/raw.hpp>
#include "../testing.hpp"

std::vector<char> test_data{1, 2, 3, 4, 5};

class TestField : public GridFormat::Field {

    GridFormat::MDLayout _layout() const override {
        return GridFormat::MDLayout{{test_data.size()}};
    }

    GridFormat::DynamicPrecision _precision() const override {
        return {GridFormat::Precision<char>{}};
    }

    GridFormat::Serialization _serialized() const override {
        GridFormat::Serialization result(sizeof(char)*test_data.size());
        std::ranges::copy(test_data, result.as_span_of<char>().begin());
        return result;
    }
};

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "encoded_field_ascii"_test = [] () {
        const TestField field;
        std::ostringstream s;
        s << GridFormat::EncodedField{field, GridFormat::Encoding::ascii};
        expect(eq(s.str(), std::string{"12345"}));
    };

    "encoded_field_base64"_test = [] () {
        const TestField field;
        std::ostringstream s;
        s << GridFormat::EncodedField{field, GridFormat::Encoding::base64};
        expect(eq(s.str(), std::string{"AQIDBAU="}) || eq(s.str(), std::string{"AQIDBAV="}));
    };

    "encoded_field_raw"_test = [] () {
        const TestField field;
        std::ostringstream s;
        s << GridFormat::EncodedField{field, GridFormat::Encoding::raw};
        expect(std::ranges::equal(
            std::span{test_data},
            std::span{s.str().data(), s.str().size()}
        ));
    };

    return 0;
}
