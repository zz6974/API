#ifndef REFLECTION_CC_CLASS_FACTORY_INFO_H
#define REFLECTION_CC_CLASS_FACTORY_INFO_H

namespace CTL {
    class ClassFactoryInfo {
        std::string Name;
        std::string typeName;
        size_t offset = 0;
    public:
        //-------------------------------------------------------------------------------------------
        // 默认构造函数
        ClassFactoryInfo() = default;
        // 基本构造函数
        explicit ClassFactoryInfo(const std::string& name) {
            Name = name;
        }
        explicit ClassFactoryInfo(const std::string& name, const std::string& typeName, const size_t offset) {
            Name = name;
            this->typeName = typeName;
            this->offset = offset;
        }
        ~ClassFactoryInfo() = default;
        //-------------------------------------------------------------------------------------------
        [[nodiscard]] std::string getName() const {
            return Name;
        }
        [[nodiscard]] std::string getTypeName() const {
            return typeName;
        }
        [[nodiscard]] size_t getOffset() const {
            return offset;
        }
        //-------------------------------------------------------------------------------------------
    private:

        //-------------------------------------------------------------------------------------------
    };
}

#endif