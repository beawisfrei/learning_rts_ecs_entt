#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "json_comparator.hpp"

struct TestParams {
	std::string inputFile;      // e.g., "input.json"
	std::string expectedFile;   // e.g., "expected.json"
	int iterations;             // number of simulation steps
	float deltaTime;            // fixed dt per step (e.g., 0.01666f for 60 FPS)
};

class RegressionTestRunner {
public:
	RegressionTestRunner(const std::string& testDir);
	
	// Load test configuration from test_config.json
	bool LoadTestConfig();
	
	// Run the simulation
	bool RunTest();
	
	// Compare results (normal mode)
	bool CompareResults();
	
	// Generate expected file (generation mode)
	bool GenerateExpected();
	
	// Check if we're in generation mode
	bool IsGenerateMode();
	
	// Get last error message
	const std::string& GetLastError() const { return _lastError; }
	
private:
	std::string _testDir;
	TestParams _params;
	bool _testRun;
	std::string _lastError;
	std::string _outputPath;
	bool _generateMode;
	
	bool loadWorldConfig(nlohmann::json& config);
};

