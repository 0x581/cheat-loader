#pragma once
#include <unordered_map>
#include <iostream>

constexpr auto DisableOwnerCheck = true;
class c_globals {
public:
	bool active = true;
	char key[255] = "key";
	bool login_form = true;
	bool login_loading = false;
	bool main_form = false;
	bool loading_product = false;
	bool loaded_product = false;
	bool checked_connection = true;
	bool failed = false;
	bool initialized_new_window_name = false;
	bool loaded_successfull = false;
	std::string fail_message = "";
	std::string connection_message = "";
	std::string status = "unknown";
	std::string version = "0.0.1";
	std::string last_uptade = "29.09.2023";
	int final_selected_product = 0;
	int tick_count = 0;
	int tick_count2 = 0;
};

inline c_globals globals;