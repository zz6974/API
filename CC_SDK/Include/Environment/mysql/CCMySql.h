#ifndef CC_MySql_H
#define CC_MySql_H

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include "mysql.h"
#include "string"
#include "vector"
#include "nlohmann/json.hpp"

/*
 * create DATABASE xxx;  创建库
 * DROP DATABASE xxx; 删除库
 *
 *
 * //创建表
 * CREATE TABLE xxx (
        column1 datatype,
        column2 datatype,
        ...
    );

    //删除表
    DROP TABLE xxx;  or  DROP TABLE [IF EXISTS] xxx;

    //插入表
    INSERT INTO xxx (column1, column2, column3, ...) VALUES (value1, value2, value3, ...);

    //查询表
    SELECT column1, column2, ... FROM xxx;  or  SELECT * FROM xxx;   or SELECT * FROM xxx WHERE ID='xx';
 * */

namespace CTL{
    struct RetData{
        std::vector<nlohmann::json> Data;
        /// 结果集中行数
        int Row = 0;
        /// 结果集中列数
        int Col = 0;
        /// 操作是否成功标志
        bool Flag = false;
        std::string Error;
    };
    class MySql{
        MYSQL* mysql = nullptr; //mysql连接
    public:
        MySql();
        ~MySql();
        /**
         * 连接到MySQL数据库
         * @param host 数据库主机地址
         * @param user 数据库用户名
         * @param passwd 数据库密码
         * @param db 数据库名
         * @param port 数据库端口
         * @return 连接成功返回true，否则返回false
         */
        bool Connect(const std::string& host, const std::string& user, const std::string& passwd, const std::string& db, int port) const;
        /**
         * 执行SQL查询
         * @param sql 要执行的SQL查询语句
         * @return 返回查询结果，包括数据表中的数据
         */
        RetData Query(const std::string& sql) const;
        /**
         * 执行SQL语句（非查询）
         * @param sql 要执行的SQL语句，如插入、更新、删除操作
         * @return 执行成功返回true，否则返回false
         */
        RetData Execute(const std::string& sql) const;
        /**
         * 关闭数据库连接
         */
        void Close() const;
    private:

    public:

    };
}

#endif
