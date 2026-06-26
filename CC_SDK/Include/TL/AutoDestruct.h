#ifndef SDK_TL_AUTODESTRUCT_H
#define SDK_TL_AUTODESTRUCT_H

#include <iostream>
#include <functional>

namespace CTL{
    template<typename T>
    class AutoDestruct{
    public:
        //--------------------------------------------------------------------------------------------------------------
        AutoDestruct(T* ptr = nullptr, const bool is_array = false)
            : m_ptr(ptr)
            , m_is_owner(true)
            , is_s(is_array)
            , m_destroy_func(nullptr)
        {
        }

        AutoDestruct(AutoDestruct&& other) noexcept
            : m_ptr(other.m_ptr)
            , m_is_owner(other.m_is_owner)
            , is_s(other.is_s)
            , m_destroy_func(std::move(other.m_destroy_func))
        {
            other.m_ptr = nullptr;
            other.m_is_owner = false;
        }

        ~AutoDestruct(){
            try {
                if (m_destroy_func) {
                    m_destroy_func();
                }
            }
            catch (std::exception& e) {
                // 忽略异常
            }
            if (m_ptr && m_is_owner) {
                if (is_s) {
                    delete[] m_ptr;
                }
                else {
                    delete m_ptr;
                }
            }
            m_ptr = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------
        T* Get(){
            return m_ptr;
        }
        const T* Get() const{
            return m_ptr;
        }
        T* get(){
            return m_ptr;
        }
        const T* get() const{
            return m_ptr;
        }
        explicit operator bool() const {
            return m_ptr != nullptr;
        }
        T& operator*() const {
            return *m_ptr;
        }
        T* operator->() const {
            return m_ptr;
        }
        AutoDestruct& operator=(T* ptr){
            if (m_ptr && m_is_owner) {
                if (is_s) {
                    delete[] m_ptr;
                } else {
                    delete m_ptr;
                }
            }
            m_ptr = ptr;
            m_is_owner = true;
            return *this;
        }
        AutoDestruct& operator=(AutoDestruct&& rhs) noexcept {
            if (this != &rhs) {
                if (m_ptr && m_is_owner) {
                    if (is_s) {
                        delete[] m_ptr;
                    } else {
                        delete m_ptr;
                    }
                }
                m_ptr = rhs.m_ptr;
                m_is_owner = rhs.m_is_owner;
                is_s = rhs.is_s;
                m_destroy_func = std::move(rhs.m_destroy_func);
                rhs.m_ptr = nullptr;
                rhs.m_is_owner = false;
            }
            return *this;
        }
        void reset(T* ptr = nullptr){
            if (m_ptr && m_is_owner) {
                if (is_s) {
                    delete[] m_ptr;
                } else {
                    delete m_ptr;
                }
            }
            m_ptr = ptr;
            m_is_owner = true;
        }
        void release() {
            m_is_owner = false;
            m_ptr = nullptr;
        }
        void set_destroy_func(const std::function<void()>& func) {
            m_destroy_func = func;
        }
        //--------------------------------------------------------------------------------------------------------------
    private:
        T* m_ptr = nullptr;
        bool m_is_owner = true;
        bool is_s = false;
        std::function<void()> m_destroy_func;
    };

    template<typename T>
    class AutoDestruct<T[]>{
    public:
        AutoDestruct(T* ptr = nullptr)
            : m_ptr(ptr)
            , m_is_owner(true)
        {
        }

        AutoDestruct(AutoDestruct&& other) noexcept
            : m_ptr(other.m_ptr)
            , m_is_owner(other.m_is_owner)
        {
            other.m_ptr = nullptr;
            other.m_is_owner = false;
        }

        ~AutoDestruct(){
            if (m_ptr && m_is_owner) {
                delete[] m_ptr;
            }
            m_ptr = nullptr;
        }
        T* Get(){
            return m_ptr;
        }
        const T* Get() const{
            return m_ptr;
        }
        T* get(){
            return m_ptr;
        }
        const T* get() const{
            return m_ptr;
        }
        T& operator[](size_t index) const {
            return m_ptr[index];
        }
        explicit operator bool() const {
            return m_ptr != nullptr;
        }
        AutoDestruct& operator=(T* ptr){
            if (m_ptr && m_is_owner) {
                delete[] m_ptr;
            }
            m_ptr = ptr;
            m_is_owner = true;
            return *this;
        }
        AutoDestruct& operator=(AutoDestruct&& rhs) noexcept {
            if (this != &rhs) {
                if (m_ptr && m_is_owner) {
                    delete[] m_ptr;
                }
                m_ptr = rhs.m_ptr;
                m_is_owner = rhs.m_is_owner;
                rhs.m_ptr = nullptr;
                rhs.m_is_owner = false;
            }
            return *this;
        }
        void reset(T* ptr = nullptr){
            if (m_ptr && m_is_owner) {
                delete[] m_ptr;
            }
            m_ptr = ptr;
            m_is_owner = true;
        }
        void release() {
            m_is_owner = false;
            m_ptr = nullptr;
        }
    private:
        T* m_ptr = nullptr;
        bool m_is_owner = true;
    };
}

#endif
