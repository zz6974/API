#include "DataModule/NetSqlite.h"
#include <CCThread.h>

CTL::NetSqlite::~NetSqlite() {
    Close();
}

bool CTL::NetSqlite::InitService(const NetSqlInfo &Host) {
    const CCFile SQLite_(SQLite_Path);
    if (!SQLite_.isExists()) {
        CCFile::Create(SQLiteFolder_Path,true);
        CCFile::Create(BackUpFolder_Path,true);
        CCFile::Create(SQLite_Path);
    }
    if (m_Sqlite.Open(SQLite_Path)) {
        if (Host.CORS_Header != nullptr) {
            m_Socket.SetWebCORS(*Host.CORS_Header);
        }
        this->m_User = Host.User;
        this->M_Pass = Host.Pass;
        const bool F = m_Socket.Init(Any,Host.Port);
        if (F) {
            m_Socket.SetWebServlet("/SQLOperation",CC_ServletFunClass_CXX11(SQLOperation));
            m_Socket.SetWebServlet("/SQLBackup",CC_ServletFunClass_CXX11(SQLBackup));
            m_Socket.SetWebServlet("/SQLRestore",CC_ServletFunClass_CXX11(SQLRestore));
            m_Socket.SetWebServlet("/SQLDeleteBackup",CC_ServletFunClass_CXX11(SQLDeleteBackup));
            m_Socket.SetWebServlet("/GetSQLBackupList",CC_ServletFunClass_CXX11(GetSQLBackupList));
        }
        return F;
    }
    m_Sqlite.Close();
    m_Socket.Stop();
    return false;
}

void CTL::NetSqlite::Close() {
    Flag = false;
    m_Sqlite.Close();
    m_Socket.Stop();
}

void CTL::NetSqlite::Running() {
    this->Flag = true;
    WSBind bind;
    bind.URL = "/SQLITE";
    bind.OnOpenFun = CC_WSFunClass_CXX11(OnOpen);
    bind.OnCloseFun = CC_WSFunClass_CXX11(OnClose);
    bind.OnMessageFun = CC_WSFunClass_CXX11(OnMessage);
    bind.OnErrorFun = CC_WSFunClass_CXX11(OnError);
    m_Socket.SetWebSocket(bind);
    while (this->Flag) {
        Thread::Sleep(1000 * 500);
        m_Socket.Running(true);
    }
}

bool CTL::NetSqlite::Restore(const String &Name) {
    m_Sqlite.Close();
    Thread::Sleep(1000 * 100);
    const CCFile fa(SQLite_Path);
    bool F = fa.Delete();
    if (F) {
        const String dst = CCFile::NormalizePath(BackUpFolder_Path + "/" + Name + ".db");
        if (!CCFile::Copy(dst, SQLite_Path)) {
            F = false;
        }
    }
    m_Sqlite.Open(SQLite_Path);
    return F;
}

CTL::String CTL::NetSqlite::GetStrKey(const int ID) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const unsigned seed = static_cast<unsigned int>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    std::mt19937 generator(seed);
    std::uniform_int_distribution<> dist(0, charset.size() - 1);

    std::string result;
    result.reserve(32);
    for (size_t i = 0; i < 32; ++i) {
        result += charset[dist(generator)];
    }
    return result + "@" + std::to_string(ID);
}

