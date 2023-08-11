// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \ingroup XML
 * \copydoc GridFormat::XMLTag
 */
#ifndef GRIDFORMAT_XML_TAG_HPP_
#define GRIDFORMAT_XML_TAG_HPP_

#include <ranges>
#include <concepts>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <algorithm>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/string_conversion.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace Detail {

    template<typename Attribute>
    auto _attr_find_lambda(std::string_view name) {
        return [name] (const Attribute& attr) { return attr.first == name; };
    }

    template<typename T>
    concept RepresentableAsString = requires(const T& t) {
        { as_string(t) } -> std::convertible_to<std::string>;
    };

}  // end namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup XML
 * \brief Class to represent an XML tag consisting of a name and attributes.
 */
class XMLTag {
 public:
    using Attribute = std::pair<std::string, std::string>;

    explicit XMLTag(std::string name)
    : _name(std::move(name))
    {}

    const std::string& name() const noexcept {
        return _name;
    }

    std::size_t number_of_attributes() const noexcept {
        return _attributes.size();
    }

    bool has_attribute(std::string_view name) const {
        return _find(name) != _attributes.end();
    }

    template<Detail::RepresentableAsString ValueType>
    void set_attribute(std::string attr_name, ValueType&& value) {
        if (auto it = _find(attr_name); it != _attributes.end())
            *it = Attribute{std::move(attr_name), as_string(value)};
        else
            _attributes.emplace_back(Attribute{std::move(attr_name), as_string(value)});
    }

    bool remove_attribute(std::string_view attr_name) {
        return std::erase_if(_attributes, Detail::_attr_find_lambda<Attribute>(attr_name));
    }

    template<std::ranges::input_range R>
        requires(std::convertible_to<std::ranges::range_value_t<R>, std::string_view>)
    std::size_t remove_attributes(R&& attrs_to_remove) {
        return std::erase_if(_attributes,
            [&] (const Attribute& attr) {
                return std::ranges::any_of(attrs_to_remove,
                        [&] (std::string_view attr_to_remove) {
                            return attr_to_remove == attr.first;
                });
        });
    }

    template<typename T = std::string>
    T get_attribute(std::string_view attr_name) const {
        if (auto result = _get_attribute<T>(attr_name); result)
            return *std::move(result);
        throw InvalidState("No attribute with name '" + std::string(attr_name) + "'");
    }

    template<typename T = std::string>
    T get_attribute_or(T fallback, std::string_view attr_name) const {
       return _get_attribute<T>(attr_name).value_or(fallback);
    }

     friend decltype(auto) attributes(const XMLTag& tag) {
        return std::views::keys(tag._attributes);
    }

 private:
    template<typename T>
    std::optional<T> _get_attribute(std::string_view attr_name) const {
        if (const auto it = _find(attr_name); it != _attributes.end())
            return from_string<T>(it->second);
        return {};
    }

    typename std::vector<Attribute>::const_iterator _find(std::string_view n) const {
        return std::ranges::find_if(_attributes, Detail::_attr_find_lambda<Attribute>(n));
    }
    typename std::vector<Attribute>::iterator _find(std::string_view n) {
        return std::ranges::find_if(_attributes, Detail::_attr_find_lambda<Attribute>(n));
    }

    std::string _name;
    std::vector<Attribute> _attributes;
};

}  // end namespace GridFormat

#endif  // GRIDFORMAT_XML_TAG_HPP_
