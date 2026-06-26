#ifndef CC_Web_CORS_H
#define CC_Web_CORS_H

#include <vector>
#include <map>
#include "CCString.h"
#include "CC.h"

#define CC_CORS_Method "GET,POST,PUT,DELETE,OPTIONS"
#define CC_CORS_Header "Content-Type,Authorization,X-Requested-With"

/**
 * 命名空间CTL，用于跨语言项目的公共类型和工具类
 */
namespace CTL {
    /**
     * CORS类用于管理跨域资源共享(CORS)策略
     * 它允许你添加允许的源、头以及设置方法和头
     * 还可以获取当前的允许源列表和所有设置的头
     * 并且可以设置是否限制IP
     */
    class CORS
    {
    public:
        CORS() = default;

        /**
         * 构造函数，初始化CORS对象
         * @param cors 另一个CORS对象
         */
        CORS(const CORS &cors) {
            this->Init(cors);
        }

        /**
         * 初始化CORS对象
         * @param cors 另一个CORS对象
        */
        void Init(const CORS &cors) {
            this->CORSA = cors.CORSA;
            this->AllowOrigin = cors.AllowOrigin;
            this->IPRestrictions = cors.IPRestrictions;
        }

        /**
         * 添加一个允许的跨域源
         * @param Origin 允许的源，例如域名
         */
        void AddAllowOrigin(const CTL::String& Origin);

        /**
         * 添加一个HTTP头到CORS策略中
         * @param Key 头的名称
         * @param Value 头的值
         */
        void AddHeader(const CTL::String& Key,const CTL::String& Value);

        /**
         * 设置允许的方法，如GET, POST等
         * @param Value 允许的方法，用逗号分隔
         */
        void SetMethods(const CTL::String& Value);

        /**
         * 设置允许的头
         * @param Value 允许的头，用逗号分隔
         */
        void SetHeaders(const CTL::String& Value);

        /**
         * 获取所有允许的源
         * @return 允许的源列表
         */
        std::vector<CTL::String> GetAllowOrigin();

        /**
         * 获取所有设置的CORS头
         * @return 包含所有CORS头的map
         */
        std::map<CTL::String,CTL::String> GetHeader();

        /**
         * 检查是否设置了IP限制
         * @return 如果限制IP返回true，否则返回false
         */
        bool RestrictIP() const;

        /**
         * 设置是否限制IP
         * @param F 一个布尔值，true表示限制IP，false表示不限制
         */
        void SetIPRestrictions(bool F);

    private:
        std::map<CTL::String,CTL::String> CORSA;
        std::vector<CTL::String> AllowOrigin;
        bool IPRestrictions = false;
    };
}

// 添加一个允许的源到AllowOrigin列表
inline void CTL::CORS::AddAllowOrigin(const CTL::String& Origin) {
    AllowOrigin.push_back(Origin);
}

// 向CORSA map中插入一个键值对
inline void CTL::CORS::AddHeader(const CTL::String &Key, const CTL::String &Value) {
    CORSA.insert(std::pair<CTL::String,CTL::String>(Key,Value));
}

// 设置允许的方法，插入到CORSA map中
inline void CTL::CORS::SetMethods(const CTL::String &Value) {
    CORSA.insert(std::pair<CTL::String,CTL::String>("Access-Control-Allow-Methods",Value));
}

// 设置允许的头，插入到CORSA map中
inline void CTL::CORS::SetHeaders(const CTL::String &Value) {
    CORSA.insert(std::pair<CTL::String,CTL::String>("Access-Control-Allow-Headers",Value));
}

// 返回AllowOrigin列表
inline std::vector<CTL::String> CTL::CORS::GetAllowOrigin() {
    return AllowOrigin;
}

// 构建并返回包含CORS头的map
inline std::map<CTL::String,CTL::String> CTL::CORS::GetHeader() {
    std::map<String,String> CORS = CORSA;
    String AllowOrigin_Str = "Access-Control-Allow-Origin";
    String AllowOrigin_Value = "*";
    for (CCVar IP : AllowOrigin) {
        if(AllowOrigin_Value.empty()) {
            AllowOrigin_Value.append(IP);
        }
        else {
            AllowOrigin_Value.append(",").append(IP);
        }
    }
    if (CORSA.find(AllowOrigin_Str) == CORSA.end()) {
        const auto a = std::make_pair(AllowOrigin_Str,AllowOrigin_Value);
        CORS.insert(a);
    }
    return CORS;
}

// 返回IP限制状态
inline bool CTL::CORS::RestrictIP() const {
    return IPRestrictions;
}

// 设置IP限制状态
inline void CTL::CORS::SetIPRestrictions(const bool F) {
    AllowOrigin.clear();
    AllowOrigin.emplace_back("*");
    IPRestrictions = F;
}


#endif
