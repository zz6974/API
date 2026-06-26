#include "CCClassFactory.h"

CTL::ClassFactory::ClassFactory() {

}

CTL::ClassFactory::~ClassFactory() {

}

CTL::ClassFactory * CTL::ClassFactory::getInstance() {
    static ClassFactory instance;
    return &instance;
}

void CTL::ClassFactory::register_class(const std::string &name, const std::function<Object *()> &create_func) {
    m_map[name] = create_func;
}

CTL::Object * CTL::ClassFactory::create_class(const std::string &name) {
    const auto it = m_map.find(name);
    if (it != m_map.end()) {
        return it->second();
    }
    return nullptr;
}

void CTL::ClassFactory::register_info(const std::string &name, const std::string &infoName,
    const std::string &type,const size_t offset) {
    m_map_info[name].push_back(new ClassFactoryInfo(infoName, type, offset));
}

int CTL::ClassFactory::get_info_count(const std::string &className) {
    return m_map_info[className].size();
}

CTL::ClassFactoryInfo * CTL::ClassFactory::get_info(const std::string &className, const int pos) {
    const int size = m_map_info[className].size();
    if (pos < size || pos >= size) {
        return nullptr;
    }
    return m_map_info[className][pos];
}

CTL::ClassFactoryInfo * CTL::ClassFactory::get_info(const std::string &className, const std::string &infoName) {
    const auto info = m_map_info[className];
    for (auto &item : info) {
        if (item->getName() == infoName) {
            return item;
        }
    }
    return nullptr;
}
