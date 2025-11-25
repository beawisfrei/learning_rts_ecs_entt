#include "regression_test_runner.hpp"
#include "world/world.hpp"
#include "utils/resource_loader.hpp"
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <sstream>

RegressionTestRunner::RegressionTestRunner(const std::string& testDir)
	: _testDir(testDir)
	, _testRun(false)
{
}

bool RegressionTestRunner::IsGenerateMode() {
	return _generateMode;
}

bool RegressionTestRunner::LoadTestConfig() {
	// Load test_config.json (test parameters)
	std::string testConfigPath = _testDir + "/test.json";
	std::ifstream file(testConfigPath);
	if (!file.is_open()) {
		_lastError = "Failed to open test.json: " + testConfigPath;
		return false;
	}
	
	nlohmann::json testConfig;
	try {
		file >> testConfig;
	} catch (const std::exception& e) {
		_lastError = "JSON parse error in test.json: " + std::string(e.what());
		return false;
	}
	
	// Extract test parameters
	_params.inputFile = testConfig.value("input_file", "input.json");
	_params.expectedFile = testConfig.value("expected_file", "expected.json");
	_params.iterations = testConfig.value("iterations", 100);
	_params.deltaTime = testConfig.value("delta_time", 0.01666f);
	
	return true;
}

bool RegressionTestRunner::loadWorldConfig(nlohmann::json& config) {
	// Load test_config.json for world initialization (unit stats, etc.)
	// This is a copy of config.json but for tests
	std::string worldConfigPath = "data/test_config.json";
	
	if (!ResourceLoader::load_config(worldConfigPath, config)) {
		_lastError = "Failed to load world config: " + worldConfigPath;
		return false;
	}

	// Generate expected files if generate_expected is set to 1
	_generateMode = config["global"].value("generate_expected", 0) == 1;
	
	return true;
}

bool RegressionTestRunner::RunTest() {
	// Load world configuration
	nlohmann::json worldConfig;
	if (!loadWorldConfig(worldConfig)) {
		return false;
	}
	
	// Create and initialize world
	World world;
	if (!world.Initialize(worldConfig, false)) {
		_lastError = "Failed to initialize world";
		return false;
	}
	
	// Load input world state
	std::string inputPath = _testDir + "/" + _params.inputFile;
	if (!world.LoadGame(inputPath)) {
		_lastError = "Failed to load input world state: " + inputPath;
		return false;
	}
	
	// Run simulation for specified iterations
	for (int i = 0; i < _params.iterations; ++i) {
		world.Update(_params.deltaTime);
	}
	
	// Determine output path
	if (IsGenerateMode()) {
		// In generate mode, save to expected file location
		_outputPath = _testDir + "/" + _params.expectedFile;
	} else {
		// In normal mode, save to temp file in build directory
		// Use a unique temp filename based on test directory name
		std::filesystem::path testPath(_testDir);
		std::string testName = testPath.filename().string();
		_outputPath = "test_output_" + testName + ".json";
	}
	
	// Save result
	if (!world.SaveGame(_outputPath)) {
		_lastError = "Failed to save output world state: " + _outputPath;
		return false;
	}
	
	_testRun = true;
	return true;
}

bool RegressionTestRunner::CompareResults() {
	if (!_testRun) {
		_lastError = "Test has not been run yet";
		return false;
	}
	
	// Load expected result
	std::string expectedPath = _testDir + "/" + _params.expectedFile;
	nlohmann::json expected;
	{
		std::ifstream expectedFile(expectedPath);
		if (!expectedFile.is_open()) {
			_lastError = "Failed to open expected file: " + expectedPath;
			return false;
		}
		
		try {
			expectedFile >> expected;
		} catch (const std::exception& e) {
			_lastError = "JSON parse error in expected file: " + std::string(e.what());
			return false;
		}
	} // expectedFile closes here
	
	// Load actual result
	nlohmann::json actual;
	{
		std::ifstream actualFile(_outputPath);
		if (!actualFile.is_open()) {
			_lastError = "Failed to open actual output file: " + _outputPath;
			return false;
		}
		
		try {
			actualFile >> actual;
		} catch (const std::exception& e) {
			_lastError = "JSON parse error in actual output file: " + std::string(e.what());
			return false;
		}
	} // actualFile closes here
	
	// Compare using JSON comparator
	JsonComparator comparator;
	if (!comparator.Compare(actual, expected)) {
		_lastError = "Comparison failed: " + comparator.GetLastError();
		return false;
	}
	
	// Clean up temp file on success (file is now closed)
	if (!IsGenerateMode()) {
		std::filesystem::remove(_outputPath);
	}
	
	return true;
}

bool RegressionTestRunner::GenerateExpected() {
	if (!_testRun) {
		_lastError = "Test has not been run yet";
		return false;
	}
	
	// In generate mode, the output was already saved to expected file location
	// Just verify it exists
	if (!std::filesystem::exists(_outputPath)) {
		_lastError = "Generated expected file does not exist: " + _outputPath;
		return false;
	}
	
	return true;
}

