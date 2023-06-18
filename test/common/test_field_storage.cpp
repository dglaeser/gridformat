// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>

#include <gridformat/common/field.hpp>
#include <gridformat/common/field_storage.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/exceptions.hpp>

#include "../testing.hpp"

class MyField : public GridFormat::Field {
 public:
    MyField(int id) : _id(id) {}

 private:
    int _id;

    GridFormat::MDLayout _layout() const override {
        return GridFormat::MDLayout{{1}};
    }

    GridFormat::DynamicPrecision _precision() const override {
        return {GridFormat::Precision<int>{}};
    }

    typename GridFormat::Serialization _serialized() const override {
        typename GridFormat::Serialization result(sizeof(int));
        auto ints = result.template as_span_of<int>();
        ints[0] = _id;
        return result;
    }
};

int get_id_from_serialization(const GridFormat::Field& field) {
    const auto serialization = field.serialized();
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;
    expect(eq(serialization.size(), std::size_t{sizeof(int)}));
    return serialization.template as_span_of<int>()[0];
}

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
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

    "field_storage_invalid_access"_test = [] () {
        GridFormat::FieldStorage storage;
        storage.set("test", MyField{42});
        expect(throws([&] () { storage.get("fail"); }));
    };

    "field_storage_field_removal"_test = [] () {
        GridFormat::FieldStorage storage;
        storage.set("test", MyField{42});
        storage.get("test");
        GridFormat::FieldPtr ptr = storage.pop("test");
        expect(throws([&] () { storage.get("test"); }));
    };

    return 0;
}
