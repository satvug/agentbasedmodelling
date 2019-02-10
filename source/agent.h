
struct AgentId
{
	u32 value;
};

inline bool is_valid(AgentId id)
{
	bool result = id.value != 0;

	return result;
}

inline bool operator == (AgentId a, AgentId b)
{
	bool result = a.value == b.value;

	return result;
}

inline bool operator > (AgentId a, AgentId b)
{
	bool result = a.value > b.value;

	return result;
}

inline bool operator < (AgentId a, AgentId b)
{
	bool result = a.value < b.value;

	return result;
}

struct AgentIdHashNode
{
	AgentId id;
	u16 index;
	AgentIdHashNode *prev;
	AgentIdHashNode *next;
};

struct Agent
{
	AgentId id;
	
	V2 p;
	V2 dp;

	Waypoint current_waypoint;
	f32 desired_velocity;

	f32 relaxation_time; //the higher it is, the longer it takes for an agent to achieve the desired velocity
	f32 repulsion_coefficient;
	f32 falloff_length;
	f32 body_force_constant; //basically just stiffness
	f32 friction_coefficient;

	f32 radius;
	f32 private_zone_radius;

	f32 last_waypoint_update;

	// stats
	u32 stats_index;
	f32 lifetime;
};
