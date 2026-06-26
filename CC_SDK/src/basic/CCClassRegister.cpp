#include "CCClassRegister.h"


CTL::ClassRegister::ClassRegister(const std::string &name, const std::function<Object*()>& create) {
    ClassFactory::getInstance()->register_class(name,create);
}

CTL::ClassRegister::ClassRegister(const std::string &name, const std::string &infoName,
    const std::string &type, const size_t offset) {
    const auto c = ClassFactory::getInstance();
    c->register_info(name,infoName,type,offset);
}


