#include "json_comparator.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>

JsonComparator::JsonComparator(float epsilon)
	: _epsilon(epsilon)
{
}

bool JsonComparator::Compare(const nlohmann::json& actual, const nlohmann::json& expected) {
	_lastError.clear();
	return compareValue(actual, expected, "");
}

bool JsonComparator::compareValue(const nlohmann::json& actual, const nlohmann::json& expected, const std::string& path) {
	// Check if types match
	if (actual.type() != expected.type()) {
		std::ostringstream oss;
		oss << "Type mismatch at '" << path << "': actual is " << actual.type_name() 
		    << ", expected is " << expected.type_name();
		_lastError = oss.str();
		return false;
	}
	
	// Handle different JSON types
	switch (actual.type()) {
		case nlohmann::json::value_t::object: {
			// Check if all keys in expected exist in actual
			for (auto it = expected.begin(); it != expected.end(); ++it) {
				std::string key = it.key();
				std::string newPath = path.empty() ? key : path + "." + key;
				
				if (!actual.contains(key)) {
					std::ostringstream oss;
					oss << "Missing key '" << key << "' at '" << path << "'";
					_lastError = oss.str();
					return false;
				}
				
				if (!compareValue(actual[key], expected[key], newPath)) {
					return false;
				}
			}
			
			// Check if there are extra keys in actual (optional - might want to allow this)
			// For now, we'll allow extra keys in actual
			break;
		}
		
		case nlohmann::json::value_t::array: {
			if (actual.size() != expected.size()) {
				std::ostringstream oss;
				oss << "Array size mismatch at '" << path << "': actual has " 
				    << actual.size() << " elements, expected has " << expected.size();
				_lastError = oss.str();
				return false;
			}
			
			for (size_t i = 0; i < actual.size(); ++i) {
				std::ostringstream oss;
				oss << path << "[" << i << "]";
				if (!compareValue(actual[i], expected[i], oss.str())) {
					return false;
				}
			}
			break;
		}
		
		case nlohmann::json::value_t::number_float:
		case nlohmann::json::value_t::number_integer:
		case nlohmann::json::value_t::number_unsigned:
			return compareNumbers(actual, expected, path);
		
		case nlohmann::json::value_t::string:
		case nlohmann::json::value_t::boolean:
		case nlohmann::json::value_t::null:
			if (actual != expected) {
				std::ostringstream oss;
				oss << "Value mismatch at '" << path << "': actual is " 
				    << actual << ", expected is " << expected;
				_lastError = oss.str();
				return false;
			}
			break;
		
		default:
			// For any other type, do exact comparison
			if (actual != expected) {
				std::ostringstream oss;
				oss << "Value mismatch at '" << path << "': actual is " 
				    << actual << ", expected is " << expected;
				_lastError = oss.str();
				return false;
			}
			break;
	}
	
	return true;
}

bool JsonComparator::compareNumbers(const nlohmann::json& actual, const nlohmann::json& expected, const std::string& path) {
	double actualVal = actual.get<double>();
	double expectedVal = expected.get<double>();
	
	double diff = std::abs(actualVal - expectedVal);
	
	// Use relative tolerance for large numbers, absolute for small numbers
	double tolerance = _epsilon;
	if (std::abs(expectedVal) > 1.0) {
		tolerance = std::abs(expectedVal) * _epsilon;
	}
	
	if (diff > tolerance) {
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(9);
		oss << "Numeric mismatch at '" << path << "': actual is " << actualVal 
		    << ", expected is " << expectedVal << " (diff: " << diff << ", tolerance: " << tolerance << ")";
		_lastError = oss.str();
		return false;
	}
	
	return true;
}

