#pragma once
#include <unordered_map>
#include <iostream>

constexpr auto DisableOwnerCheck = true;

enum ui_status { login, login_loading, main, loading_product, loaded_product, failed };

class c_globals {
public:
	ui_status form_state;
	bool active = true;
	char key[255]{ '\0' };
	bool initialized_new_window_name = false;
	bool loaded_successfull = false;
	std::string fail_message = "";
	std::string connection_message = "";
	std::string status = "unknown";
	std::string version = "0.0.1";
	std::string last_update = "29.09.2023";
	int final_selected_product = 0;
	int tick_count = 0;
	int tick_count2 = 0;
};

inline c_globals globals;