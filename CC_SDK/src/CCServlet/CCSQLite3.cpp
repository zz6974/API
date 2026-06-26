#include "CCSQLite3.h"
#include "httplib.h"

using namespace CTL;
SQLite3::SQLite3(const CTL::String &Path) {
    Open(Path);
}

bool SQLite3::Open(const CTL::String& Path) {
    int ret = sqlite3_open(Path.c_str(),&SQL);
    if (ret)
    {
        return false;
    }
    RemoteLink = false;
    return true;
}

bool SQLite3::Init(sqlite3* db) {
    this->SQL = db;
    return this->SQL != nullptr;
}

bool SQLite3::Open(const NetSqlInfo &Host) {
    RemoteLink = true;
    Blockage = true;
    this->M_Host = Host;
    ws.OnBind(CC_WSFunClass_CXX11(OnOpen),CC_WSFunClass_CXX11(OnClose),
        CC_WSFunClass_CXX11(OnMessage),CC_WSFunClass_CXX11(OnError));
    bool F = ws.Connect(this->M_Host.IP, Host.Port,"/SQLITE");
    if (F) {
        M_Thread.SetThread(&WebSocketClient::Running,&ws);
        M_Thread.Start();
    }
    else {
        return false;
    }
    while (this->Blockage && F) {
        CTL::Thread::SleepMS(100);
    }
    return this->LinkStatus;
}

CCSqlSet SQLite3::SqlUpdate(const CTL::String& sql) {
    CCSqlSet Set;
    if (!RemoteLink) {
        Set.Flag = SQlite(sql.c_str(), nullptr, nullptr);
        if (!Set.Flag) {
            Set.Error = GetError();
        }
    }
    else {
        httplib::Client client(M_Host.IP, M_Host.Port);
        JSONObject json;
        json.put("Type",1);
        json.put("Sql",sql);
        json.put("User",M_Host.User);
        json.put("Pass",M_Host.Pass);
        json.put("Code",ClientInfo.Code);
        json.put("ID",ClientInfo.ID);
        auto res = client.Post("/SQLOperation", json.to_String(), "application/json");
        if (res->status == 200) {
            CTL::JSONObject Json = JSONObject::parse(res->body);
            const int Code = Json.get("Code");
            Set.Flag = Code == 1 ? true : false;
            if (!Set.Flag) {
                Set.Error = Json.get("Message");
            }
        }
    }
    return Set;
}

CCResultSet SQLite3::SqlQuery(const CTL::String& sql) {
    if (!RemoteLink) {
 CCResultSet Row_Col;
        sqlite3_stmt* stmt = nullptr;

        // 准备语句
        int rc = sqlite3_prepare_v2(SQL, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            Row_Col.Error = sqlite3_errmsg(SQL);
            return Row_Col;
        }

        // 获取列数
        const int nCol = sqlite3_column_count(stmt);
        Row_Col.Col = nCol;

        // 执行查询并遍历结果
        int nRow = 0;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            JSON strCol;

            for (int j = 0; j < nCol; j++) {
                const char* colName = sqlite3_column_name(stmt, j);
                int actualType = sqlite3_column_type(stmt, j);

                // 根据实际数据类型获取值
                switch(actualType) {
                    case SQLITE_INTEGER: {
                        sqlite3_int64 val = sqlite3_column_int64(stmt, j);
                        strCol[colName] = static_cast<int>(val);
                        break;
                    }
                    case SQLITE_FLOAT: {
                        double val = sqlite3_column_double(stmt, j);
                        strCol[colName] = val;
                        break;
                    }
                    case SQLITE_TEXT: {
                        const unsigned char* val = sqlite3_column_text(stmt, j);
                        if (val) {
                            strCol[colName] = reinterpret_cast<const char*>(val);
                        } else {
                            strCol[colName] = "";
                        }
                        break;
                    }
                    case SQLITE_BLOB: {
                        int bytes = sqlite3_column_bytes(stmt, j);
                        const auto* blob = static_cast<const unsigned char*>(sqlite3_column_blob(stmt, j));
                        if (blob && bytes > 0) {
                            // 将 BLOB 数据转换为 base64 字符串或十六进制字符串
                            std::string hexStr;
                            for (int i = 0; i < bytes; i++) {
                                char hex[4];
                                snprintf(hex, sizeof(hex), "%02x", blob[i]);
                                hexStr += hex;
                            }
                            strCol[colName] = hexStr;
                        } else {
                            strCol[colName] = "";
                        }
                        break;
                    }
                    case SQLITE_NULL: {
                        strCol[colName] = nullptr;
                        break;
                    }
                    default: {
                        // 未知类型，尝试作为文本处理
                        const unsigned char* val = sqlite3_column_text(stmt, j);
                        if (val) {
                            strCol[colName] = reinterpret_cast<const char*>(val);
                        } else {
                            strCol[colName] = "";
                        }
                        break;
                    }
                }
            }

            Row_Col.Data.push_back(strCol);
            nRow++;
        }

        Row_Col.Row = nRow;

        // 检查是否有错误
        if (rc != SQLITE_DONE) {
            Row_Col.Error = sqlite3_errmsg(SQL);
            sqlite3_finalize(stmt);
            return Row_Col;
        }

        // 清理
        sqlite3_finalize(stmt);
        Row_Col.Flag = true;
        return Row_Col;
    }
    else {
        CCResultSet Row_Col;
        httplib::Client client(M_Host.IP, M_Host.Port);
        JSONObject json;
        json.put("Type",0);
        json.put("Sql",sql);
        json.put("User",M_Host.User);
        json.put("Pass",M_Host.Pass);
        json.put("Code",ClientInfo.Code);
        json.put("ID",ClientInfo.ID);
        auto res = client.Post("/SQLOperation", json.to_String(), "application/json");
        if (res->status == 200) {
            CTL::JSONObject Json = JSONObject::parse(res->body);
            const int Code = Json.get("Code");
            if (Code == 1) {
                Row_Col.Data = Json.get("Data").get<std::vector<JSON>>();
                Row_Col.Row = Row_Col.Data.size();
                Row_Col.Flag = true;
            }
            else {
                Row_Col.Error = Json.get("Message");
            }
        }
        return Row_Col;
    }
}

