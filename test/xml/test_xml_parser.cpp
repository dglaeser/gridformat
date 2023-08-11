// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <iostream>
#include <sstream>

#include <gridformat/xml/parser.hpp>
#include "../testing.hpp"

void print_content(const GridFormat::XMLElement& e, GridFormat::XMLParser& parser) {
    if (parser.has_content(e))
        std::cout << "Content for " << e.name() << " ='" << parser.read_content_for(e) << "'" << std::endl;
    for (const auto& child : children(e))
        print_content(child, parser);
}

int main() {
    std::stringstream s;
    s << "<!--comment1-->\n";
    s << "<?xml version=\"1.0\"?>\n";
    s << "<!--comment2-->\n";
    s << "<elem1 attr1=\"attr1\"\n";
    s << "       attr2=\"attr2\">\n";
    s << "    <!--comment3 with braces <hello@hello.com> and sub braces <<what <hey>>>-->\n";
    s << "    <subchild sc_attr1=\"sc_attr1\" sc_attr2=\"sc_attr2\">\n";
    s << "      <child_with_content>\n";
    s << "        I am content\n";
    s << "      </child_with_content>\n";
    s << "      <child_with_content_and_child_after>\n";
    s << "        I am content\n";
    s << "        <child_besides_content />\n";
    s << "      </child_with_content_and_child_after>\n";
    s << "      <child_with_content_and_child_before>\n";
    s << "        <child_besides_content/>\n";
    s << "        I am content\n";
    s << "      </child_with_content_and_child_before>\n";
    s << "    </subchild>\n";
    s << "</elem1>";

    GridFormat::XMLParser parser{s};
    GridFormat::write_xml(parser.get_xml(), std::cout);

    std::cout << "Printing contents" << std::endl;
    print_content(parser.get_xml(), parser);

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;
    "xml_parser_elements"_test = [&] () {
        const auto& root = parser.get_xml();
        for (const auto& child : children(root)) {
            expect(child.name() == "elem1");
            expect(child.has_attribute("attr1"));
            expect(child.get_attribute("attr1") == "attr1");
            expect(child.has_attribute("attr2"));
            expect(child.get_attribute("attr2") == "attr2");
            expect(eq(child.number_of_children(), std::size_t{1}));
            for (const auto& sub_child : children(child)) {
                expect(sub_child.name() == "subchild");
                expect(sub_child.has_attribute("sc_attr1"));
                expect(sub_child.get_attribute("sc_attr1") == "sc_attr1");
                expect(sub_child.has_attribute("sc_attr2"));
                expect(sub_child.get_attribute("sc_attr2") == "sc_attr2");
                expect(eq(sub_child.number_of_children(), std::size_t{3}));
                for (const auto& sub_sub_child : children(sub_child)) {
                    expect(sub_sub_child.name().starts_with("child_with_content"));
                    auto content = parser.read_content_for(sub_sub_child);
                    content = content.substr(content.find_first_not_of(" \t\n"));
                    content = content.substr(0, content.find_last_not_of(" \t\n") + 1);
                    expect(eq(content, std::string{"I am content"}));
                    if (sub_sub_child.name().find("_and_child_") != std::string::npos) {
                        expect(eq(sub_sub_child.number_of_children(), std::size_t{1}));
                        expect(sub_sub_child.has_child("child_besides_content"));
                    }
                }
            }
        }
    };

    return 0;
}
