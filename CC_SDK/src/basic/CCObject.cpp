#include "CCObject.h"
#include "iostream"
#include <chrono>
#include <random>

#include "CCClassFactory.h"

std::mutex CTL::Object::Mutex_Object;

bool CTL::Object::equals(const Object *other) {
    return this == other; // 默认比较内存地址
}

size_t CTL::Object::hashCode() const noexcept {
    return std::hash<const Object*>{}(this);
}

std::string CTL::Object::toString() const {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Object@%p", static_cast<const void*>(this));
    return buffer;
}

void CTL::Object::set_class_name(const std::string &name) {
    this->m_class_Name = name;
}

std::string CTL::Object::get_class_name() const{
    return m_class_Name;
}

int CTL::Object::get_info_count() const {
    const auto factory = ClassFactory::getInstance();
    return factory->get_info_count(this->m_class_Name);
}

CTL::ClassFactoryInfo * CTL::Object::get_info(const int pos) const {
    const auto factory = ClassFactory::getInstance();
    return factory->get_info(this->m_class_Name, pos);
}

CTL::ClassFactoryInfo * CTL::Object::get_info(const std::string &infoName) const {
    const auto factory = ClassFactory::getInstance();
    return factory->get_info(this->m_class_Name, infoName);
}

std::ostream & CTL::operator<<(std::ostream &os, const Object &map) {
    os << map.toString();
    return os;
}
