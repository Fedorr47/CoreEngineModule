namespace
{
	struct AliveNodeRemap_
	{
		std::vector<int> oldToNew;
		std::vector<int> newToOld;
	};

	[[nodiscard]] inline AliveNodeRemap_ BuildAliveNodeRemap_(const LevelAsset& level)
	{
		AliveNodeRemap_ remap;
		remap.oldToNew.assign(level.nodes.size(), -1);
		remap.newToOld.reserve(level.nodes.size());
		for (std::size_t i = 0; i < level.nodes.size(); ++i)
		{
			if (!level.nodes[i].alive)
			{
				continue;
			}
			remap.oldToNew[i] = static_cast<int>(remap.newToOld.size());
			remap.newToOld.push_back(static_cast<int>(i));
		}
		return remap;
	}
}
