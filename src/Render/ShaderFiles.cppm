module;

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <stdexcept>

export module core:shader_files;

import :rhi;

struct TextFile
{
	std::string text;
	std::vector<std::filesystem::path> dpendencies;
};

static std::string ReadAllText(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::in);
	if (!file)
	{
		throw std::runtime_error("Filed to open file:" + path.string());
	}

	std::ostringstream stringStream;
	stringStream << file.rdbuf();
	return stringStream.str();
}

static bool StartsWithTrimmed(std::string_view line, std::string_view token)
{
	size_t idx = line.find_first_not_of("\t");
	if (idx == std::string_view::npos)
	{
		return false;
	}
	return line.substr(idx).starts_with(token);
}

static void ExpandGLSL(
	const std::filesystem::path& file,
	std::unordered_set<std::filesystem::path>& includeStack,
	std::vector<std::filesystem::path>& dependencies,
	std::string& output,
	bool isRoot
)
{
	auto absPath = std::filesystem::weakly_canonical(file);
	if (includeStack.contains(absPath))
	{
		throw std::runtime_error("Recursive #include detected: " + absPath.string());
	}

	includeStack.insert(absPath);
	dependencies.push_back(absPath);

	const std::string source = ReadAllText(absPath);
	std::istringstream inputStream(source);

	std::string line;
	while (std::getline(inputStream, line))
	{
		if (!isRoot && StartsWithTrimmed(line, "#version"))
		{
			output += "// (stripped) " + line + "\n";
			continue;
		}

		if (StartsWithTrimmed(line, "#include"))
		{
			const auto quote0 = line.find('"');
			const bool isQ0npos = (quote0 == std::string::npos);
			const auto quote1 = isQ0npos ? std::string::npos : line.find('"', quote0 + 1);
			if (isQ0npos || quote1 == std::string::npos || quote1 <= quote0 + 1)
			{
				throw std::runtime_error("Invalid #include syntax in: " + absPath.string());
			}

			const auto relatedName = line.substr(quote0 + 1, quote1 - (quote0 + 1));
			const auto includeName = absPath.parent_path() / std::filesystem::path(relatedName);

			output += "\n// --- begin include: " + includeName.string() + "\n";
			ExpandGLSL(includeName, includeStack, dependencies, output, false);
			output += "// --- end include: " + includeName.string() + "\n\n";
		}
		else
		{
			output += line;
			output += "\n";
		}
	}

	includeStack.erase(absPath);
}

export namespace rendern
{
	export TextFile LoadTextFile(const std::filesystem::path& path)
	{
		TextFile outputFile;
		outputFile.text = ReadAllText(path);
		outputFile.dpendencies = { std::filesystem::weakly_canonical(path) };
		return outputFile;
	}

	// Loads GLSL from file and expands `#include "..."` recursively.
	// - Included paths are resolved relative to the including file.
	// - Nested files must NOT contain `#version` (it will be stripped to avoid GLSL errors).
	export TextFile LoadGLSLWithIncludes(const std::filesystem::path& path)
	{
		TextFile outputFile;
		std::unordered_set<std::filesystem::path> stack;
		ExpandGLSL(path, stack, outputFile.dpendencies, outputFile.text, true);
		return outputFile;
	}

	// Inserts `#define ...` lines after `#version` (if present), otherwise prepends them.
	// Accepts entries like: "FOO", "USE_FOG=1" (the first '=' will be replaced by space).
	export std::string AppplyDefinesToGLSL(
		std::string_view source, const std::vector<std::string>& defines)
	{
		if (defines.empty())
		{
			return std::string(source);
		}

		auto makeDefineLine = [](std::string defineStr) -> std::string
			{
				auto equation = defineStr.find('=');
				if (equation != std::string::npos)
				{
					defineStr[equation] = ' ';
				}
				return std::string("define ") + defineStr + "\n";
			};

		std::string definesBlock;
		definesBlock.reserve(defines.size() * 24);
		for (const auto& defineStr : defines)
		{
			definesBlock += makeDefineLine(defineStr);
		}

		// Find a version line
		const std::string_view verToken = "#version";
		const auto verPosition = source.find(verToken);
		if (verPosition != std::string_view::npos)
		{
			const auto endOfLine = source.find('\n', verPosition);
			if (endOfLine != std::string_view::npos)
			{
				std::string output;
				output.reserve(source.size() + definesBlock.size() + 1);
				output.append(source.substr(0, endOfLine + 1));
				output.append(definesBlock);
				output.append(source.substr(endOfLine + 1));
				return output;
			}
		}

		return definesBlock + std::string(source);
	}
}

