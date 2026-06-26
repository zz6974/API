#ifndef THREADPOOLDEMO_CCSQLITE3_H
#define THREADPOOLDEMO_CCSQLITE3_H
#pragma once

#include "Sys/sqlite3.h"
#include "CCString.h"
#include "vector"
#include "mutex"
#include "CCWebServlet.h"

#ifdef MY_SQL_BOOL
#include "CCMySql.h"
#endif

struct NetSqlInfo {
    CTL::String IP = "";
    int Port = -1;
    CTL::String User = "";
    CTL::String Pass = "";
    CTL::String DBName = "";
    CTL::CORS* CORS_Header = nullptr;
};

/*
 * INTEGER: 整数类型，在两种数据库中都表示整数值
 * REAL: 浮点数类型，在两种数据库中都表示浮点数值
 * NUMERIC: 精确数值类型，在两种数据库中都支持高精度数值存储
 * TEXT: 文本字符串类型，在两种数据库中都用于存储字符数据
 * VARCHAR(n): 可变长度字符串类型，在两种数据库中都支持最大长度限制
 * DATE: 日期类型，存储年月日信息
 * TIME: 时间类型，存储时分秒信息
 * DATETIME: 日期时间类型，存储完整的日期和时间信息
 * TIMESTAMP: 时间戳类型，通常用于记录数据修改时间
 * BLOB: 二进制大对象类型，用于存储二进制数据
 * BOOLEAN: 布尔值类型，存储true/false值
 *
 * SQLite 数据库操作示例：
 * CREATE TABLE database_name.table_name(
           column1 datatype  PRIMARY KEY(one or more columns),
           column2 datatype,
           column3 datatype,
           .....
           columnN datatype,);

 *  DROP TABLE database_name.table_name;
 *  INSERT INTO TABLE_NAME VALUES (value1,value2,value3,...valueN);
 *  SELECT * FROM table_name;
 *  SELECT * FROM table_name WHERE xx='xx';
 *  UPDATE xxx SET ADDRESS = 'Texas' WHERE xxx = xxx;
 *  DELETE FROM table_name WHERE xxx=xxx;
 *
 */

namespace CTL {

    /**
     * 结构体用于存储 SQL 查询结果集
     */
    struct CCResultSet {
        /// 查询结果数据，以 JSON 格式存储
        std::vector<JSON> Data;
        /// 结果集中行数
        int Row = 0;
        /// 结果集中列数
        int Col = 0;
        /// 操作是否成功标志
        bool Flag = false;
        String Error = "";
    };
    struct CCSqlSet {
        bool Flag = false;
        String Error = "";
    };
    /**
     * SQLite3 数据库操作类
     */
    class SQLite3 {
        struct ClientInfo_tSQL {
            int ID = -1;
            String Code = "";
        };
    public:
        SQLite3() = default;
        ~SQLite3() = default;
        /**
         * 构造函数，初始化数据库路径
         *
         * @param Path 数据库文件路径
         */
        SQLite3(const CTL::String &Path);

        /**
         * 打开数据库连接
         *
         * @param Path 数据库文件路径
         * @return 是否成功打开数据库
         */
        bool Open(const CTL::String& Path);

        bool Init(sqlite3* db);

        /**
         * 打开数据库连接
         *
         * @param Host 数据库远程链接信息
         * @return 是否成功打开数据库
         */
        bool Open(const NetSqlInfo& Host);

        /**
         * 关闭数据库连接
         *
         * @return 是否成功关闭数据库
         */
        bool Close();

        /**
         * 执行更新语句（如 INSERT、UPDATE、DELETE）
         *
         * @param sql 要执行的 SQL 语句
         * @return 是否成功执行
         */
        CCSqlSet SqlUpdate(const CTL::String& sql);

        /**
         * 执行查询语句并返回结果集
         *
         * @param sql 要执行的 SQL 查询语句
         * @return 查询结果集
         */
        CCResultSet SqlQuery(const CTL::String& sql);

        /**
         * 获取最近一次操作的错误信息
         *
         * @return 错误信息字符串
         */
        CTL::String GetError();
        sqlite3* GetSqlite() const;
    private:
        static std::string getTypeName(int type);
        static bool queryWithTypes(sqlite3* db, const std::string& sql);
        sqlite3* SQL = {}; ///< SQLite 数据库连接指针
        char *zErrMsg = nullptr; ///< 错误信息指针
        bool RemoteLink = false,LinkStatus = false,Blockage = false;
        ClientInfo_tSQL ClientInfo;
        std::mutex mutex; ///< 线程同步互斥锁
        NetSqlInfo M_Host;
        CTL::Thread M_Thread;
        CTL::Timer timer;
        WebSocketClient ws;
        String zErrMsgStr = "";
        /**
         * 内部方法，执行 SQL 语句
         *
         * @param sql 要执行的 SQL 语句
         * @param callback 回调函数（可选）
         * @param a 用户自定义参数（可选）
         * @return 是否成功执行
         */
        bool SQlite(const char* sql, int(*callback)(void*, int, char**, char**) = NULL, void* a = NULL);
        /**
         * 检查字符串中是否包含小数点
         *
         * @param str 输入字符串
         * @return 是否包含小数点
         */
        static bool containsDecimal(const std::string &str);
        static bool is_int(const std::string &str);
        /**
         * 根据字符串内容转换为指定类型的数据
         *
         * @tparam T 目标数据类型
         * @param str 输入字符串
         * @return 转换后的值
         */
        template<typename T>
        T convert(const std::string &str);
        void OnOpen(CTL::WebSocketInfo& info);
        void OnMessage(CTL::WebSocketInfo& info);
        void OnClose(const CTL::WebSocketInfo& info);
        void OnError(const CTL::WebSocketInfo& info);
    };

    template<typename T>
    T SQLite3::convert(const std::string &str) {
        if (containsDecimal(str)) {
            return std::stod(str);
        }
        else {
            return std::stoi(str);
        }
    }
}

#endif
