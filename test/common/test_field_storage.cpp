#include <sstream>

#include <boost/ut.hpp>

#include <gridformat/common/field_storage.hpp>

class MyField : public GridFormat::Field {
    using ParentType = GridFormat::Field;
    static constexpr int dummy_num_components = 1;
    static constexpr auto dummy_precision = GridFormat::DynamicPrecision::int32;

 public:
    MyField(int id) : Field(dummy_num_components, dummy_precision), _id(id) {}
    int id() const { return _id; }

 private:
    int _id;

    void _stream(std::ostream& s) const override {
        s << _id;
    }

    std::vector<std::byte> _serialized() const override {
        return {};
    }
};

int main() {
    using namespace boost::ut;

    "field_storage_set"_test = [] () {
        GridFormat::FieldStorage storage;
        storage.set("test", MyField{1});
    };

    "field_storage_get"_test = [] () {
        GridFormat::FieldStorage storage;
        storage.set("test", MyField{42});

        std::stringstream stream;
        storage.get("test").stream(stream);
        expect(eq(stream.str(), std::string{"42"}));
    };

    "field_storage_overwrite"_test = [] () {
        GridFormat::FieldStorage storage;
        storage.set("test", MyField{42});
        storage.set("test", MyField{45});

        std::stringstream stream;
        storage.get("test").stream(stream);
        expect(eq(stream.str(), std::string{"45"}));
    };

    return 0;
}