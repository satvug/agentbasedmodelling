
// NOTE: Dijkstra's algorithm acceleration structure
struct HeapEntry
{
    f32 key;
    s16 index;
};

struct Heap
{
    s32 size;
    HeapEntry *entries;
};

struct AgentParams
{
	f32 desired_velocity;
	f32 relaxation_time;
	f32 radius;
	f32 private_zone_radius;
	f32 repulsion_coefficient;
	f32 falloff_length;
	f32 body_force_constant;
	f32 friction_coefficient;
	f32 random_force;
	f32 waypoint_update_distance;
	f32 waypoint_update_interval;
	f32 collision_avoidance;
};

struct SimState
{
	Camera2 sim_camera;
	Rect2 sim_boundary;

	bool paused;
	bool real_time_sim;
	
	s32 current_run_index;
	u32 current_run_repeat_count;

	u32 last_requested_spawn_count;
	u32 remaining_reloads;
	f32 simulation_time;

	// Agent params
	AgentParams params;
};

enum Visibility
{
	visibility_none,
	visibility_direct,
	visibility_left,
	visibility_right,
};

struct Waypoint
{
	V2 p;
	Visibility visibility;
};
