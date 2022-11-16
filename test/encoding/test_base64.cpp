#include <sstream>
#include <span>

#include <gridformat/encoding/base64.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "base64_encoded_stream"_test = [] () {
        char bytes[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        std::ostringstream s;
        GridFormat::Encoding::base64(s).write(std::span{bytes});
        expect(eq(s.str(), std::string{"AQIDBAUGBwgJ"}));
    };

    return 0;
}