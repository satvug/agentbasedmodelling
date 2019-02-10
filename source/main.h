
enum SimMode
{
	sim_mode_sim,
	sim_mode_editor,
};

enum RenderingPriorities
{
	rendering_priority_sim_background = 0,
	rendering_priority_sim_base,
	rendering_priority_sim_overlay = 32,
	rendering_priority_ui_base = 128,
};

// ?
// List of frames each containing agent stats
// vs
// List of agent stats each containing frame info

struct AgentFrameNode
{
	V2 p;
	f32 t;

	// from the beginning of the stats block
	AgentFrameNode *next;
};

struct AgentStats
{
	f32 lifetime;

	// okay, I give up.
	//AgentFrameNode frames_sentinel;
	AgentFrameNode front_sentinel;
	AgentFrameNode back_sentinel;
};

struct RunStats
{
	AgentParams params;
	f32 run_time;
	u32 agent_count;
	//AgentStats agent_records[128];
};

struct Run
{
	u32 repeat_count;
	u32 agent_count;
	AgentParams params;
};

struct SimStorage
{
	ProfessionalQualityConsole console;
	
	SimMode mode;
	
	AgentId last_agent_id;
	AgentIdHashNode *first_free_node;
	AgentIdHashNode *agent_id_hash_table;

	u16 available_position_count;
	V2 *available_positions;
	
    u16 agent_count;
    Agent *agents;
	
    u16 obstacle_count;
    Obstacle *obstacles;

	u16 intermediate_waypoint_count;
	u16 target_waypoint_count;
	V2 *waypoints;
	u16 sink_rect_count;
	Rect2 *sink_rects;
	s16 **shortest_path_indices;

	u32 run_count;
	Run *runs;

	RNG rng;
	
	// NOTE: One additional partition for the people outside the sim boundary.
	// Debug only (?) An agent outside of the sim boundary should be deleted
	WorldPartition sim_grid[SIM_GRID_DIM * SIM_GRID_DIM + 1];

	// memory blocks
    MemoryBlock persistent_memory;
    MemoryBlock scratch_memory;
    MemoryBlock stats_memory;

	//
	// debug things
	AgentId selected_agent;
	s16 selected_target_index;

    SimState state;
    SimEditor editor;

	//f32 *stats;
	u32 current_run;
	RunStats *stats;
	
	char last_successful_load_path_buffer[256];
	String last_successful_load_path;
	u32 frame_counter;
};

void flush_storage(SimStorage *storage);

static bool show_debug_overlay = true;
