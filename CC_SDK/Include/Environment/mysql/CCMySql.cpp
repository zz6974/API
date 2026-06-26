#include "CCMySql.h"
#include "CCSystem.h"


CTL::MySql::MySql(){
    mysql = new MYSQL;
}

CTL::MySql::~MySql(){
    delete mysql;
    mysql = nullptr;
}

bool CTL::MySql::Connect(const std::string& host, const std::string& user,
                         const std::string& passwd, const std::string& db, const int port) const{
    //初始化mysql
    mysql_init(mysql);  //连接mysql，数据库
//    const char host[] = "localhost";
//    const char user[] = "root";
//    const char psw[] = "111111";
//    const char table[] = "test";
//    const int port = 3306;
    //返回false则连接失败，返回true则连接成功
    if (!(mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0))){
        //中间分别是主机，用户名，密码，数据库名，端口号（可以写默认0或者3306等），可以先写成参数再传进去
        CTL::System::Println("Error connecting to database:{}\n", mysql_error(mysql));
        return false;
    }
    else{
        // printf("Connected...\n");
        return true;
    }
}

CTL::RetData CTL::MySql::Query(const std::string& sql) const{
    MYSQL_RES *Res;
    MYSQL_ROW Row;
    CTL::RetData data;
    if (mysql_query(mysql, sql.c_str())) {
        // CCTimeData time;
        // std::string msg = "[" + time.to_String() + "] -> The query failed -> " + mysql_error(&mysql);
        // CC::Println(msg);
        return {};
    }
    // 获取结果集
    Res = mysql_store_result(mysql);
    if (Res == nullptr) {
        // CCTimeData time;
        // std::string msg = "[" + time.to_String() + "] -> Failed to get the result set -> " + mysql_error(&mysql);
        // CC::Println(msg);
        return {};
    }
    // 遍历结果集
    while ((Row = mysql_fetch_row(Res)) != nullptr) {
        const auto var = mysql_num_fields(Res);
        MYSQL_FIELD* fields = mysql_fetch_fields(Res);
        nlohmann::json json;
        for (unsigned int i = 0; i < var; ++i) {
            if (fields[i].type == MYSQL_TYPE_LONG || fields[i].type == MYSQL_TYPE_TINY
                || fields[i].type == MYSQL_TYPE_SHORT) {
                json[fields[i].name]= String(Row[i]).to_int();
            }
            else if (fields[i].type == MYSQL_TYPE_DOUBLE) {
                json[fields[i].name]= String(Row[i]).to_double();
            }
            else if (fields[i].type == MYSQL_TYPE_FLOAT) {
                json[fields[i].name]= String(Row[i]).to_float();
            }
            else if (fields[i].type == MYSQL_TYPE_BOOL) {
                json[fields[i].name]= String(Row[i]).to_int() == 1;
            }
            else{
                json[fields[i].name]= Row[i];
            }
        }
        data.Data.push_back(json);
    }
    data.Flag = true;
    data.Col = mysql_field_count(mysql);
    data.Row = mysql_num_rows(Res);
    data.Error = mysql_error(mysql);
    // 释放资源
    mysql_free_result(Res);
    return data;
}

void CTL::MySql::Close() const{
    mysql_close(mysql);
}

CTL::RetData CTL::MySql::Execute(const std::string& sql) const{
    RetData data;
    if (mysql_query(mysql, sql.c_str())) {
        // CCTimeData time;
        // std::string msg = "[" + time.to_String() + "] -> Failed to update the database -> " + mysql_error(&mysql);
        // CC::Println(msg);
        data.Flag = false;
        data.Error = mysql_error(mysql);
        return data;
    }
    data.Flag = true;
    return data;
}


