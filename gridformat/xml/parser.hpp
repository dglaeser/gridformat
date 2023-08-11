// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \ingroup XML
 * \copydoc GridFormat::XMLParser
 */
#ifndef GRIDFORMAT_XML_PARSER_HPP_
#define GRIDFORMAT_XML_PARSER_HPP_

#include <cmath>
#include <string>
#include <memory>
#include <istream>
#include <fstream>
#include <type_traits>
#include <unordered_map>
#include <functional>
#include <optional>
#include <concepts>
#include <limits>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/istream_helper.hpp>
#include <gridformat/xml/element.hpp>
#include <gridformat/xml/tag.hpp>

namespace GridFormat {


/*!
 * \ingroup XML
 * \brief Parses an XML file into an XMLElement.
 * \note Discards any comments.
 * \note Creates a single root element in which the parsed elements are placed.
 * \note The XML element contents are not read. Instead, their bounds within the input stream
 *       are stored separately and the content can be retrieved via get_content(const XMLElement&).
 * \note Content inside XML elements is assumed to be either before or after child elements. If multiple
         pieces of content are intermingled with child elements, only the first piece of content will be detected.
 * \note This implementation is not a fully-fleshed XML parser, but suffices for our requirements.
 *       It is likely to fail when textual content that can be mistaken for xml is inside the elements.
 */
class XMLParser {
 public:
    using ContentSkipFunction = std::function<bool(const XMLElement&)>;

    struct StreamBounds {
        std::streamsize begin_pos;
        std::streamsize end_pos;
    };

    /*!
     * \brief Parse an xml tree from the data in the file with the given name.
     * \param filename The name of the xml file.
     * \param root_name The name of the root element in which to place the read xml (default: "ROOT")
     * \param skip_content_parsing A function that takes an xml element and returns true if the content
     *                             of that element should not be parsed for child nodes. This is useful
     *                             if the content of an element is very large and potentially invalid xml.
     */
    explicit XMLParser(const std::string& filename,
                       const std::string& root_name = "ROOT",
                       const ContentSkipFunction& skip_content_parsing = [] (const XMLElement&) { return false; })
    : _owned{std::make_unique<std::ifstream>()}
    , _helper{*_owned}
    , _element{root_name}
    , _skip_content{skip_content_parsing} {
        _owned->open(filename);
        while (_parse_next_element(_element)) {}
    }

    //! Overload for reading from an existing stream
    explicit XMLParser(std::istream& stream,
                       const std::string& root_name = "ROOT",
                       const ContentSkipFunction& skip_content_parsing = [] (const XMLElement&) { return false; })
    : _owned{}
    , _helper{stream}
    , _element{root_name}
    , _skip_content{skip_content_parsing} {
        while (_parse_next_element(_element)) {}
    }

    //! Return a reference the read xml representation
    const XMLElement& get_xml() const & {
        return _element;
    }

    //! Return the read xml representation as an rvalue
    XMLElement&& get_xml() && {
        return std::move(_element);
    }

    //! Return true if a content was read for the given xml element
    bool has_content(const XMLElement& e) const {
        return _content_bounds.contains(&e);
    }

    //! Return the stream bounds for the content of the given xml element
    const StreamBounds& get_content_bounds(const XMLElement& e) const {
        return _content_bounds.at(&e);
    }

    //! Read and return the content of the given xml element
    std::string read_content_for(const XMLElement& e, const std::optional<std::size_t> max_chars = {}) {
        const auto& bounds = _content_bounds.at(&e);
        const auto content_size = bounds.end_pos - bounds.begin_pos;
        const auto num_chars = std::min(static_cast<std::size_t>(content_size), max_chars.value_or(content_size));
        _helper.seek_position(bounds.begin_pos);
        return _helper.read_chunk(num_chars);
    }

 private:
    // parse the content or child elements from the stream and add them to the given parent element
    void _parse_content(XMLElement& parent) {
        const std::string close_tag = "</" + parent.name();
        auto content_begin_pos = _helper.position();
        auto content_end_pos = content_begin_pos;

        if (_skip_content(parent)) {
            if (!_helper.shift_until_substr(close_tag))
                throw IOError("Could not find closing tag: " + close_tag);
            content_end_pos = _helper.position();
            _helper.shift_by(close_tag.size());
        } else {
            // check for content before the first child
            if (!_helper.shift_until_any_of("<"))
                throw IOError("Could not find closing tag for '" + parent.name() + "'");
            content_end_pos = _helper.position();
            _helper.seek_position(content_begin_pos);
            _helper.shift_whitespace();
            const bool have_read_content = _helper.position() < content_end_pos;
            _helper.seek_position(content_end_pos);

            // parse all children
            std::optional<std::streamsize> position_after_last_child;
            while (true) {
                auto pos = _parse_next_element(parent, close_tag);
                if (pos) position_after_last_child = pos;
                else break;
            }

            // (maybe) check for content after the children
            if (!have_read_content && position_after_last_child.has_value()) {
                content_begin_pos = position_after_last_child.value();
                content_end_pos = _helper.position();
            }

            if (_helper.read_chunk(close_tag.size()) != close_tag)
                throw IOError("Could not find closing tag for '" + parent.name() + "'");
            if (!_helper.shift_until_any_of(">"))
                throw IOError("Could not find closing tag for '" + parent.name() + "'");
            if (!_helper.is_end_of_file())
                _helper.shift_by(1);
        }

        _content_bounds[&parent] = StreamBounds{.begin_pos = content_begin_pos, .end_pos = content_end_pos};
    }

