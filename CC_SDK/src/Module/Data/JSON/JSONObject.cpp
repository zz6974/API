#include "JSON/JSONObject.h"
#include "JSON/Parser.h"
#include <sstream>

CTL::JsonObject::JsonObject(){
    m_type = json_null;
    m_value = {};
}

CTL::JsonObject::JsonObject(const Type type){
    m_type = type;
    switch (m_type){
        case json_null:{
            break;
        }
        case json_bool:{
            m_value.m_bool = false;
            break;
        }
        case json_int:{
            m_value.m_int = 0;
            break;
        }
        case json_double:{
            m_value.m_double = 0.0;
            break;
        }
        case json_string:{
            m_value.m_string = new std::string("");
            break;
        }
        case json_array:{
            m_value.m_array = new std::list<JsonObject>();
            break;
        }
        case json_object:{
            m_value.m_object = new std::map<std::string, JsonObject>();
        }
        default:{
            break;
        }
    }
}

CTL::JsonObject::JsonObject(const bool value){
    m_type = json_bool;
    m_value.m_bool = value;
}

CTL::JsonObject::JsonObject(const int value){
    m_type = json_int;
    m_value.m_int = value;
}

CTL::JsonObject::JsonObject(const double value){
    m_type = json_double;
    m_value.m_double = value;
}

CTL::JsonObject::JsonObject(const char* value){
    m_type = json_string;
    m_value.m_string = new std::string(value);
}

CTL::JsonObject::JsonObject(const std::string& value){
    m_type = json_string;
    m_value.m_string = new std::string(value);
}

CTL::JsonObject::JsonObject(const JsonObject& other){
    copy(other);
}

CTL::JsonObject::~JsonObject(){

}

CTL::JsonObject::Type CTL::JsonObject::type() const
{
    return m_type;
}

bool CTL::JsonObject::isNull() const
{
    return m_type == json_null;
}

bool CTL::JsonObject::isBool() const
{
    return m_type == json_bool;
}

bool CTL::JsonObject::isInt() const
{
    return m_type == json_int;
}

bool CTL::JsonObject::isDouble() const
{
    return m_type == json_double;
}

bool CTL::JsonObject::isString() const
{
    return m_type == json_string;
}

bool CTL::JsonObject::isArray() const
{
    return m_type == json_array;
}

bool CTL::JsonObject::isObject() const
{
    return m_type == json_object;
}

bool CTL::JsonObject::asBool() const
{
    if (m_type == json_bool)
    {
        return m_value.m_bool;
    }
    throw std::logic_error("function Json::asBool value type error");
}

int CTL::JsonObject::asInt() const
{
    if (m_type == json_int)
    {
        return m_value.m_int;
    }
    throw std::logic_error("function Json::asInt value type error");
}

double CTL::JsonObject::asDouble() const
{
    if (m_type == json_double)
    {
        return m_value.m_double;
    }
    throw std::logic_error("function Json::asDouble value type error");
}

std::string CTL::JsonObject::asString() const
{
    if (m_type == json_string)
    {
        return *(m_value.m_string);
    }
    throw std::logic_error("function Json::asString value type error");
}

CTL::JsonObject::iterator CTL::JsonObject::begin() const{
    return (m_value.m_array)->begin();
}

CTL::JsonObject::iterator CTL::JsonObject::end() const{
    return (m_value.m_array)->end();
}

void CTL::JsonObject::copy(const JsonObject& other){
    m_type = other.m_type;
    m_value = other.m_value;
}

std::ostream& CTL::operator<<(std::ostream& os, const JsonObject& json){
    os << json.toString();
    return os;
}

int CTL::JsonObject::size() const
{
    switch (m_type)
    {
        case json_array:
            return (m_value.m_array)->size();
        case json_object:
            return (m_value.m_object)->size();
        default:
            break;
    }
    throw std::logic_error("function Json::size value type error");
}

bool CTL::JsonObject::empty() const
{
    switch (m_type)
    {
        case json_null:
            return true;
        case json_array:
            return (m_value.m_array)->empty();
        case json_object:
            return (m_value.m_object)->empty();
        default:
            break;
    }
    return false;
}

void CTL::JsonObject::clear()
{
    switch (m_type)
    {
        case json_null:
        case json_bool:
        case json_int:
        case json_double:
            break;
        case json_string:
            {
                delete m_value.m_string;
            }
            break;
        case json_array:
            {
                for (auto it = (m_value.m_array)->begin(); it != (m_value.m_array)->end(); it++)
                {
                    it->clear();
                }
                delete m_value.m_array;
            }
            break;
        case json_object:
            {
                for (auto it = (m_value.m_object)->begin(); it != (m_value.m_object)->end(); it++)
                {
                    it->second.clear();
                }
                delete m_value.m_object;
            }
            break;
        default:
            break;
    }
    m_type = json_null;
}

bool CTL::JsonObject::has(const int index) const
{
    if (m_type != json_array)
    {
        return false;
    }
    int size = (m_value.m_array)->size();
    return (index >= 0) && (index < size);
}

bool CTL::JsonObject::has(const char * key) const
{
    std::string name(key);
    return has(name);
}

bool CTL::JsonObject::has(const std::string & key) const
{
    if (m_type != json_object)
    {
        return false;
    }
    return (m_value.m_object)->find(key) != (m_value.m_object)->end();
}

CTL::JsonObject CTL::JsonObject::get(const int index) const
{
    if (!has(index))
    {
        return CTL::JsonObject();
    }
    auto it = (m_value.m_array)->begin();
    for (int i = 0; i < index; i++)
    {
        it++;
    }
    return *it;
}