void CTL::NetSqlite::SQLOperation(Request& request, Response& response) {
    try {
        JSONObject j = request.GetJson();
        const String User = j.get("User");
        const String Pass = j.get("Pass");
        try {
            const int ID = j.get("ID");
            const String Code = j.get("Code");
            auto Client = m_Sqlite_List_User.Get(ID);
            if (Client) {
                if (*Client != Code) {
                    JSONObject SData = {};
                    SData.put("Code",0);
                    SData.put("Message","Verification code error");
                    response.Write(SData.to_String(),200);
                    return;
                }
            }
            else {
                JSONObject SData = {};
                SData.put("Code",0);
                SData.put("Message","Client is unverified or does not exist");
                response.Write(SData.to_String(),200);
                return;
            }
        }
        catch (CCException& e) {
            JSONObject SData = {};
            SData.put("Code",0);
            SData.put("Message","Verification code error");
            response.Write(SData.to_String(),200);
            return;
        }
        if (User == this->m_User && Pass == this->M_Pass) {
            try {
                const int Type = j.get("Type");
                const String Sql = j.get("Sql");
                if (Type == 1) {
                    auto F = m_Sqlite.SqlUpdate(Sql);
                    String Error = "";
                    if (!F.Flag) {
                        Error = m_Sqlite.GetError();
                    }
                    JSONObject SData = {};
                    SData.put("Code",F.Flag ? 1 : 0);
                    SData.put("Message",Error);
                    const String SendStr = SData.to_String();
                    response.Write(SendStr,200);
                }
                else if (Type == 0) {
                    auto Result = m_Sqlite.SqlQuery(Sql);
                    JSONObject SData = {};
                    SData.put("Code",Result.Flag ? 1 : 0);
                    SData.put("Message",Result.Error);
                    SData.put("Data",Result.Data);
                    const String SendStr = SData.to_String();
                    response.Write(SendStr,200);
                }
            }
            catch (CCException& e) {
                JSONObject SData = {};
                SData.put("Code",0);
                SData.put("Message",String::format("Receive processing -> %s",e.what()));
                response.Write(SData.to_String(),200);
            }
        }
        else {
            JSONObject SData = {};
            SData.put("Code",1);
            SData.put("Message","The user name or password is incorrect");
            response.Write(SData.to_String(),200);
        }
    }
    catch (CCException& e) {
        JSONObject SData = {};
        SData.put("Code",0);
        SData.put("Message",String::format("Data parsing exception -> %s",e.what()));
        response.Write(SData.to_String(),200);
    }
}

void CTL::NetSqlite::SQLBackup(Request &request, Response &response) {
    try {
        JSONObject j = request.GetJson();
        try {
            const int ID = j.get("ID");
            const String Code = j.get("Code");
            auto Client = m_Sqlite_List_User.Get(ID);
            if (Client) {
                if (*Client != Code) {
                    JSONObject SData = {};
                    SData.put("Code",0);
                    SData.put("Message","Verification code error");
                    response.Write(SData.to_String(),200);
                    return;
                }
            }
            else {
                JSONObject SData = {};
                SData.put("Code",0);
                SData.put("Message","Client is unverified or does not exist");
                response.Write(SData.to_String(),200);
                return;
            }
        }
        catch (CCException& e) {
            JSONObject SData = {};
            SData.put("Code",0);
            SData.put("Message","Verification code error");
            response.Write(SData.to_String(),200);
            return;
        }
        const String Name = j.get("Name");
        if (Name == "") {
            JSONObject SData = {};
            SData.put("Code",0);
            SData.put("Message","The name cannot be empty");
            response.Write(SData.to_String(),200);
            return;
        }
        const String dst = CCFile::NormalizePath(BackUpFolder_Path + "/" + Name + ".db");
        const bool F = CCFile::Copy(SQLite_Path,dst);
        JSONObject SData= {};
        SData.put("Code",F ? 1 : 0);
        response.Write(SData.to_String(),200);
    }
    catch (CCException& e) {
        JSONObject SData = {};
        SData.put("Code",0);
        SData.put("Message",String::format("Data parsing exception -> %s",e.what()));
        response.Write(SData.to_String(),200);
    }
}

void CTL::NetSqlite::SQLRestore(Request &request, Response &response) {
    try {
        JSONObject j = request.GetJson();
        try {
            const int ID = j.get("ID");
            const String Code = j.get("Code");
            const auto Client = m_Sqlite_List_User.Get(ID);
            if (Client) {
                if (*Client != Code) {
                    JSONObject SData = {};
                    SData.put("Code",0);
                    SData.put("Message","Verification code error");
                    response.Write(SData.to_String(),200);
                    return;
                }
            }
            else {
                JSONObject SData = {};
                SData.put("Code",0);
                SData.put("Message","Client is unverified or does not exist");
                response.Write(SData.to_String(),200);
                return;
            }
        }
        catch (CCException& e) {
            JSONObject SData = {};
            SData.put("Code",0);
            SData.put("Message","Verification code error");
            response.Write(SData.to_String(),200);
            return;
        }
        const String Name = j.get("Name");
        if (Name == "") {
            JSONObject SData = {};
            SData.put("Code",0);
            SData.put("Message","The name cannot be empty");
            response.Write(SData.to_String(),200);
            return;
        }
        const bool F = Restore(Name);
        JSONObject SData = {};
        SData.put("Code",F ? 1 : 0);
        response.Write(SData.to_String(),200);
    }
    catch (CCException& e) {
        JSONObject SData = {};
        SData.put("Code",0);
        SData.put("Message",String::format("Data parsing exception -> %s",e.what()));
        response.Write(SData.to_String(),200);
    }
}

