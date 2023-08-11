// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <ranges>
#include <string>
#include <sstream>
#include <algorithm>

#include <gridformat/xml/element.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    "xml_element_child_access"_test = [] () {
        GridFormat::XMLElement element("some_element");
        auto& child = element.add_child("some_child");
        expect(eq(child.name(), std::string{"some_child"}));

        auto& _child = element.get_child("some_child");
        expect(eq(child.name(), std::string{"some_child"}));
        _child.set_attribute("something", 42);
        expect(eq(child.get_attribute("something"), std::string{"42"}));
    };

    "xml_element_parent_access"_test = [] () {
        GridFormat::XMLElement element("some_element");
        auto& child = element.add_child("some_child");
        expect(eq(child.parent().name(), std::string{"some_element"}));
    };

    "xml_element_child_iterator"_test = [] () {
        GridFormat::XMLElement element("some_element");
        element.add_child("some_child");

        for (const auto& a_child : children(element))
            expect(eq(a_child.name(), std::string{"some_child"}));

        const auto& elem_ref = element;
        for (const auto& a_child : children(elem_ref))
            expect(eq(a_child.name(), std::string{"some_child"}));
    };

    "xml_element_mutable_child_iterator"_test = [] () {
        GridFormat::XMLElement element("some_element");
        element.add_child("some_child");

        const auto some_child_has_attr = [&] (const std::string& attr) {
            return std::ranges::any_of(children(element), [&] (const auto& e) {
                return e.has_attribute(attr);
            });
        };

        expect(!some_child_has_attr("some_attr"));
        std::ranges::for_each(
            children(element),
            [] (auto& e) {
                e.set_attribute("some_attr", int{42});
        });
        expect(some_child_has_attr("some_attr"));
    };

    "xml_element_remove_children"_test = [] () {
        GridFormat::XMLElement element("some_element");
        element.add_child("some_child");

        expect(eq(element.number_of_children(), std::size_t{1}));
        expect(element.has_child("some_child"));
        expect(element.remove_child("some_child"));
        expect(!element.has_child("some_child"));
        expect(eq(element.number_of_children(), std::size_t{0}));
    };

    "xml_element_set_and_stream_content"_test = [] () {
        GridFormat::XMLElement element("some_element");
        element.set_content(int{42});

        std::stringstream str_stream;
        element.stream_content(str_stream);
        expect(eq(str_stream.str(), std::string{"42"}));
    };

    "xml_element_overwrite_content"_test = [] () {
        GridFormat::XMLElement element("some_element");
        element.set_content(int{42});

        std::stringstream str_stream;
        element.stream_content(str_stream);
        expect(eq(str_stream.str(), std::string{"42"}));

        str_stream = std::stringstream{};
        element.set_content("content");
        element.stream_content(str_stream);
        expect(eq(str_stream.str(), std::string{"content"}));
    };

    "xml_element_get_content"_test = [] () {
        GridFormat::XMLElement element("some_element");
        element.set_content(int{42});

        expect(eq(element.get_content<int>(), int{42}));
        expect(throws([&](){ element.get_content<double>(); }));
    };

    "xml_element_write"_test = [] () {
        GridFormat::XMLElement element("some_element");
        element.set_attribute("attr", "value");
        auto& child = element.add_child("some_child");

        std::stringstream stream;
        GridFormat::write_xml(element, stream, GridFormat::Indentation{{.width = 0, .level = 0}});
        expect(eq(stream.str(), std::string{"<some_element attr=\"value\">\n<some_child/>\n</some_element>"}));

        std::stringstream formatted_stream;
        GridFormat::write_xml(element, formatted_stream, GridFormat::Indentation{{.width = 2, .level = 0}});
        expect(eq(formatted_stream.str(), std::string{"<some_element attr=\"value\">\n  <some_child/>\n</some_element>"}));

        child.set_content(int{42});
        std::stringstream formatted_stream_with_content;
        GridFormat::write_xml(element, formatted_stream_with_content, GridFormat::Indentation{{.width = 2, .level = 0}});
        expect(eq(formatted_stream_with_content.str(), std::string{"<some_element attr=\"value\">\n  <some_child>\n42\n  </some_child>\n</some_element>"}));
    };

    return 0;
}
