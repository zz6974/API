#ifndef REFLECTION_CC_CLASS_FACTORY_H
#define REFLECTION_CC_CLASS_FACTORY_H

#include <string>
#include <map>
#include <functional>
#include "iostream"
#include "CCClassFactoryInfo.h"
#include "CCObject.h"
#include "vector"

namespace CTL {
    class ClassFactory {
        std::map<std::string, std::function<Object*()>> m_map;
        std::map<std::string, std::vector<ClassFactoryInfo*>> m_map_info;
        ClassFactory();
        ~ClassFactory();
    public:
        static ClassFactory* getInstance();
        void register_class(const std::string& name, const std::function<Object*()> &create_func);
        Object* create_class(const std::string& name);
        void register_info(const std::string& name, const std::string& infoName, const std::string& type,size_t offset);
        int get_info_count(const std::string& className);
        ClassFactoryInfo* get_info(const std::string& className, int pos);
        ClassFactoryInfo* get_info(const std::string& className, const std::string& infoName);
    };

    #define CC_CREATE_CLASS(className) CTL::ClassFactory::getInstance()->create_class(#className)

    #define CC_CREATE_CLASS_FROM_STRING(classNameStr) \
    [](const std::string& str) -> void* { \
    return CTL::ClassFactory::getInstance()->create_class(str); \
    }(classNameStr)

    template<typename T>
    T Object::get(const std::string &infoName) {
        const auto factory = ClassFactory::getInstance();
        const auto info = factory->get_info(m_class_Name, infoName);
        const size_t offset = info->getOffset();
        return *reinterpret_cast<T *>(reinterpret_cast<unsigned char *>(this) + offset);
    }
    template<typename T>
    void Object::set(const std::string& infoName, T value) {
        const auto factory = ClassFactory::getInstance();
        const auto info = factory->get_info(m_class_Name, infoName);
        const size_t offset = info->getOffset();
        *reinterpret_cast<T *>(reinterpret_cast<char *>(this) + offset) = value;
    }

}


#endif
