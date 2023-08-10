// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <ranges>
#include <string>
#include <algorithm>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/xml/tag.hpp>
#include "../testing.hpp"

int main () {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    "xml_tag_name"_test = [] () {
        GridFormat::XMLTag tag("some_tag");
        expect(eq(tag.name(), std::string{"some_tag"}));
    };

    "xml_tag_set_attributes"_test = [] () {
        GridFormat::XMLTag tag("some_tag");

        tag.set_attribute("some_int", 42);
        tag.set_attribute("some_double", 42.0);
        tag.set_attribute("some_int_key", 42);

        expect(tag.has_attribute("some_int"));
        expect(tag.has_attribute("some_double"));
        expect(tag.has_attribute("some_int_key"));
        expect(!tag.has_attribute("non_existing"));
        expect(eq(tag.number_of_attributes(), std::size_t{3}));

        expect(eq(tag.get_attribute("some_int"), std::string{"42"}));
        expect(eq(tag.get_attribute<std::string>("some_int"), std::string{"42"}));
        expect(eq(tag.get_attribute<int>("some_int"), int{42}));
        expect(eq(tag.get_attribute<int>("some_int_key"), int{42}));
        expect(eq(tag.get_attribute<double>("some_double"), double{42.0}));
        expect(eq(tag.get_attribute<double>("some_int"), double{42.0}));
        expect(throws([&] () { tag.get_attribute<int>("some_double"); }));
    };

    "xml_tag_remove_attributes"_test = [] () {
        GridFormat::XMLTag tag("some_tag");
        tag.set_attribute("some_int", 42);
        tag.set_attribute("some_other_int", 42);
        tag.set_attribute("some_yet_other_int", 42);

        expect(tag.has_attribute("some_int"));
        expect(tag.remove_attribute("some_int"));
        expect(!tag.remove_attribute("some_int"));
        expect(!tag.has_attribute("some_int"));
        expect(eq(tag.number_of_attributes(), std::size_t{2}));

        std::vector<std::string> to_remove{{{"some_other_int"}, {"some_yet_other_int"}}};
        expect(eq(tag.remove_attributes(to_remove), std::size_t{2}));
        expect(eq(tag.number_of_attributes(), std::size_t{0}));
    };

    "xml_tag_attributes_iterator"_test = [] () {
        GridFormat::XMLTag tag("some_tag");
        tag.set_attribute("some_int", 42);
        tag.set_attribute("some_double", 42.0);

        const auto count = std::ranges::count_if(
            attributes(tag),
            [] (const auto& name) {
                return name == "some_int" || name == "some_double";
            });
        expect(eq(count, 2));
    };

    return 0;
}
