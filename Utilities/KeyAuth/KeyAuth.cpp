#include "KeyAuth.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "../../nlohmann/json.hpp"

using json = nlohmann::json;

using namespace KeyAuth;

std::tm timet_to_tm(std::time_t timestamp) {
    std::tm time_struct;
    gmtime_s(&time_struct, &timestamp);
    return time_struct;
}

std::time_t string_to_timet(std::string timestamp) {
    return static_cast<std::time_t>(std::stoll(timestamp));
}

std::string tm_to_readable_time(std::tm time_struct) {
    std::stringstream ss;
    ss << std::put_time(&time_struct, "%m/%d/%Y %I:%M %p");
    return ss.str();
}

std::string ReadFromJson(std::string path, std::string section)
{
	if (!std::filesystem::exists(path))
		return skCrypt("File Not Found").decrypt();
	std::ifstream file(path);
	json data = json::parse(file);
	return data[section];
}

bool CheckIfJsonKeyExists(std::string path, std::string section)
{
	if (!std::filesystem::exists(path))
		return skCrypt("File Not Found").decrypt();
	std::ifstream file(path);
	json data = json::parse(file);
	return data.contains(section);
}

bool WriteToJson(std::string path, std::string name, std::string value, bool userpass, std::string name2, std::string value2)
{
	json file;
	if (!userpass)
	{
		file[name] = value;
	}
	else
	{
		file[name] = value;
		file[name2] = value2;
	}

	std::ofstream jsonfile(path, std::ios::out);
	jsonfile << file;
	jsonfile.close();
	if (!std::filesystem::exists(path))
		return false;

	return true;
}

std::string name = skCrypt("ApplicationID").decrypt();
std::string ownerid = skCrypt("OwnerID").decrypt();
std::string version = skCrypt("1.0").decrypt();
std::string url = skCrypt("https://keyauth.win/api/1.3/").decrypt();
std::string path = skCrypt("").decrypt();

api KeyAuthApp(name, ownerid, version, url, path);

bool KeyManager::init() {
    KeyAuthApp.init();
    return KeyAuthApp.response.success;
}

bool KeyManager::login_with_license(std::string key) {
    KeyAuthApp.license(key);
    return KeyAuthApp.response.success;
}

bool KeyManager::login_with_coords(std::string user, std::string pass) {
    KeyAuthApp.login(user, pass, "");
    return KeyAuthApp.response.success;
}

bool KeyManager::register_user(std::string user, std::string pass, std::string key) {
    KeyAuthApp.regstr(user, pass, key);

    if (KeyAuthApp.response.success) {
        WriteToJson("DONTSHAREWITHANYONE.json", "username", user, true, "password", pass);
        return true;
    }
    else {
        return false;
    }
}

std::string KeyManager::get_expiry() {
    if (!KeyAuthApp.user_data.subscriptions.empty()) {
        auto expiry = KeyAuthApp.user_data.subscriptions[0].expiry;
        return tm_to_readable_time(timet_to_tm(string_to_timet(expiry)));
    }
    return "No active subscription";
}

bool KeyManager::auto_login() {
    if (!std::filesystem::exists("DONTSHAREWITHANYONE.json")) {
        return false;
    }

    try {
        std::ifstream json("DONTSHAREWITHANYONE.json");
        nlohmann::json data;
        json >> data;

        if (data.contains("license")) {
            std::string jlicense = data["license"];
            return this->login_with_license(jlicense);
        }
        else if (data.contains("username") && data.contains("password")) {
            std::string user = data["username"];
            std::string pass = data["password"];
            return this->login_with_coords(user, pass);
        }
    }
    catch (...) {
        return false;
    }

    return false;
}