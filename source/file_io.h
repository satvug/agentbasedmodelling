
enum Specifier
{
	specifier_none,
	
	specifier_sim_boundary,
	specifier_obstacle_lines,
	specifier_obstacle_circles,
	specifier_obstacle_rects,
	specifier_targets,
	specifier_sink_areas,
	specifier_waypoints,
	specifier_agents,
	specifier_agent_positions,
	specifier_paths,
	
	specifier_count,
};

// NOTE: used only during custom path parsing
struct Path
{
	s16 target;
	s16 index_from;
	s16 index_to;
};

enum _TokenType
{
	token_type_unknown = 0,
	
	token_type_specifier,
	token_type_number,

	token_type_end_of_string,
};

struct Token
{
	_TokenType type;

	String string;
};

// Not a big fan of this
// Allows to output more meaningful error messages though.
enum StatusCode
{
	status_code_none,
	
	status_code_success,
	status_code_invalid_path,
	status_code_unknown_specifier,
};

struct Tokenizer
{
	String string;
	u32 offset;
};
