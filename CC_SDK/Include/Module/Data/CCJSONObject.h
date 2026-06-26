#ifndef _CC_JSONOBJECT_H
#define _CC_JSONOBJECT_H

#include "nlohmann/json.hpp"
#include "CC.h"
#include "string"

using JSON = nlohmann::json;

// 定义用于简化json序列化和反序列化的宏
#define JSON_TYPE_INTRUSIVE(Type, ...) \
friend void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t) { NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__)) } \
friend void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t) { NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM, __VA_ARGS__)) }

namespace CTL {
    // 定义一个继承自nlohmann::json的JSONObject类
    class JSONObject: public JSON{
    public:
        // 继承nlohmann::json的构造函数
        using nlohmann::json::json;
        JSONObject() = default;
        // 构造函数，接受一个JSON对象作为参数
        JSONObject(const JSON& j) : JSON(j) {}
        // 向JSON对象中添加或更新键值对
        void put(const std::string& key, const nlohmann::json &value);
        // 获取指定键的值
        JSON get(const std::string& key) const;
        String getString(const char* key) const;
        int getInt(const char* key) const;
        double getDouble(const char* key) const;
        float getFloat(const char* key) const;
        bool getBool(const char* key) const;
        // 生成描述JSON对象的字符串表示
        [[nodiscard]] std::string to_String() const;
        JSON to_JSON() const;
    private:

    };
}

#endif
