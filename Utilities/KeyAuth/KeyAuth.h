#pragma once
#include "auth.hpp"
#include "skStr.h"
#include <string>
#include <vector>

class KeyManager {
public:
    bool init();
    bool login_with_license(std::string key);
    bool login_with_coords(std::string user, std::string pass);
    bool register_user(std::string user, std::string pass, std::string key);
    std::string get_expiry();

    bool auto_login();

private:
    
};

std::string tm_to_readable_time(tm ctx);
static std::time_t string_to_timet(std::string timestamp);
static std::tm timet_to_tm(time_t timestamp);
// json (autologin)
std::string ReadFromJson(std::string path, std::string section);
bool CheckIfJsonKeyExists(std::string path, std::string section);
bool WriteToJson(std::string path, std::string name, std::string value, bool userpass, std::string name2, std::string value2);