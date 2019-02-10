
s32 get_partition_index(SimStorage *storage, V2 p)
{
    s32 result = -1;
	Rect2 sim_boundary = storage->state.sim_boundary;
	V2 sim_dim = get_dim(sim_boundary);
	V2 cell_dim = sim_dim / (f32)SIM_GRID_DIM;
	V2 sim_relative = p - sim_boundary.min;
	if(sim_relative.x >= 0 && sim_relative.x <= sim_dim.x &&
	   sim_relative.y >= 0 && sim_relative.y <= sim_dim.y)
	{
		u16 x_index = (u16)(sim_relative.x / cell_dim.x);
		u16 y_index = (u16)(sim_relative.y / cell_dim.y);
		result = y_index * SIM_GRID_DIM + x_index;
	}
	return result;
}

WorldPartition *get_partition(SimStorage *storage, V2 p)
{
	WorldPartition *result = 0;
	s32 partition_index = get_partition_index(storage, p);
	if(partition_index >= 0)
	{
		result = storage->sim_grid + partition_index;
	}
	return result;
}

inline s32 get_agent_count(WorldPartition *partition)
{
	s32 result = partition->one_over_last_agent_index - partition->first_agent_index;
	return result;
}

inline bool is_empty(WorldPartition *partition)
{
	bool result = get_agent_count(partition) <= 0;
	return result;
}

void flush_partitions(SimStorage *storage)
{
	for(u32 i = 0; i < array_count(storage->sim_grid); ++i)
	{
		storage->sim_grid[i].first_agent_index = 0;
		storage->sim_grid[i].one_over_last_agent_index = 0;
	}
}
