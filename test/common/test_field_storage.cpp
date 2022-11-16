#include <vector>

#include <gridformat/common/field.hpp>
#include <gridformat/common/field_storage.hpp>
#include <gridformat/common/serialization.hpp>

#include "../testing.hpp"

class MyField : public GridFormat::Field {
    using ParentType = GridFormat::Field;
    static constexpr int dummy_num_components = 1;

 public:
    MyField(int id)
    : ParentType(
        GridFormat::MDLayout{std::vector<int>{1}},
        GridFormat::int32
    ), _id(id)
    {}

 private:
    int _id;

    typename GridFormat::Serialization _serialized() const override {
        typename GridFormat::Serialization result(sizeof(int));
        int* data = reinterpret_cast<int*>(result.as_span().data());
        data[0] = _id;
        return result;
    }
};

int get_id_from_serialization(const GridFormat::Field& field) {
    const auto serialization = field.serialized();
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;
    expect(eq(serialization.size(), std::size_t{sizeof(int)}));
    const int* data = reinterpret_cast<const int*>(serialization.as_span().data());
    return data[0];
}

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "field_storage_set"_test = [] () {
        GridFormat::FieldStorage storage;
        storage.set("test", MyField{1});
    };

    "field_storage_get"_test = [] () {
        GridFormat::FieldStorage storage;
        storage.set("test", MyField{42});

        const auto& field = storage.get("test");
        expect(eq(get_id_from_serialization(field), 42));
    };

    "field_storage_overwrite"_test = [] () {
        GridFormat::FieldStorage storage;
        storage.set("test", MyField{42});
        storage.set("test", MyField{45});

        const auto& field = storage.get("test");
        expect(eq(get_id_from_serialization(field), 45));
    };

    return 0;
}