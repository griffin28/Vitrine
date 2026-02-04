#include "VulkanRenderer.h"

#include <QGuiApplication>
#include <QVulkanWindow>

#include <gtest/gtest.h>

class TestVulkanRenderer : public testing::Test {
protected:
	TestVulkanRenderer() = default;
	virtual ~TestVulkanRenderer() = default;

	void SetUp() override {
		static int argc = 1;
		static char appName[] = "unit_tests";
		static char* argv[] = { appName, nullptr };
		static QGuiApplication app(argc, argv);
	}

	void TearDown() override {
		// Code here will be called immediately after each test
		// (right before the destructor).
	}

	testing::AssertionResult createRenderer(QVulkanWindow* window) {
		myvulkan::VulkanRenderer *renderer = new myvulkan::VulkanRenderer(window);

		if (renderer == nullptr) {
			return testing::AssertionFailure() << "Failed to create VulkanRenderer";	
		}

		delete renderer;
		return testing::AssertionSuccess();
	}
};

TEST_F(TestVulkanRenderer, CanInstantiateVulkanRenderer) {
	QVulkanWindow vulkanWindow;
	auto result = createRenderer(&vulkanWindow);
	EXPECT_TRUE(result);
}
