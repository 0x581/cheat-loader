#pragma once

#include <unordered_map>
#include <iostream>
#include "keyauth/skStr.h"

constexpr auto DisableOwnerCheck = true;

enum ui_status { login, main, loading_product, loaded_product, failed };

enum ProductStatus { invalid, detected, undetected, testing};


struct ProductInfo
{
	bool valid = false;
	std::string name{ 0 };
	ProductStatus status{ invalid };
	std::string version{ 0 };
	std::string last_update{ 0 };
	std::string key{ 0 };
};

class c_globals {
public:
	bool active = true;
	ui_status form_state;
	std::string error_message = "";

	char key[255]{ 0 };

	ProductInfo products[16];
	int selectedProduct = 0;

	int final_selected_product = 0;
};

inline c_globals globals;