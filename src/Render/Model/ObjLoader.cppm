module;

#include <cstdint>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module core:obj_loader;

import :mesh;
import :file_system;

struct Key
{
	int v, vt, vn;
	friend bool operator==(const Key& a, const Key& b) { return a.v == b.v && a.vt == b.vt && a.vn == b.vn; }
};

struct KeyHash
{
	std::size_t operator()(const Key& k) const noexcept
	{
		return static_cast<std::size_t>(k.v) * 73856093u ^ static_cast<std::size_t>(k.vt) * 19349663u ^ static_cast<std::size_t>(k.vn) * 83492791u;
	}
};

inline int FixIndex(int idx, int count)
{
	if (idx > 0)
	{
		return idx - 1;
	}
	if (idx < 0)
	{
		return count + idx;
	}
	return -1;
}

inline void ParseFaceToken(std::string_view token, int& v, int& vt, int& vn)
{
	v = vt = vn = 0;

	std::string strToken(token);
	std::vector<std::string> parts;
	{
		std::string current;
		for (char character : strToken)
		{
			if (character == '/')
			{
				parts.push_back(current);
				current.clear();
			}
			else current.push_back(character);
		}
		parts.push_back(current);
	}

	if (!parts.empty() && !parts[0].empty())
	{
		v = std::stoi(parts[0]);
	}
	if (parts.size() > 1 && !parts[1].empty())
	{
		vt = std::stoi(parts[1]);
	}
	if (parts.size() > 2 && !parts[2].empty())
	{
		vn = std::stoi(parts[2]);
	}
}

export namespace rendern
{
	inline MeshCPU LoadObj(const std::filesystem::path& pathIn)
	{
		namespace fs = std::filesystem;

		fs::path path = pathIn;
		if (!path.is_absolute())
		{
			path = corefs::ResolveAsset(path);
		}

		const auto txt = FILE_UTILS::ReadAllText(path);

		std::vector<float> pos;
		std::vector<float> nor;
		std::vector<float> uv;

		MeshCPU mesh;
		std::unordered_map<Key, std::uint32_t, KeyHash> dedup;

		std::istringstream ss(txt);
		std::string line;

		while (std::getline(ss, line))
		{
			if (line.empty())
			{
				continue;
			}

			std::istringstream ls(line);
			std::string tag;
			ls >> tag;

			if (tag == "v")
			{
				float x, y, z;
				ls >> x >> y >> z;
				pos.insert(pos.end(), { x,y,z });
			}
			else if (tag == "vn")
			{
				float x, y, z; 
				ls >> x >> y >> z;
				nor.insert(nor.end(), { x,y,z });
			}
			else if (tag == "vt")
			{
				float u0, v0; 
				ls >> u0 >> v0;
				uv.insert(uv.end(), { u0,v0 });
			}
			else if (tag == "f")
			{
				std::vector<std::string> tokens;
				std::string tok;

				while (ls >> tok)
				{
					tokens.push_back(tok);
				}

				if (tokens.size() < 3)
				{
					continue;
				}

				auto emitVertex = [&](const std::string& t)->std::uint32_t
					{
						int iv, it, in;
						ParseFaceToken(t, iv, it, in);

						const int vCount = static_cast<int>(pos.size()) / 3;
						const int vtCount = static_cast<int>(uv.size()) / 2;
						const int vnCount = static_cast<int>(nor.size()) / 3;

						const int vi = FixIndex(iv, vCount);
						const int vti = FixIndex(it, vtCount);
						const int vni = FixIndex(in, vnCount);

						Key key{ vi, vti, vni };
						if (auto cachedIt = dedup.find(key); cachedIt != dedup.end())
						{
							return cachedIt->second;
						}

						VertexDesc vertexDesc{};
						vertexDesc.px = pos[vi * 3 + 0]; 
						vertexDesc.py = pos[vi * 3 + 1]; 
						vertexDesc.pz = pos[vi * 3 + 2];

						if (vni >= 0)
						{
							vertexDesc.nx = nor[vni * 3 + 0]; 
							vertexDesc.ny = nor[vni * 3 + 1]; 
							vertexDesc.nz = nor[vni * 3 + 2];
						}
						else 
						{ 
							vertexDesc.nx = 0; 
							vertexDesc.ny = 0; 
							vertexDesc.nz = 1; 
						}

						if (vti >= 0)
						{
							vertexDesc.u = uv[vti * 2 + 0]; 
							vertexDesc.v = uv[vti * 2 + 1];
						}
						else 
						{ 
							vertexDesc.u = 0; 
							vertexDesc.v = 0; 
						}

						std::uint32_t newIndex = static_cast<std::size_t>(mesh.vertices.size());
						mesh.vertices.push_back(vertexDesc);
						dedup.emplace(key, newIndex);
						return newIndex;
					};

				// fan triangulation: (0, i, i+1)
				const std::uint32_t i0 = emitVertex(tokens[0]);
				for (size_t i = 1; i + 1 < tokens.size(); ++i)
				{
					const std::uint32_t i1 = emitVertex(tokens[i]);
					const std::uint32_t i2 = emitVertex(tokens[i + 1]);
					mesh.indices.push_back(i0);
					mesh.indices.push_back(i1);
					mesh.indices.push_back(i2);
				}
			}
		}

		if (mesh.vertices.empty() || mesh.indices.empty())
		{
			throw std::runtime_error("OBJ is empty or unsupported: " + path.string());
		}

		return mesh;
	}
}