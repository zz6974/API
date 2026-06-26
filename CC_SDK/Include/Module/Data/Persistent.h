#ifndef PERSISTENT_H
#define PERSISTENT_H

#include "CCClassFactory.h"
#include "CCClassRegister.h"
#include "CCSQLite3.h"

namespace CTL{
    enum DatabaseType{
        SQLite,
        MySQL,
    };
    struct PersistentSet{
        bool Flag = false;
        String Error = "";
    };
    class database {
        DatabaseType type = SQLite;
        SQLite3* sqlite{};
#if MY_SQL_BOOL
        MySql *mysql = nullptr;
#endif
    public:
        database();
        ~database();
        sqlite3* getSqlite() const;
        bool connect(const std::string& path);
        bool connect(const NetSqlInfo& Host);
        void close();
        DatabaseType get_sql_type() const;
        CCResultSet execute(const std::string& sql,bool get = false) const;
        void set_type(const DatabaseType& type);
    private:

    };
    struct Column{
        String key; // 列名 必须与注册的字段名一致
        String type; // 数据类型: INTEGER TEXT DATE TIME BOOLEAN BLOB
        String attached; // 附加信息 比如: PRIMARY KEY AUTOINCREMENT
    };
    class Persistent {
        database* db = nullptr;
        struct Column_t{
            String key;
            String type;
            String attached;
            int offset = 0;
        };
        Map<String,std::vector<Column_t>> map_Columns;
    public:
        Persistent() = default;
        ~Persistent();
        void bind(database* db);
        template<typename T>
        PersistentSet createTable(const std::vector<Column>& keys = {},const std::vector<String>& primaryKeys = {}){
            if (!db) return PersistentSet{false,"The database is not connected"};
            const std::string tableName = getTableName<T>();
            T t;
            const auto json = t.to_json();
            String sql = "CREATE TABLE " + tableName + " (";
            Map<String,Column_t> map;
            for (const auto& col: keys) {
                map.put(col.key,{col.key,col.type,col.attached});
            }
            int index = 0;
            for (auto it = json.begin(); it != json.end(); ++it) {
                const String key = it.key();
                const JSON::value_t type = it.value().type();
                String str;
                if (map.IsContains(key)) {
                    const auto col = map.get(key);
                    str = String::format(",{} {} {}",col->key.c_str(),
                        col->type.c_str(),col->attached.c_str());
                }
                else {
                    String type_t = getSqlType(type);
                    map.put(key,{key,type_t, ""});
                    if (db->get_sql_type() == MySQL) {
                        if (type_t == "LIST") {
                            type_t = "TEXT";
                        }
                    }
                    str= String::format(",{} {}",key.c_str(),type_t.c_str());
                }
                if (index == 0) {
                    const auto s = str.find(",");
                    str= str.substr(s + 1,str.length());
                }
                sql.append(str);
                index++;
            }
            if (!primaryKeys.empty()) {
                sql.append(",PRIMARY KEY (");
                index = 0;
                for (const auto& k : primaryKeys) {
                    String str = String::format(",{}",k.c_str());
                    if (index == 0) {
                        const auto s = str.find(",");
                        str= str.substr(s + 1,str.length());
                    }
                    sql.append(str);
                    index++;
                }
                sql.append(")");
            }
            sql.append(");");
            if (map_Columns.IsContains(tableName)) {
                map_Columns.Revise(tableName,map.values());
            }
            else {
                map_Columns.put(tableName,map.values());
            }
            CCResultSet result = db->execute(sql, false);
            if (!result.Flag && result.Error.find("already exists") != String::npos) {
                return syncTableStructure<T>(map.values());
            }
            return PersistentSet{result.Flag,result.Error};
        }
        template<typename T>
        PersistentSet insert(const T& obj){
            if (!db) return {false,"The database is not connected"};
            const std::string tableName = getTableName<T>();
            T t;
            const auto json = obj.to_json();
            String sql = "INSERT INTO " + tableName + " (";
            int index = 0;
            for (auto it = json.begin(); it != json.end(); ++it) {
                const String key = it.key();
                const JSON::value_t type = it.value().type();
                String str = String::format(",{}",key.c_str());
                if (index == 0) {
                    const auto s = str.find(",");
                    str= str.substr(s + 1,str.length());
                }
                sql.append(str);
                index++;
            }
            sql.append(") VALUES (");
            index = 0;
            for (auto it = json.begin(); it != json.end(); ++it) {
                try {
                    const JSON::value_t type = it.value().type();
                    String type_t = getSqlType(type);
                    String str;
                    if (type_t == "LIST") {
                        const auto value = it.value();
                        // JSONObject Json;
                        // Json.put("LIST",value);
                        const String str_t = value.dump();
                        str = String::format(",'{}'",str_t.c_str());
                    }
                    else if (type_t == "TEXT") {
                        const String value = it.value();
                        str = String::format(",'{}'",value.c_str());
                    }
                    else if (type_t == "BOOLEAN") {
                        const bool value = it.value();
                        str = String::format(",{}",value);
                    }
                    else if (type_t == "REAL"){
                        const float value = it.value();
                        str = String::format(",{}",value);
                    }
                    else if (type_t == "INTEGER"){
                        const int value = it.value();
                        str = String::format(",{}",value);
                    }
                    else if (type_t == "OBJ") {
                        const JSON value = it.value();
                        const String str_t = value.dump();
                        str = String::format(",'{}'",str_t.c_str());
                    }
                    if (index == 0) {
                        const auto s = str.find(",");
                        str= str.substr(s + 1,str.length());
                    }
                    sql.append(str);
                    index++;
                }
                catch (CCException& e) {
                    System::Println("Persistent insert Error: {}",e.what());
                }
            }
            sql.append(");");
            const auto result = db->execute(sql, false);
            return {result.Flag,result.Error};
        }
        template<typename T,typename ... Args>
        std::vector<T> query(const char * where,Args... args) const{
            std::vector<T> results;
            if (!db) return results;
            // 格式化WHERE子句
            std::string formattedWhere = String::format(where, args...);
            // 获取表名
            const std::string tableName = getTableName<T>();
            // 构建SQL查询语句
            String sql = String::format("SELECT * FROM {}", tableName.c_str());
            // 添加WHERE条件
            if (!formattedWhere.empty()) {
                sql.append(" WHERE ").append(formattedWhere);
            }
            sql.append(";");
            // 执行查询
            const CCResultSet result = db->execute(sql, true);
            if (!result.Flag) return results;
            // 将结果转换为对象
            for (int i = 0;i < result.Row;i++) {
                try {
                    T obj;
                    JSON row = result.Data[i];
                    const auto cols = map_Columns.at(tableName);
                    for (const auto& col : cols) {
                        if (col.type == "LIST") {
                            String jsonString = row[col.key];
                            if (jsonString.length() >= 2 && jsonString.front() == '"' && jsonString.back() == '"') {
                                jsonString = jsonString.substr(1, jsonString.length() - 2);
                            }
                            row[col.key] = JSON::parse(jsonString);
                        }
                        else if (col.type == "BOOLEAN") {
                            row[col.key]= row[col.key].get<int>() == 1;
                        }
                        else if (col.type == "OBJ") {
                            String jsonString = row[col.key];
                            row[col.key]= JSON::parse(jsonString);
                        }
                    }
                    obj.from_json_t(row);
                    results.push_back(obj);
                }
                catch (CCException& e) {
                    System::Println("Persistent query Error: {}",e.what());
                }
            }
            return results;
        }
        template<typename T,typename ... Args>
        PersistentSet remove(const char * where,Args... args) const{
            if (!db) return {false,"The database is not connected"};
            const std::string formattedWhere = String::format(where, args...);
            const std::string tableName = getTableName<T>();
            const String sql = String::format("DELETE FROM {} WHERE {}",
                tableName.c_str(), formattedWhere.c_str());
            const auto result = db->execute(sql, false);
            return {result.Flag,result.Error};
        }
        template<typename T,typename ... Args>
        PersistentSet update(const T& obj,const char * where,Args... args) const{
            if (!db) return {false,"The database is not connected"};
            std::string formattedWhere = String::format(where, args...);
            const std::string tableName = getTableName<T>();
            String sql = String::format("UPDATE {} SET ", tableName.c_str());
            const auto json = obj.to_json();
            int index = 0;
            for (auto it = json.begin(); it != json.end(); ++it) {
                try {
                    const String key = it.key();
                    const JSON::value_t type = it.value().type();
                    String type_t = getSqlType(type);
                    String str;
                    if (type_t == "LIST") {
                        const auto value = it.value();
                        const String str_t = value.dump();
                        str= String::format(",{} = '{}'",key.c_str(),str_t.c_str());
                    }
                    else if (type_t == "TEXT") {
                        const String value = it.value();
                        str= String::format(",{} = '{}'",key.c_str(),value.c_str());
                    }
                    else if (type_t == "BOOLEAN") {
                        const bool value = it.value();
                        str= String::format(",{} = {}",key.c_str(),value);
                    }
                    else if (type_t == "REAL"){
                        const float value = it.value();
                        str= String::format(",{} = {}",key.c_str(),value);
                    }
                    else if (type_t == "INTEGER"){
                        const int value = it.value();
                        str= String::format(",{} = {}",key.c_str(),value);
                    }
                    else if (type_t == "OBJ") {
                        const JSON value = it.value();
                        const String str_t = value.dump();
                        str = String::format(",'{}'",str_t.c_str());
                    }
                    if (index == 0) {
                        const auto s = str.find(",");
                        str= str.substr(s + 1,str.length());
                    }
                    sql.append(str);
                    index++;
                }
                catch (CCException& e) {
                    System::Println("Persistent update Error: {}",e.what());
                }
            }
            sql.append(" WHERE ").append(formattedWhere);
            const auto result = db->execute(sql, false);
            return {result.Flag,result.Error};
        }
        template<typename T>
        PersistentSet deleteTable() const{
            if (!db) return {false,"The database is not connected"};
            const std::string tableName = getTableName<T>();
            const auto result = db->execute(String::format("DROP TABLE IF EXISTS {}", tableName.c_str()), false);
            return{result.Flag,result.Error};
        }
        database* getDatabase() const;
    private:
        String getSqlType(const JSON::value_t& type) const;
        template<typename T>
        static std::string getTableName() {
            std::string name = typeid(T).name();
#ifdef _MSC_VER
            // MSVC 处理: "class User" -> "User"
            size_t pos = name.find(' ');
            if (pos != std::string::npos) {
                name = name.substr(pos + 1);
            }
#elif defined(__GNUC__)
            // GCC/MinGW 处理: "4User" -> "User"
            // GCC 返回的名字可能以数字开头表示长度
            if (!name.empty() && std::isdigit(name[0])) {
                int len = 0;
                size_t i = 0;
                // 解析长度前缀
                while (i < name.length() && std::isdigit(name[i])) {
                    len = len * 10 + (name[i] - '0');
                    i++;
                }
                if (i + len <= name.length()) {
                    name = name.substr(i, len);
                }
            }
#endif
            return name;
        }
        template<typename T>
        bool addColumn(const String& columnName, const String& columnType) const{
            if (!db) return false;
            const std::string tableName = getTableName<T>();
            String sql = String::format("ALTER TABLE %s ADD COLUMN %s %s;",
                                       tableName.c_str(), columnName.c_str(), columnType.c_str());
            CCResultSet result = db->execute(sql, false);
            return result.Flag;
        }
        template<typename T>
        PersistentSet syncTableStructure(const std::vector<Column_t>& expectedColumns) const{
            if (!db) return {false,"The database is not connected"};
            const std::string tableName = getTableName<T>();
            String pragmaSql = String::format("PRAGMA table_info(%s);", tableName.c_str());
            CCResultSet currentSchema = db->execute(pragmaSql, true);
            std::set<String> existingColumns;
            for (const auto& row : currentSchema.Data) {
                existingColumns.insert(row["name"].get<String>());
            }
            bool success = true;
            for (const auto& expectedCol : expectedColumns) {
                if (existingColumns.find(expectedCol.key) == existingColumns.end()) {
                    String alterSql = String::format("ALTER TABLE %s ADD COLUMN %s %s %s;",
                                                   tableName.c_str(),
                                                   expectedCol.key.c_str(),
                                                   expectedCol.type.c_str(),
                                                   expectedCol.attached.c_str());
                    CCResultSet result = db->execute(alterSql, false);
                    if (!result.Flag) {
                        success = false;
                    }
                }
            }
            return {success,currentSchema.Error};
        }
    };
}

#define PERSISTENT_INTRUSIVE(Type, ...) \
JSON_TYPE_INTRUSIVE(Type,__VA_ARGS__) \
public: \
CTL::JSONObject to_json() const{ \
    return *this; \
} \
void from_json_t(const JSON& json){ \
const auto user = json.get<Type>(); \
    *this = user; \
}


#endif
