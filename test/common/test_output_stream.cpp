// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <span>
#include <string>
#include <vector>
#include <sstream>
#include <utility>
#include <algorithm>

#include <gridformat/common/output_stream.hpp>
#include "../testing.hpp"

template<typename Stream>
class OutputStreamWrapper : public GridFormat::OutputStreamWrapperBase<Stream> {
 public:
    OutputStreamWrapper(Stream& s, std::string prefix)
    : GridFormat::OutputStreamWrapperBase<Stream>(s)
    , _prefix{std::move(prefix)}
    {}

    OutputStreamWrapper& operator<<(const std::string& msg) {
        this->_write_formatted(_prefix + msg);
        return *this;
    }

 private:
    std::string _prefix;
};

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "output_stream_span_output"_test = [] () {
        std::vector<int> v{42, 43, 44};
        std::ostringstream s;
        GridFormat::OutputStream ostream{s};
        ostream.write(std::span{v});
        expect(std::ranges::equal(
            std::as_bytes(std::span{v}),
            std::as_bytes(std::span{s.str().data(), s.str().size()})
        ));

        s = {};
        ostream << int{1};
        expect(eq(s.str(), std::string{"1"}));

        s = {};
        ostream << double{1.0};
        expect(eq(s.str(), std::string{"1"}));

        s = {};
        ostream << std::string{"hello"};
        expect(eq(s.str(), std::string{"hello"}));
    };

    "output_stream_wrapper"_test = [] () {
        std::ostringstream s;
        OutputStreamWrapper wrapper(s, "pre");
        wrapper << "hello";
        expect(eq(s.str(), std::string{"prehello"}));
    };

    "output_stream_wrapped_twice"_test = [] () {
        std::ostringstream s;
        OutputStreamWrapper wrapper_1(s, "pre1");
        OutputStreamWrapper wrapper_2{wrapper_1, "pre2"};
        wrapper_2 << "hello";
        expect(eq(s.str(), std::string{"pre1pre2hello"}));
    };

    return 0;
}