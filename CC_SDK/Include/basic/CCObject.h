#ifndef CC_OBJECT_H
#define CC_OBJECT_H
#pragma once

#include <functional>
#include <string>
#include <memory>
#include "mutex"
#include "CCClassFactoryInfo.h"

namespace CTL {
    class Object {
        static std::mutex Mutex_Object;
        std::string MemoryID;
        std::string m_class_Name;
    public:

    private:

    public:
        //-------------------------------------------------------------------------------------
        Object() = default;
        virtual ~Object() = default;
        //-------------------------------------------------------------------------------------
        // 比较两个对象是否逻辑相等（默认实现为地址比较）
        virtual bool equals(const Object* other);
        // 生成基于对象地址的哈希值
        [[nodiscard]] virtual size_t hashCode() const noexcept;
        // 返回对象的字符串表示（默认格式：类名@地址）
        [[nodiscard]] virtual std::string toString() const;
        void set_class_name(const std::string& name);
        std::string get_class_name() const;
        //-------------------------------------------------------------------------------------
        int get_info_count() const;
        ClassFactoryInfo* get_info(int pos) const;
        ClassFactoryInfo* get_info(const std::string& infoName) const;
        template<typename T>
        T get(const std::string& infoName);
        template<typename T>
        void set(const std::string& infoName, T value);
        //-------------------------------------------------------------------------------------
        friend std::ostream& operator<<(std::ostream& os, const Object& map);
        template<typename T = Object, typename... Args>
        static std::shared_ptr<T> create(Args&&... args) {
            return std::make_shared<T>(std::forward<Args>(args)...);
        }
        //-------------------------------------------------------------------------------------
    };
}

#endif