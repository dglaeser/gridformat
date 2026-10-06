// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>
#include <fstream>
#include <sstream>
#include <ostream>
#include <filesystem>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/output_file.hpp>

#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    "write_to_writes_content"_test = [] () {
        const std::string filename = "test_output_file_content.txt";
        GridFormat::write_to(filename, [] (std::ostream& s) { s << "hello"; });
        std::ifstream in{filename};
        std::ostringstream content;
        content << in.rdbuf();
        expect(eq(content.str(), std::string{"hello"}));
    };

    "write_to_throws_on_non_existing_directory"_test = [] () {
        expect(throws<GridFormat::IOError>([] () {
            GridFormat::write_to("non_existing_dir/file.txt", [] (std::ostream& s) { s << "hello"; });
        }));
    };

    "write_to_throws_on_failed_write"_test = [] () {
        if (!std::filesystem::exists("/dev/full"))
            return;
        expect(throws<GridFormat::IOError>([] () {
            GridFormat::write_to("/dev/full", [] (std::ostream& s) { s << "hello"; });
        }));
    };

    return 0;
}
