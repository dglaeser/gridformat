#include <vector>
#include <memory>
#include <ranges>

#include <boost/ut.hpp>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/field.hpp>

class MyFieldVisitor : public GridFormat::FieldVisitor {
 public:
    const std::vector<int> data() const {
        return _data;
    }

 private:
    std::vector<int> _data;

    void _take_field_values(const GridFormat::DynamicPrecision& prec,
                            const std::byte* data,
                            const std::size_t size) {
        _data.clear();
        prec.visit([&] <typename T> (const GridFormat::Precision<T>&) {
            if (!std::is_same_v<T, int>)
                throw GridFormat::InvalidState("Unexpected scalar type");
            _data.resize(size/sizeof(int));
            std::byte* _my_data = reinterpret_cast<std::byte*>(_data.data());
            std::copy_n(data, size, _my_data);
        });
    }
};

class MyField : public GridFormat::Field {
 public:
    MyField() : GridFormat::Field(
        GridFormat::MDLayout{std::vector<int>{4}},
        GridFormat::Precision<int>{}
    ) {}

 private:
    std::vector<int> _values{1, 2, 3, 4};

    void _visit(GridFormat::FieldVisitor& visitor) const override {
        visitor.take_field_values(
            this->precision(),
            reinterpret_cast<const std::byte*>(_values.data()),
            _values.size()*sizeof(int)
        );
    }
};

int main() {

    namespace bt = boost::ut;
    using namespace bt::literals;
    using bt::operator""_test;

    "field_layout"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        bt::expect(bt::eq(field->layout().dimension(), 1_ul));
        bt::expect(bt::eq(field->layout().extent(0), 4_ul));
        bt::expect(bt::eq(field->precision().is_integral(), true));
        bt::expect(bt::eq(field->precision().is_signed(), true));
        bt::expect(bt::eq(field->precision().number_of_bytes(), sizeof(int)));
    };

    "field_visitor"_test = [] () {
        MyFieldVisitor visitor;
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        field->visit(visitor);
        bt::expect(std::ranges::equal(visitor.data(), std::vector<int>{1, 2, 3, 4}));
    };

    return 0;
}