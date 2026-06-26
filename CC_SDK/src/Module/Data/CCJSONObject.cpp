#include "CCJSONObject.h"

void CTL::JSONObject::put(const std::string& key, const nlohmann::json &value) {
    operator[](typename object_t::key_type(key)) = value;
}

JSON CTL::JSONObject::get(const std::string& key) const{
    if (!contains(key)) {
        return {};
    }
    try {
        return operator[](typename object_t::key_type(key));
    }
    catch (CCException &e) {
        return {}; // 返回一个空的 JSON 对象
    }
}

CTL::String CTL::JSONObject::getString(const char *key) const {
    if (!contains(key)) {
        return {};
    }
    const auto j = get(key);
    if (j.is_string()) return j.get<String>();
    if (j.is_number()) return j.dump();        // 数字 → 字符串
    if (j.is_boolean()) return j.get<bool>() ? "true" : "false";
    return "";  // 对象/数组/null 返回空串
}

int CTL::JSONObject::getInt(const char *key) const {
    if (!contains(key)) {
        return {};
    }
    const auto j = get(key);
    if (j.is_number()) return j.get<int>();
    if (j.is_string()) {
        try { return std::stoi(j.get<String>().c_str()); } catch (...) { return 0; }
    }
    if (j.is_boolean()) return j.get<bool>() ? 1 : 0;
    return 0;
}

double CTL::JSONObject::getDouble(const char *key) const {
    if (!contains(key)) {
        return {};
    }
    const auto j = get(key);
    if (j.is_number()) return j.get<double>();
    if (j.is_string()) {
        try { return std::stod(j.get<String>().c_str()); } catch (...) { return 0.0; }
    }
    if (j.is_boolean()) return j.get<bool>() ? 1.0 : 0.0;
    return 0.0;
}

float CTL::JSONObject::getFloat(const char *key) const {
    if (!contains(key)) {
        return {};
    }
    const auto j = get(key);
    if (j.is_number()) return j.get<float>();
    if (j.is_string()) {
        try { return std::stof(j.get<String>().c_str()); } catch (...) { return 0.0f; }
    }
    if (j.is_boolean()) return j.get<bool>() ? 1.0f : 0.0f;
    return 0.0f;
}

bool CTL::JSONObject::getBool(const char *key) const {
    if (!contains(key)) {
        return {};
    }
    const auto j = get(key);
    if (j.is_boolean()) return j.get<bool>();
    if (j.is_number()) return j.get<int>() != 0;
    if (j.is_string()) {
        auto s = j.get<String>();
        return s == "true" || s == "1";
    }
    return false;
}

std::string CTL::JSONObject::to_String() const {
    return dump();
}

JSON CTL::JSONObject::to_JSON() const {
    return *this;
}


