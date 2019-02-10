
#define color_black   v4(0.0f, 0.0f, 0.0f, 1.0f)
#define color_white   v4(1.0f, 1.0f, 1.0f, 1.0f)
#define color_red     v4(1.0f, 0.0f, 0.0f, 1.0f)
#define color_green   v4(0.0f, 1.0f, 0.0f, 1.0f)
#define color_blue    v4(0.0f, 0.0f, 1.0f, 1.0f)
#define color_yellow  v4(1.0f, 1.0f, 0.0f, 1.0f)
#define color_purple  v4(1.0f, 0.0f, 1.0f, 1.0f)
#define color_cyan    v4(0.0f, 1.0f, 1.0f, 1.0f)
#define color_orange  v4(1.0f, 0.5f, 0.0f, 1.0f)

// Each pixel is 4 bytes long and has the Little Endian format:
// |AA|BB|GG|RR| -> |RR|GG|BB|AA|
#define BYTES_PER_PIXEL 4
#define RED_MASK    0x000000ff
#define GREEN_MASK  0x0000ff00
#define BLUE_MASK   0x00ff0000
#define ALPHA_MASK  0xff000000
#define RED_SHIFT   0
#define GREEN_SHIFT 8
#define BLUE_SHIFT  16
#define ALPHA_SHIFT 24
struct Bitmap
{
    u32 *pixels;
    u32 width;
    u32 height;

    u32 gl_handle;

    //
    // Test
    V2 min_uv;
    V2 max_uv;
};

// camera's p corresponds to its _center_ position
struct Camera2
{
    V2 p;
    V2 view_dim;
};

struct RenderBatch
{
	union
	{
		struct
		{
			union
			{
				struct
				{
					u8 texture_handle_index;
					u8 shader_handle_index;
				};
				u16 asset_id;
			};
			u16 priority;
		};
		u32 sorting_key;
	};
	union
	{
		u32 offset;
		u32 vertex_count;
	};
};

struct Vertex
{
    V2 p;
    V2 uv;
    u32 c;
    f32 smoothing_uv;
	f32 thickness; // ignored when drawing quads/lines
};

struct GLInfo
{
    // Uniforms
	GLuint sampler;
    GLuint transform;
	//GLuint shifts;

	// Attrib locations
	GLuint p_attrib;
	GLuint uv_attrib;
	GLuint c_attrib;
	GLuint smoothing_uv_attrib;
	GLuint thickness_attrib;
};

struct FontGlyph
{
	u8 texture_handle_index;
	
	V2 uv_min;
	V2 uv_max;

	V2 pixel_dim;
	V2 offset;

	f32 x_advance;
};

struct Font
{
	// NOTE: in pixels
	f32 height;
	f32 line_step;

	u8 first_character;
	u8 last_character;
	
	FontGlyph glyphs[0x80];

	u32 atlas_resolution;

	//
	// TODO? kerning table
};

struct FontFooter
{
	// NOTE: from the beginning of the file
	u32 font_offset;
};

struct Assets
{
	Font debug_font;
	
	u8 texture_count;
	u32 texture_handles[0xff];
	u8 shader_count;
	u32 shader_handles[0xff];
};

enum VerticalAlignment
{
	vertical_alignment_top,
	vertical_alignment_center,
	vertical_alignment_bottom,
};

enum HorizontalAlignment
{
	horizontal_alignment_left,
	horizontal_alignment_center,
	horizontal_alignment_right,
};

struct RenderInfo
{
	MemoryBlock memory;
	V4 clear_color;
	
    Camera2 *current_camera;
	V2 target_camera_p;
	V2 target_camera_view_dim;
    
    u32 vertex_count;
    Vertex *vertex_array;
	RenderBatch *batch_array;

    V2 window_viewport_min;
    V2 window_viewport_dim;
    
    u32 framebuffer_handle;
    Bitmap framebuffer_color_texture;

	// NOTE: fraction of the current view height
	f32 smoothing_distance_coefficient;
	
	// OpenGL-specific
    GLInfo gl_info;

	u8 current_texture_handle_index;
	u8 current_shader_handle_index;

	u8 line_shader_index;
	u8 texture_shader_index;
	u8 test_texture_index;
	
	Assets assets;
};
