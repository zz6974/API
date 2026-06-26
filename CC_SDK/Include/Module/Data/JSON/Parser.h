#pragma once

#include <string>
using namespace std;

#include <JSON/JSONObject.h>

namespace CTL {
    class Parser{
    public:
        Parser();
        ~Parser();
        void load(const string & str);
        JsonObject parse();
    private:
        void skip_white_space();
        char get_next_token();
        JsonObject parse_null();
        JsonObject parse_bool();
        JsonObject parse_number();
        string parse_string();
        JsonObject parse_array();
        JsonObject parse_object();
        static bool in_range(int x, int lower, int upper);
    private:
        string m_str;
        size_t m_idx;
    };
}