bool SQLite3::Close(){
    if (!RemoteLink) {
        return sqlite3_close(SQL) != 0;
    }
    else {
        ws.Stop();
    }
    return false;
}

bool SQLite3::SQlite(const char* sql,int(*callback)(void *,int,char**,char**),void* a){
    CCUniqueLock _mutex(mutex);
    const int rc = sqlite3_exec(SQL, sql, callback, a, &zErrMsg);
    if (rc != SQLITE_OK){
        zErrMsgStr = zErrMsg;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

CTL::String SQLite3::GetError() {
    return zErrMsgStr;
}

sqlite3* SQLite3::GetSqlite() const{
    return SQL;
}

std::string SQLite3::getTypeName(const int type) {
    switch(type) {
        case SQLITE_INTEGER: return "INTEGER";
        case SQLITE_FLOAT:   return "FLOAT";
        case SQLITE_TEXT:    return "TEXT";
        case SQLITE_BLOB:    return "BLOB";
        case SQLITE_NULL:    return "NULL";
        default:             return "UNKNOWN";
    }
}

bool SQLite3::queryWithTypes(sqlite3 *db, const std::string &sql) {
    sqlite3_stmt* stmt = nullptr;

    // 准备语句
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // 获取列数和列名
    const int nCol = sqlite3_column_count(stmt);
    // std::cout << "列数: " << nCol << std::endl;

    // 获取每一列的名称和声明类型
    for (int i = 0; i < nCol; i++) {
        const char* colName = sqlite3_column_name(stmt, i);
        const char* colType = sqlite3_column_decltype(stmt, i); // 声明的类型
        const auto* colValue = sqlite3_column_value(stmt, i);

        std::cout << "列 " << i << ": " << colName;
        if (colType) {
            std::cout << " (声明类型: " << colType << ")";
        }
        std::cout << std::endl;
    }

    // 执行查询并遍历结果
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        for (int i = 0; i < nCol; i++) {
            // 获取实际数据类型
            int actualType = sqlite3_column_type(stmt, i);

            // 根据类型获取值
            switch(actualType) {
                case SQLITE_INTEGER: {
                    sqlite3_int64 val = sqlite3_column_int64(stmt, i);
                    std::cout << val << " (INTEGER)";
                    break;
                }
                case SQLITE_FLOAT: {
                    double val = sqlite3_column_double(stmt, i);
                    std::cout << val << " (FLOAT)";
                    break;
                }
                case SQLITE_TEXT: {
                    const unsigned char* val = sqlite3_column_text(stmt, i);
                    std::cout << (val ? (const char*)val : "NULL") << " (TEXT)";
                    break;
                }
                case SQLITE_BLOB: {
                    int bytes = sqlite3_column_bytes(stmt, i);
                    std::cout << "[BLOB " << bytes << " bytes]";
                    break;
                }
                case SQLITE_NULL: {
                    std::cout << "NULL";
                    break;
                }
            }
            std::cout << "\t";
        }
        std::cout << std::endl;
    }

    // 清理
    sqlite3_finalize(stmt);
    return true;
}

bool SQLite3::containsDecimal(const std::string &str) {
    try {
        char* p = nullptr;
        const double d = strtod(str.c_str(), &p);
        return d != HUGE_VAL && *p == '\0';
    }
    catch (std::exception &e){
        return false;
    }
}

bool SQLite3::is_int(const std::string& str){
    // 字符串str转换成整型数字i
    int i = atoi(str.c_str());
    // 数字i转换成字符串strtmp
    char temp[12] = {};
    snprintf(temp, 12, "%d", i);
    // 比较字符串转换后是否前后一致
    if (strncmp(str.c_str(), temp, 12) != 0){
        return false;
    }
    return true;
}

void SQLite3::OnOpen(CTL::WebSocketInfo &info) {
    timer.Start([this]() {
        this->Blockage = false;
        timer.Stop();
    },5,CTL::TimeType::Second);
}

void SQLite3::OnMessage(CTL::WebSocketInfo &info) {
    auto buffer = info.GetBuffer();
    auto str = buffer.toString();
    JSONObject json = JSONObject::parse(str);
    const CTL::String Key= json.get("Key");
    if (Key == "Connect") {
        const CTL::String Code = json.get("Code");
        const int ID = json.get("ID");
        this->ClientInfo.ID = ID;
        this->ClientInfo.Code = Code;
        this->LinkStatus = true;
        this->Blockage = false;
        timer.Stop();
    }
    else {
        CC::Println("NOT Key");
    }
}

void SQLite3::OnClose(const CTL::WebSocketInfo &info) {
    ClientInfo.ID = -1;
    LinkStatus = false;
}

void SQLite3::OnError(const CTL::WebSocketInfo &info) {
    ClientInfo.ID = -1;
    LinkStatus = false;
}

