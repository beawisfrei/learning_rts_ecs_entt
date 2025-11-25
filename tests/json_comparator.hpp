#pragma once

#include <nlohmann/json.hpp>
#include <string>

class JsonComparator {
public:
	JsonComparator(float epsilon = 1e-4f);
	
	// Compare two JSON objects with tolerance for floating-point values
	bool Compare(const nlohmann::json& actual, const nlohmann::json& expected);
	
	// Get the last error message
	const std::string& GetLastError() const { return _lastError; }
	
private:
	float _epsilon;
	std::string _lastError;
	
	bool compareValue(const nlohmann::json& actual, const nlohmann::json& expected, const std::string& path);
	bool compareNumbers(const nlohmann::json& actual, const nlohmann::json& expected, const std::string& path);
};

