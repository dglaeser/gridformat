#include <string>

#include <boost/ut.hpp>

#include <gridformat/common/indentation.hpp>

int main() {
    using namespace boost::ut;

    "indentation"_test = [] () {
        for (unsigned width : {0, 1, 2}) {
            for (unsigned level : {0, 1}) {
                std::string initial_indent = std::string(level*width, ' ');
                std::string delta_indent = std::string(width, ' ');
                GridFormat::Indentation ind({.width = width, .level = level});
                expect(eq(ind.get(), initial_indent));
                expect(eq(ind++.get(), initial_indent));
                expect(eq(ind.get(), initial_indent + delta_indent));
                expect(eq((++ind).get(), initial_indent + delta_indent + delta_indent));
                expect(eq(ind--.get(), initial_indent + delta_indent + delta_indent));
                expect(eq(ind.get(), initial_indent + delta_indent));
                expect(eq((--ind).get(), initial_indent));

                std::ostringstream stream;
                stream << ind;
                expect(eq(stream.str(), initial_indent));
            }
        }
    };

    return 0;
}