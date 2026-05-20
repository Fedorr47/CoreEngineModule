#pragma once

#include <filesystem>
#include <string>
#include <string_view>

std::filesystem::path GetTestFixtureRoot();
std::filesystem::path ResolveFixturePath(std::string_view relativePath);
std::string LoadTextFixture(std::string_view relativePath);