void CTL::NetSqlite::SQLDeleteBackup(Request &request, Response &response) {
    try {
        JSONObject j = request.GetJson();
        try {
            const int ID = j.get("ID");
            const String Code = j.get("Code");
            const auto Client = m_Sqlite_List_User.Get(ID);
            if (Client) {
                if (*Client != Code) {
                    JSONObject SData = {};
                    SData.put("Code",0);
                    SData.put("Message","Verification code error");
                    response.Write(SData.to_String(),200);
                    return;
                }
            }
            else {
                JSONObject SData = {};
                SData.put("Code",0);
                SData.put("Message","Client is unverified or does not exist");
                response.Write(SData.to_String(),200);
                return;
            }
        }
        catch (CCException& e) {
            JSONObject SData = {};
            SData.put("Code",0);
            SData.put("Message","Verification code error");
            response.Write(SData.to_String(),200);
            return;
        }
        const String Name = j.get("Name");
        if (Name == "") {
            JSONObject SData = {};
            SData.put("Code",0);
            SData.put("Message","The name cannot be empty");
            response.Write(SData.to_String(),200);
            return;
        }
        const String dst = CCFile::NormalizePath(BackUpFolder_Path + "/" + Name + ".db");
        const CCFile f(dst);
        const bool F = f.Delete();
        JSONObject SData= {};
        SData.put("Code",F ? 1 : 0);
        response.Write(SData.to_String(),200);
    }
    catch (CCException& e) {
        JSONObject SData = {};
        SData.put("Code",0);
        SData.put("Message",String::format("Data parsing exception -> %s",e.what()));
        response.Write(SData.to_String(),200);
    }
}

void CTL::NetSqlite::GetSQLBackupList(Request &request, Response &response) {
    try {
        const CCVector<CCFileInfo> FList = CCFile::GetDirectoryContents(BackUpFolder_Path);
        JSONObject SData = {};
        SData.put("Code",1);
        SData.put("Message","Success");
        CCVector<JSON> List;
        for (const auto& i : FList) {
            JSONObject Data;
            if (i.IsDirectory) {
                continue;
            }
            const String Name = i.Name;
            const size_t Size = i.Size;
            CCTimeData ta(CCTimeData::GetTime(i.LastWriteTime));
            const String Time = ta.to_String();
            Data.put("Name",Name);
            Data.put("Size",Size);
            Data.put("Time",Time);
            List.add_lock(Data);
        }
        SData.put("Data",List);
        response.Write(SData.to_String(),200);
    }
    catch (CCException& e) {
        JSONObject SData = {};
        SData.put("Code",0);
        SData.put("Message",String::format("Data parsing exception -> %s",e.what()));
        response.Write(SData.to_String(),200);
    }
}

void CTL::NetSqlite::OnOpen(CTL::WebSocketInfo &info) {
    const int ID = info.GetID();
    const CTL::String str = GetStrKey(ID);
    m_Sqlite_List_User.Put(ID,str);
    CTL::Thread::SleepMS(1000);
    CTL::JSONObject Json = {};
    Json.put("Key","Connect");
    Json.put("Code",str);
    Json.put("ID",ID);
    info.SendText(Json.to_String());
    CC::Println("NetSqlite OnOpen");
}

void CTL::NetSqlite::OnMessage(CTL::WebSocketInfo &info) {

}

void CTL::NetSqlite::OnClose(const CTL::WebSocketInfo &info) {
    const int ID = info.GetID();
    m_Sqlite_List_User.Remove(ID);
}

void CTL::NetSqlite::OnError(const CTL::WebSocketInfo &info) {
    const int ID = info.GetID();
    m_Sqlite_List_User.Remove(ID);
}


