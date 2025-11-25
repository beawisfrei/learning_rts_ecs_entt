#include <gtest/gtest.h>
#include "regression_test_runner.hpp"
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// Parameterized test fixture for regression tests
class RegressionTest : public ::testing::TestWithParam<std::string> {
protected:
	void SetUp() override {
		// Test setup if needed
	}
	
	void TearDown() override {
		// Test cleanup if needed
	}
};

// Parameterized test that runs for each discovered test directory
TEST_P(RegressionTest, RunRegressionTest) {
	const std::string& testDir = GetParam();
	RegressionTestRunner runner(testDir);
	
	ASSERT_TRUE(runner.LoadTestConfig()) 
		<< "Failed to load test config for " << testDir << ": " << runner.GetLastError();
	ASSERT_TRUE(runner.RunTest()) 
		<< "Failed to run test for " << testDir << ": " << runner.GetLastError();
	
	if (runner.IsGenerateMode()) {
		ASSERT_TRUE(runner.GenerateExpected()) 
			<< "Failed to generate expected for " << testDir << ": " << runner.GetLastError();
		std::cout << "Generated expected file for " << testDir << " test" << std::endl;
	} else {
		ASSERT_TRUE(runner.CompareResults()) 
			<< "Test failed for " << testDir << ": " << runner.GetLastError();
	}
}

// Helper function to discover test directories
std::vector<std::string> DiscoverTestDirectories(const std::string& baseDir) {
	std::vector<std::string> testDirs;
	
	if (!std::filesystem::exists(baseDir)) {
		std::cerr << "Warning: Test directory does not exist: " << baseDir << std::endl;
		return testDirs;
	}
	
	for (const auto& entry : std::filesystem::directory_iterator(baseDir)) {
		if (entry.is_directory()) {
			std::string testDir = entry.path().string();
			std::string testJsonPath = testDir + "/test.json";
			
			// Only include directories that have a test.json file
			if (std::filesystem::exists(testJsonPath)) {
				testDirs.push_back(testDir);
			}
		}
	}
	
	return testDirs;
}

// Instantiate parameterized tests for all discovered test directories
INSTANTIATE_TEST_SUITE_P(
	DiscoveredTests,
	RegressionTest,
	::testing::ValuesIn(DiscoverTestDirectories("data/tests")),
	[](const ::testing::TestParamInfo<std::string>& info) {
		// Generate a test name from the directory path
		std::filesystem::path p(info.param);
		std::string testName = p.filename().string();
		// Replace any invalid characters for test names
		std::replace(testName.begin(), testName.end(), '/', '_');
		std::replace(testName.begin(), testName.end(), '\\', '_');
		return testName;
	}
);

