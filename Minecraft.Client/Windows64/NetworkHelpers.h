#pragma once
#include <string>
#include <functional>
#include "json.hpp"

using Json = nlohmann::json;

bool HttpPost(const char* url, const Json& payload);
bool HttpGet(const char* url,
             std::function<void(const Json&)> onSuccess,
             std::function<void()> onError = nullptr);
