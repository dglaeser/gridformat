#include <vector>
#include <memory>
#include <ranges>

#include <gridformat/common/serialization.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/field.hpp>

#include "../testing.hpp"

class MyField : public GridFormat::Field {
 public:
    MyField() : GridFormat::Field(
        GridFormat::MDLayout{std::vector<int>{4}},
        GridFormat::Precision<int>{}
    ) {}

 private:
    std::vector<int> _values{1, 2, 3, 4};

    GridFormat::Serialization _serialized() const override {
        GridFormat::Serialization result(_values.size()*sizeof(int));
        std::ranges::copy(
            std::as_bytes(std::span{_values}),
            result.as_span().begin()
        );
        return result;
    }
};

int main() {

    using namespace GridFormat::Testing::Literals;
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "field_layout"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        expect(eq(field->dimension(), 1_ul));
        expect(eq(field->extent(0), 4_ul));
        expect(eq(field->precision().is_integral(), true));
        expect(eq(field->precision().is_signed(), true));
        expect(eq(field->precision().number_of_bytes(), sizeof(int)));
    };

    return 0;
}