    // parse the next child element from the stream and return the position after it (or none if no child found)
    std::optional<std::streamsize> _parse_next_element(XMLElement& parent, const std::string& close_tag = "") {
        while (true) {
            if (_helper.is_end_of_file()) {
                if (!close_tag.empty())
                    throw IOError("Did not find closing tag: " + close_tag);
                return {};
            }

            if (!_helper.shift_until_any_of("<")) {
                if (!close_tag.empty())
                    throw IOError("Did not find closing tag: " + close_tag);
                return {};
            }

            const auto cur_pos = _helper.position();
            const auto chunk = _helper.read_chunk(4);
            if (chunk.starts_with("<?"))
                continue;

            if (chunk.starts_with("<!--")) {
                _helper.seek_position(cur_pos);
                _skip_comment();
                continue;
            }

            _helper.seek_position(cur_pos);
            if (!close_tag.empty()) {
                if (_helper.read_chunk(close_tag.size()) == close_tag) {
                    _helper.seek_position(cur_pos);
                    return {};
                }
                _helper.seek_position(cur_pos);
            }

            if (_parse_element(parent))
                return {_helper.position()};
        }
    }

    // skip beyond an xml comment in the input stream
    void _skip_comment() {
        static constexpr auto comment_begin = "<!--";
        static constexpr auto comment_end = "-->";
        std::string comment_chunk;

        const auto append_until_closing_brace = [&] () -> void {
            comment_chunk += _helper.read_until_any_of(">");
            if (!_helper.is_end_of_file())
                comment_chunk += _helper.read_chunk(1);  // read the actual ">"
        };

        const auto count = [&] (const std::string& substr) {
            std::size_t count = 0;
            auto found_pos = comment_chunk.find(substr);
            while (found_pos != std::string::npos) {
                found_pos = comment_chunk.find(substr, found_pos + 1);
                count++;
            }
            return count;
        };

        append_until_closing_brace();
        if (!comment_chunk.starts_with(comment_begin))
            throw ValueError("Stream is not at a comment start position");

        while (!comment_chunk.ends_with(comment_end) || count(comment_begin) != count(comment_end))
            append_until_closing_brace();
    }

    // try to parse a single element and return true/false if succeeded
    bool _parse_element(XMLElement& parent) {
        const auto begin_pos = _helper.position();
        if (!_helper.read_chunk(1).starts_with("<")) {
            _helper.seek_position(begin_pos);
            return false;
        }

        _helper.seek_position(begin_pos);
        _helper.shift_by(1);
        std::string name = _helper.read_until_any_of(" />");
        auto& element = parent.add_child(std::move(name));

        while (true) {
            _helper.shift_until_not_any_of(" \n");
            const auto cur_pos = _helper.position();
            if (_helper.read_chunk(2) == "/>")
                break;

            _helper.seek_position(cur_pos);
            if (_helper.read_chunk(1) == ">") {
                _parse_content(element);
                break;
            }

            _helper.seek_position(cur_pos);
            auto [name, value] = _read_attribute();
            element.set_attribute(std::move(name), std::move(value));
        }

        return true;
    }

    std::pair<std::string, std::string> _read_attribute() {
        std::string attr_name = _helper.read_until_any_of("= ");
        if (attr_name.empty())
            throw IOError("Could not parse attribute name");

        _helper.shift_until_any_of("\"");
        _helper.shift_by(1);
        std::string attr_value = _helper.read_until_any_of("\"");
        _helper.shift_by(1);

        return std::make_pair(std::move(attr_name), std::move(attr_value));
    }

    std::unique_ptr<std::ifstream> _owned;
    InputStreamHelper _helper;
    XMLElement _element;
    ContentSkipFunction _skip_content;
    std::unordered_map<const XMLElement*, StreamBounds> _content_bounds;
};

}  // end namespace GridFormat

#endif  // GRIDFORMAT_XML_PARSER_HPP_
