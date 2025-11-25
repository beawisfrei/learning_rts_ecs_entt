#include <gtest/gtest.h>
#include <iostream>
#include "utils/resource_loader.hpp"

TEST(SanityCheck, TrueIsTrue) {
    EXPECT_TRUE(true);
}

int main(int argc, char **argv) {
	// Set working directory to project root (where data folder is located)
	if (!ResourceLoader::SetDataDirectory()) {
		std::cerr << "Warning: Could not find data directory. Tests may fail to load config files." << std::endl;
	}
	
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

