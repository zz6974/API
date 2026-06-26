#ifndef SRCCTL_JSON_OBJECT_H
#define SRCCTL_JSON_OBJECT_H

#include <string>
#include <ostream>
#include <list>
#include <map>
#include "vector"



namespace CTL{
    class JsonObject{
    public:
        enum Type{
            json_null = 0,  // null value
            json_bool,      // bool value
            json_int,       // integer value
            json_double,    // double value
            json_string,    // string value
            json_array,     // array value
            json_object     // object value
        };
        JsonObject();
        JsonObject(Type type);
        JsonObject(bool value);
        JsonObject(int value);
        JsonObject(double value);
        JsonObject(const char * value);
        JsonObject(const std::string & value);
        JsonObject(const JsonObject & other);
        template<typename T>
        JsonObject(const T & other);
        template<typename T>
        JsonObject & operator = (const std::vector<T> & vec);
        template<typename K, typename V>
        JsonObject & operator = (const std::map<K,V> & map);
        bool operator == (const JsonObject & other) const;
        ~JsonObject();
        Type type() const;
        bool isNull() const;
        bool isBool() const;
        bool isInt() const;
        bool isDouble() const;
        bool isString() const;
        bool isArray() const;
        bool isObject() const;
        bool asBool() const;
        int asInt() const;
        double asDouble() const;
        std::string asString() const;
        int size() const;
        bool empty() const;
        void clear();
        bool has(int index) const;
        bool has(const char * key) const;
        bool has(const std::string & key) const;
        JsonObject get(int index) const;
        JsonObject get(const char * key) const;
        JsonObject get(const std::string & key) const;
        void remove(int index) const;
        void remove(const char * key) const;
        void remove(const std::string & key) const;
        void append(const JsonObject & value);
        JsonObject & operator = (const JsonObject & other);
        bool operator != (const JsonObject & other) const;
        JsonObject & operator [] (int index) const;
        JsonObject & operator [] (const char * key);
        JsonObject & operator [] (const std::string & key);
        friend std::ostream & operator << (std::ostream & os, const JsonObject & json);
        operator bool() const;
        operator int() const;
        operator double() const;
        operator std::string() const;
        static JsonObject parse(const std::string & str);
        std::string toString() const;
        typedef std::list<JsonObject>::iterator iterator;
        iterator begin() const;
        iterator end() const;
    private:
        void copy(const JsonObject& other);
    public:
    private:
        union Value{
            bool m_bool;
            int m_int;
            double m_double;
            std::string * m_string;
            std::list<JsonObject> * m_array;
            std::map<std::string, JsonObject> * m_object;
        };
        Type m_type = {};
        Value m_value = {};
    };

    template <typename T>
    JsonObject::JsonObject(const T& other){
        *this = other.to_json();
    }

    template <typename T>
    JsonObject& JsonObject::operator=(const std::vector<T>& vec){
        clear();
        m_type = json_array;
        m_value.m_array = new std::list<JsonObject>();
        for (const auto& item : vec) {
            m_value.m_array->push_back(JsonObject(item));
        }
        return *this;
    }

    template <typename K, typename V>
    JsonObject& JsonObject::operator=(const std::map<K, V>& map){
        clear();
        m_type = json_object;
        m_value.m_object = new std::map<std::string, JsonObject>();
        for (const auto& pair : map) {
            m_value.m_object->insert(std::pair<std::string, JsonObject>(std::to_string(pair.first), pair.second.to_json()));
        }
        return *this;
    }
}

#endif