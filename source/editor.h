
enum EditMode
{
	edit_mode_none,
	edit_mode_agent_placement,
	edit_mode_obstacle_placement,
	edit_mode_waypoint_placement, // including targets
};

struct ProfessionalQualityUIPanel
{
	V2 dim_from;
	V2 dim_to;
	
	V2 p_from;
	V2 p_to;

	V4 color_from;
	V4 color_to;
	
	f32 time_to_open;

	f32 default_openness;
	f32 min_openness;
	f32 max_openness;
	f32 target_openness;
	f32 current_openness;
};

struct SimEditor
{
	EditMode last_edit_mode;	
	EditMode current_edit_mode;	
    Camera2 debug_camera;

	bool is_drawing_line; // looks lazy but should work
	V2 current_line_p0;
	V2 world_anchor;

	s16 selected_mid_waypoint_index;
	
	f32 snapping_grid_dim; // percentage of the current sim region height
	f32 snapping_range_percentage;
	f32 vertical_apron_percentage;
	f32 camera_sensitivity;

	ProfessionalQualityUIPanel panel;
};
