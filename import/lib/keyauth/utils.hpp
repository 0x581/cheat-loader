#pragma once
#include <filesystem> 
#include <string> 
#include <fstream>

#include "json.hpp"

#include "skStr.h"
#include "auth.hpp"

using json = nlohmann::json;

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

void checkAuthenticated(std::string ownerid) {
	if (DisableOwnerCheck) { return; }

	while (true) {
		if (GlobalFindAtom(ownerid.c_str()) == 0) {
			exit(13);
		}
		Sleep(1000); // thread interval
	}
}

void sessionStatus(KeyAuth::api& KeyAuthApp) {
	KeyAuthApp.check(true); // do NOT specify true usually, it is slower and will get you blocked from API
	if (!KeyAuthApp.response.success) {
		exit(0);
	}

	if (KeyAuthApp.response.isPaid) {
		while (true) {
			Sleep(20000); // this MUST be included or else you get blocked from API
			KeyAuthApp.check();
			if (!KeyAuthApp.response.success) {
				exit(0);
			}
		}
	}
}
