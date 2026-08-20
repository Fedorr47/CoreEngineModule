#pragma once

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "FakeMeshIO.h"
#include "FakeRHI.h"
#include "FakeTextureIO.h"

import core;

namespace rendern::test
{
    class LevelInstantiateHarness
    {
    public:
        LevelInstantiateHarness()
            : textureIO_(MakeIO(textureDecoder_, textureUploader_, jobSystem_, renderQueue_))
            , meshIO_(MakeMeshIO(device_, jobSystem_, renderQueue_, meshLoader_, meshUploader_))
            , assets_(textureIO_, meshIO_)
            , bindless_(device_)
        {
        }

        [[nodiscard]] LevelInstance Instantiate(const LevelAsset& level)
        {
            return InstantiateLevel(scene_, assets_, bindless_, level, mathUtils::Mat4(1.0f));
        }
        
        [[nodiscard]] const Scene& GetScene() const noexcept
        {
            return scene_;
        }
        
        [[nodiscard]] Scene& GetScene() noexcept
        {
            return scene_;
        }

        [[nodiscard]] std::string InstantiateAndCaptureRuntimeError(const LevelAsset& level)
        {
            try
            {
                [[maybe_unused]] LevelInstance instance = Instantiate(level);
            }
            catch (const std::runtime_error& ex)
            {
                return ex.what();
            }

            return {};
        }

        void ExpectInstantiateThrowsWithFragments(
            const LevelAsset& level,
            std::initializer_list<std::string_view> fragments)
        {
            const std::string error = InstantiateAndCaptureRuntimeError(level);
            ASSERT_FALSE(error.empty()) << "Expected InstantiateLevel to throw std::runtime_error";

            for (const std::string_view fragment : fragments)
            {
                EXPECT_NE(error.find(fragment), std::string::npos)
                    << "missing fragment: '" << fragment << "' in error: " << error;
            }
        }

    private:
        FakeRHIDevice device_{};
        FakeTextureDecoder textureDecoder_{};
        FakeTextureUploader textureUploader_{};
        FakeMeshLoader meshLoader_{};
        FakeMeshUploader meshUploader_{};
        FakeJobSystem jobSystem_{};
        FakeRenderQueue renderQueue_{};

        TextureIO textureIO_;
        MeshIO meshIO_;
        AssetManager assets_;
        BindlessTable bindless_;
        Scene scene_{};
    };
}