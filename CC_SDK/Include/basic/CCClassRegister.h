#ifndef REFLECTION_CC_CLASS_REGISTER_H
#define REFLECTION_CC_CLASS_REGISTER_H

#include "CCClassFactory.h"
#include "CCObject.h"

namespace CTL {
    class ClassRegister {
    public:
        ClassRegister(const std::string &name, const std::function<Object*()>& create);
        ClassRegister(const std::string& name, const std::string& infoName, const std::string& type,size_t offset);
    };

#define CC_Register_CLASS_t(className)        \
static CTL::Object * create_##className()                 \
{                                           \
CTL::Object* obj = new className();            \
obj->set_class_name(#className); \
return obj;                             \
}

#define CC_Register_CLASS(className)        \
    CC_Register_CLASS_t(className)                           \
    inline CTL::ClassRegister classRegister##className(#className, create_##className);

#define CC_Register_INFO_t(className,infoName,infoType) \
    inline className className##infoName; \
    inline CTL::ClassRegister classRegister##className##infoName(#className,#infoName,#infoType,\
    (size_t)(&(className##infoName.infoName)) - (size_t)(&className##infoName));

#define CC_Register_INFO(className,infoName) \
    CC_Register_INFO_t(className, infoName, decltype(className::infoName))

}

#endif