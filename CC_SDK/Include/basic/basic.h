#ifndef DISTRIBUTION_SERVICE_BASIC_H
#define DISTRIBUTION_SERVICE_BASIC_H

#include "string"


namespace CTL
{
    template<typename T>
    struct is_char_ptr {
        static const bool value = false;
    };

    template<>
    struct is_char_ptr<const char*> { static const bool value = true; };

    template<>
    struct is_char_ptr<char*> { static const bool value = true; };

    template<typename T>
    struct decay_type_simple {
        typedef std::remove_cv_t<std::remove_reference_t<T>> type;
    };

    template<typename T>
    static typename std::enable_if<is_char_ptr<typename decay_type_simple<T>::type>::value, std::string>::type
    convert_to_string(T&& value) {
        return value ? value : "null";
    }

    template<typename T>
    static typename std::enable_if<std::is_same<typename decay_type_simple<T>::type, bool>::value, std::string>::type
    convert_to_string(T&& value) {
        return value ? "true" : "false";
    }

    template<typename T>
    static typename std::enable_if<std::is_integral<typename decay_type_simple<T>::type>::value &&
                                   !std::is_same<typename decay_type_simple<T>::type, bool>::value, std::string>::type
    convert_to_string(T&& value) {
        return std::to_string(static_cast<long long>(value));
    }

    template<typename T>
    static typename std::enable_if<std::is_floating_point<typename decay_type_simple<T>::type>::value, std::string>::type
    convert_to_string(T&& value) {
        return std::to_string(static_cast<long double>(value));
    }

    // 通用版本
    template<typename T>
    static typename std::enable_if<!is_char_ptr<typename decay_type_simple<T>::type>::value &&
                                   !std::is_same<typename decay_type_simple<T>::type, bool>::value &&
                                   !std::is_integral<typename decay_type_simple<T>::type>::value &&
                                   !std::is_floating_point<typename decay_type_simple<T>::type>::value, std::string>::type
    convert_to_string(T&& value) {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

    template<typename T>
    static void format_one_arg(std::string& result, T&& value) {
        size_t pos = result.find("{}");
        if (pos != std::string::npos) {
            std::string replacement = convert_to_string(std::forward<T>(value));
            result.replace(pos, 2, replacement);
        }
    }
    static std::string format_braces_template(std::string fmt_str) {
        return fmt_str;
    }
    template<typename T, typename... Rest>
    static std::string format_braces_template(std::string fmt_str, T&& first, Rest&&... rest) {
        format_one_arg(fmt_str, std::forward<T>(first));
        if (sizeof...(rest) > 0) {
            return format_braces_template(fmt_str, std::forward<Rest>(rest)...);
        }
        return fmt_str;
    }
    template<typename T>
    static std::string format_braces_template(std::string fmt_str, T&& first) {
        format_one_arg(fmt_str, std::forward<T>(first));
        return fmt_str;
    }
}

#endif