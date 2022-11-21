#include <vector>
#include <memory>

#include <gridformat/common/transformed_fields.hpp>

#include "../testing.hpp"

class MyField : public GridFormat::Field {
 private:
    std::vector<int> _values{1, 2, 3, 4};

    GridFormat::MDLayout _layout() const override {
        return GridFormat::MDLayout{std::vector<std::size_t>{_values.size()}};
    }

    GridFormat::DynamicPrecision _precision() const override {
        return GridFormat::Precision<int>{};
    }

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

    using GridFormat::TransformedField;
    using GridFormat::FieldTransformation::identity;

    "transformed_field_identity"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        TransformedField transformed_field{*field, identity};
        expect(eq(transformed_field.layout().dimension(), 1_ul));
        expect(eq(transformed_field.layout().extent(0), 4_ul));
        expect(eq(transformed_field.precision().is_integral(), true));
        expect(eq(transformed_field.precision().is_signed(), true));
        expect(eq(transformed_field.precision().size_in_bytes(), sizeof(int)));
    };

    "transformed_field_identity_identity"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        TransformedField transformed_field{TransformedField{*field, identity}, identity};
        expect(eq(transformed_field.layout().dimension(), 1_ul));
        expect(eq(transformed_field.layout().extent(0), 4_ul));
        expect(eq(transformed_field.precision().is_integral(), true));
        expect(eq(transformed_field.precision().is_signed(), true));
        expect(eq(transformed_field.precision().size_in_bytes(), sizeof(int)));
    };

    return 0;
}