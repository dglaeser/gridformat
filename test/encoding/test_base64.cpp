#include <iostream>

#include <gridformat/encoding/base64.hpp>

int main() {
    char bytes[5] = {1, 2, 3, 4, 5};
    GridFormat::Base64Stream{std::cout}.write(bytes, 5);
    std::cout << "\n";
    return 0;
}