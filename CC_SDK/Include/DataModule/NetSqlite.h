#ifndef CC_NET_SQLITE_H
#define CC_NET_SQLITE_H

#include "CCSQLite3.h"
#include "TL/Map.h"

/*
 * NetSqlite
 * {"Type":1,"User":"xxxx","Pass":"xxxxx","Sql":"......","Code":"...","ID":0} Type : 0读取 1写入,User: 用户名,Pass: 密码,Sql: SQL语句 Code: 验证码 ID: 客户端ID
 * {"Code":1,"Message":"xxxxx","Data":""} Code : 0失败 1成功,Message: 消息,Data: 数据
 */



namespace CTL {
    class NetSqlite {
    public:
        //--------------------------------------------------------------------------------------------------------------
        NetSqlite () = default;
        ~NetSqlite ();
        //--------------------------------------------------------------------------------------------------------------
        bool InitService(const NetSqlInfo &Host);
        void Close();
        void Running();
        //--------------------------------------------------------------------------------------------------------------
    private:
        //--------------------------------------------------------------------------------------------------------------
        bool Restore(const String& Name);
        //--------------------------------------------------------------------------------------------------------------
        SQLite3 m_Sqlite;
        WebServlet m_Socket;
        CTL::Map<int,CTL::String> m_Sqlite_List_User;
        bool Flag = false;
        String m_User = "";
        String M_Pass = "";
        const String Exe_Path = CCFile::NormalizePath("../");
        const String SQLiteFolder_Path = CCFile::NormalizePath("../Archive/");
        const String BackUpFolder_Path = CCFile::NormalizePath("../Archive/BackupCopy/");
        const String SQLite_Path = CCFile::NormalizePath(SQLiteFolder_Path + "/" + "Sqlite.db");
        //--------------------------------------------------------------------------------------------------------------
        static String GetStrKey(const int ID);
        void SQLOperation(Request& request, Response& response);
        void SQLBackup(Request& request, Response& response);
        void SQLRestore(Request& request, Response& response);
        void SQLDeleteBackup(Request& request, Response& response);
        void GetSQLBackupList(Request& request, Response& response);
        void OnOpen(CTL::WebSocketInfo& info);
        void OnMessage(CTL::WebSocketInfo& info);
        void OnClose(const CTL::WebSocketInfo& info);
        void OnError(const CTL::WebSocketInfo& info);
        //--------------------------------------------------------------------------------------------------------------
    };
}



#endif
