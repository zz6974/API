#include "Persistent.h"

CTL::database::database(){
    sqlite = new SQLite3;
#if MY_SQL_BOOL
    mysql = new MySql;
#endif
}

CTL::database::~database(){
    delete sqlite;
    sqlite = nullptr;
#if MY_SQL_BOOL
    delete mysql;
    mysql = nullptr;
#endif
}

sqlite3* CTL::database::getSqlite() const{
    return sqlite->GetSqlite();
}

bool CTL::database::connect(const std::string& path){
    type = SQLite;
    return sqlite->Open(path);
}

bool CTL::database::connect(const NetSqlInfo& Host){
#if MY_SQL_BOOL
    type = MySQL;
    return mysql->Connect(Host.IP,Host.User,Host.Pass, Host.DBName,Host.Port);
#else
    type = SQLite;
    return sqlite->Open(Host); // 待完善,伪码
#endif
}

void CTL::database::close(){
    if (type == SQLite) {
        sqlite->Close();
    }
    else if (type == MySQL) {
#if MY_SQL_BOOL
        mysql->Close();
#else
        sqlite->Close(); // 待完善,伪码
#endif
    }
}

CTL::DatabaseType CTL::database::get_sql_type() const{
    return type;
}

CTL::CCResultSet CTL::database::execute(const std::string& sql, const bool get) const{
    CCResultSet result;
    if (type == SQLite) {
        if (!get) {
            const auto ret = sqlite->SqlUpdate(sql);
            result.Flag = ret.Flag;
            result.Error = ret.Error;
        }
        else {
            result = sqlite->SqlQuery(sql);
        }
    }
    else if (type == MySQL) {
#if MY_SQL_BOOL
        if (!get) {
            const auto ret = mysql->Execute(sql);
            result.Flag = ret.Flag;
            result.Error = ret.Error;
        }
        else {
            const auto ret = mysql->Query(sql);
            result.Flag = ret.Flag;
            result.Col = ret.Col;
            result.Data = ret.Data;
            result.Error = ret.Error;
            result.Row = ret.Row;
        }
#endif
    }
    return result;
}

void CTL::database::set_type(const DatabaseType& type){
    this->type = type;
}

CTL::Persistent::~Persistent(){

}

void CTL::Persistent::bind(database* db){
    this->db = db;
}

CTL::database* CTL::Persistent::getDatabase() const {
    return this->db;
}

CTL::String CTL::Persistent::getSqlType(const JSON::value_t& type) const{
    const auto dbType = db->get_sql_type();
    switch (type) {
    case JSON::value_t::number_integer:
    case JSON::value_t::number_unsigned:
        return "INTEGER";
    case JSON::value_t::number_float:
        return "REAL";
    case JSON::value_t::string:
        return "TEXT";
    case JSON::value_t::array:{
        return "LIST";
    }
    case JSON::value_t::object:
        return "OBJ";
    case JSON::value_t::boolean:
        return "BOOLEAN";
    default:
        return "NULL";
    }
}
