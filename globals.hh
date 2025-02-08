#pragma once

#include <unordered_map>
#include <iostream>
#include "keyauth/skStr.h"

constexpr auto DisableOwnerCheck = true;

enum ui_status { login, login_loading, main, loading_product, loaded_product, failed };

enum ProductStatus {detected, undetected, testing};


struct ProductInfo
{
	bool valid = false;
	std::string name;
	ProductStatus status;
	std::string version;
	std::string last_update;
	std::string key;
};

class c_globals {
public:
	bool active = true;
	ui_status form_state;
	std::string fail_message = "";

	char key[255]{ 0 };

	ProductInfo products[16];
	int selectedProduct = 0;

	int final_selected_product = 0;
};

inline c_globals globals;