CTL::JsonObject CTL::JsonObject::get(const char * key) const
{
    if (!has(key))
    {
        return CTL::JsonObject();
    }
    return (*(m_value.m_object))[key];
}

CTL::JsonObject CTL::JsonObject::get(const std::string & key) const
{
    if (!has(key))
    {
        return CTL::JsonObject();
    }
    return (*(m_value.m_object))[key];
}

void CTL::JsonObject::remove(const int index) const{
    if (m_type != json_array)
    {
        return;
    }
    int size = (m_value.m_array)->size();
    if (index < 0 || index >= size)
    {
        return;
    }
    auto it = (m_value.m_array)->begin();
    for (int i = 0; i < index; i++)
    {
        it++;
    }
    it->clear();
    (m_value.m_array)->erase(it);
}

void CTL::JsonObject::remove(const char * key) const{
    std::string name = key;
    remove(name);
}

void CTL::JsonObject::remove(const std::string & key) const{
    if (m_type != json_object)
    {
        return;
    }
    auto it = (m_value.m_object)->find(key);
    if (it != (m_value.m_object)->end())
    {
        it->second.clear();
        (m_value.m_object)->erase(key);
    }
}

void CTL::JsonObject::append(const CTL::JsonObject & value)
{
    if (m_type != json_array)
    {
        clear();
        m_type = json_array;
        m_value.m_array = new std::list<CTL::JsonObject>();
    }
    (m_value.m_array)->push_back(value);
}

CTL::JsonObject & CTL::JsonObject::operator = (const CTL::JsonObject & other)
{
    clear();
    copy(other);
    return *this;
}

bool CTL::JsonObject::operator == (const CTL::JsonObject & other) const{
    if (m_type != other.type())
    {
        return false;
    }
    switch (m_type)
    {
        case json_null:
            return true;
        case json_bool:
            return (m_value.m_bool == other.m_value.m_bool);
        case json_int:
            return (m_value.m_int == other.m_value.m_int);
        case json_double:
            return (m_value.m_double == other.m_value.m_double);
        case json_string:
            return *(m_value.m_string) == *(other.m_value.m_string);
        case json_array:
            return m_value.m_array == other.m_value.m_array;
        case json_object:
            return m_value.m_object == other.m_value.m_object;
        default:
            break;
    }
    return false;
}

bool CTL::JsonObject::operator != (const CTL::JsonObject & other) const{
    return !(*this == other);
}

CTL::JsonObject & CTL::JsonObject::operator [] (const int index) const{
    if (index < 0)
    {
        throw std::logic_error("function [] index less than 0");
    }
    if (m_type != json_array)
    {
        throw std::logic_error("function [] not an array");
    }
    int size = (m_value.m_array)->size();
    if (index >= size)
    {
        throw std::logic_error("function [] out of range");
    }
    auto it = (m_value.m_array)->begin();
    for (int i = 0; i < index; i++)
    {
        it++;
    }
    return *it;
}

CTL::JsonObject & CTL::JsonObject::operator [] (const char * key)
{
    std::string name = key;
    return (*this)[name];
}

CTL::JsonObject & CTL::JsonObject::operator [] (const std::string & key)
{
    if (m_type != json_object)
    {
        clear();
        m_type = json_object;
        m_value.m_object = new std::map<std::string, CTL::JsonObject>();
    }
    return (*(m_value.m_object))[key];
}

CTL::JsonObject::operator bool() const{
    if (m_type != json_bool)
    {
        throw std::logic_error("function Json::operator (bool) requires bool value");
    }
    return m_value.m_bool;
}

CTL::JsonObject::operator int() const{
    if (m_type != json_int)
    {
        throw std::logic_error("function Json::operator (int) requires int value");
    }
    return m_value.m_int;
}

CTL::JsonObject::operator double() const{
    if (m_type != json_double)
    {
        throw std::logic_error("function Json::operator (double) requires double value");
    }
    return m_value.m_double;
}

CTL::JsonObject::operator std::string() const{
    if (m_type != json_string)
    {
        throw std::logic_error("function Json::operator (string) requires string value");
    }
    return *(m_value.m_string);
}

CTL::JsonObject CTL::JsonObject::parse(const std::string& str){
    Parser parser;
    parser.load(str);
    return parser.parse();
}

std::string CTL::JsonObject::toString() const{
    std::stringstream ss;
    switch (m_type)
    {
    case json_null:
        ss << "null";
        break;
    case json_bool:
        if (m_value.m_bool)
        {
            ss << "true";
        }
        else
        {
            ss << "false";
        }
        break;
    case json_int:
        ss << m_value.m_int;
        break;
    case json_double:
        ss << m_value.m_double;
        break;
    case json_string:
        ss << "\"" << *(m_value.m_string) << "\"";
        break;
    case json_array:
    {
        ss << "[";
        for (auto it = (m_value.m_array)->begin(); it != (m_value.m_array)->end(); it++)
        {
            if (it != (m_value.m_array)->begin())
            {
                ss << ",";
            }
            ss << (*it).toString();
        }
        ss << "]";
    }
        break;
    case json_object:
    {
        ss << "{";
        for (auto it = (m_value.m_object)->begin(); it != (m_value.m_object)->end(); it++)
        {
            if (it != (m_value.m_object)->begin())
            {
                ss << ",";
            }
            ss << "\"" << it->first << "\":" << it->second.toString();
        }
        ss << "}";
    }
        break;
    default:
        break;
    }
    return ss.str();
}


