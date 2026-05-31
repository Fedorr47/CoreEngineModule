#include <gtest/gtest.h>

#include <cstddef>
#include <type_traits>

import core;

namespace
{
	// CPU matrices/vectors are directly copied into GPU vertex data and constant payload rows.
	static_assert(std::is_standard_layout_v<mathUtils::Vec4>);
	static_assert(std::is_trivially_copyable_v<mathUtils::Vec4>);
	static_assert(alignof(mathUtils::Vec4) == 16);
	static_assert(sizeof(mathUtils::Vec4) == 16);
	static_assert(offsetof(mathUtils::Vec4, x) == 0);
	static_assert(offsetof(mathUtils::Vec4, y) == 4);
	static_assert(offsetof(mathUtils::Vec4, z) == 8);
	static_assert(offsetof(mathUtils::Vec4, w) == 12);

	static_assert(std::is_standard_layout_v<mathUtils::Mat4>);
	static_assert(std::is_trivially_copyable_v<mathUtils::Mat4>);
	static_assert(alignof(mathUtils::Mat4) == 16);
	static_assert(sizeof(mathUtils::Mat4) == 64);
	static_assert(offsetof(mathUtils::Mat4, columns) == 0);
}

TEST(GpuStructPacking, CompileTimeContractsAreLinked)
{
	// The high-value GPU payload contracts are compile-time assertions; this keeps the file discoverable.
	SUCCEED();
}

#if defined(CORE_USE_DX12)
TEST(GpuStructPacking, Dx12ShadowArrayCapsMatchShaderSlots)
{
	// DeferredLighting_dx12.hlsl declares exactly four spot and four point shadow resources.
	EXPECT_EQ(rendern::kMaxSpotShadows, 4u);
	EXPECT_EQ(rendern::kMaxPointShadows, 4u);
}
#endif