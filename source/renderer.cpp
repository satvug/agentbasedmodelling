
inline f32 get_width_over_height(Camera2 *camera)
{
    f32 result = camera->view_dim.x / camera->view_dim.y;
    return result;
}

inline f32 get_width_over_height(Bitmap *bitmap)
{
    f32 result = bitmap->width / (f32)bitmap->height;
    return result;
}

inline Rect2 get_camera_rect(Camera2 *camera)
{
	Rect2 result = center_dim_rect(camera->p, camera->view_dim);
	return result;
}

V2 unproject(V2 screenspace_p, RenderInfo *info)
{
    V2 viewport_min = info->window_viewport_min;
    V2 viewport_dim = info->window_viewport_dim;
    screenspace_p -= viewport_min;
    screenspace_p.x /= viewport_dim.x;
    screenspace_p.y /= viewport_dim.y;

	Rect2 camera_rect = get_camera_rect(info->current_camera);
	V2 result = camera_rect.min + screenspace_p * info->current_camera->view_dim;
	return result;
}

inline u32 pack_color(V4 color_unpacked)
{
    u32 result = (u32)(255.0f * color_unpacked.r) << RED_SHIFT   |
                 (u32)(255.0f * color_unpacked.g) << GREEN_SHIFT |
                 (u32)(255.0f * color_unpacked.b) << BLUE_SHIFT  |
                 (u32)(255.0f * color_unpacked.a) << ALPHA_SHIFT ;
    return result;
}

inline V4 unpack_color(u32 color_packed)
{
    V4 result = (1 / 255.0f) * v4((f32)((color_packed & RED_MASK) >> RED_SHIFT),
                                  (f32)((color_packed & GREEN_MASK) >> GREEN_SHIFT),
                                  (f32)((color_packed & BLUE_MASK) >> BLUE_SHIFT),
                                  (f32)((color_packed & ALPHA_MASK) >> ALPHA_SHIFT));
    return result;
}

inline u32 *get_pixel_ptr(Bitmap *bitmap, u32 x, u32 y)
{
    u32 *result = 0;
    if(x < bitmap->width && y < bitmap->height)
    {
        result = bitmap->pixels + y * bitmap->width + x;
    }
	
    return result;
}

inline u32 get_color_packed(Bitmap *bitmap, u32 x, u32 y)
{
    u32 result = 0;
    if(x < bitmap->width && y < bitmap->height)
    {
        result = *(bitmap->pixels + y * bitmap->width + x);
    }
	
    return result;
}

inline void set_target_camera_p(RenderInfo *info, V2 v)
{
	info->target_camera_p = v;
}

inline void move_camera(RenderInfo *info, V2 v)
{
	info->target_camera_p = info->current_camera->p + v;
}

inline void set_view_dim(RenderInfo *info, V2 dim)
{
    info->target_camera_view_dim = dim;
}

void push_quad(RenderInfo *info, V4 color, u16 priority,
			   V2 p0, V2 p1, V2 p2, V2 p3,
			   V2 uv0, V2 uv1, V2 uv2, V2 uv3,
			   V2 dim_world, f32 thickness,
			   u8 texture_handle_index, u8 shader_handle_index)
{
	u16 vertex_count = (u16)info->vertex_count;
	u16 quad_count = vertex_count >> 2;

	if(vertex_count + 4 < MAX_RENDER_VERTEX_COUNT)
	{
		V2 p[4] = {p0, p1, p2, p3};
		V2 uv[4] = {uv0, uv1, uv2, uv3};

		u32 c = pack_color(color);

		info->batch_array[quad_count].offset = vertex_count;
		info->batch_array[quad_count].priority = priority;
		info->batch_array[quad_count].texture_handle_index = texture_handle_index;
		info->batch_array[quad_count].shader_handle_index = shader_handle_index;

		f32 view_height_world = info->current_camera->view_dim.y;
		f32 smoothing_radius_world = info->smoothing_distance_coefficient * view_height_world;

		f32 smoothing_uv = clamp(0.001f, smoothing_radius_world / dim_world.x, 1.0f);
		
		for(u32 i = 0; i < 4; ++i)
		{
			Vertex *v = info->vertex_array + info->vertex_count++;
			v->p  = p[i];
			v->uv = uv[i];
			v->c  = c;
			v->smoothing_uv = smoothing_uv;
			v->thickness = thickness;
		}
	}
	else
	{
		crash();
	}
}

inline void push_rect(RenderInfo *info, Rect2 rect,
			   V4 color = v4(1, 1, 1, 1), u16 priority = 0)
{
	V2 uv = v2(0.5, 0.5);
	push_quad(info, color, priority, rect.min, v2(rect.max.x, rect.min.y), rect.max,
			  v2(rect.min.x, rect.max.y), uv, uv, uv, uv, get_dim(rect), 0, 0, info->line_shader_index);
}

void push_line(RenderInfo *info, V2 p0_, V2 p1_, f32 thickness,
			   V4 color = v4(1, 1, 1, 1), u16 priority = 0)
{
	V2 dim_world = {thickness, infinity()};

	V2 line = p1_ - p0_;
	f32 line_length = length(line);

	V2 edge_apron = thickness * line / line_length;
	V2 line_perp = perp(edge_apron);
	
	V2 p0 = p0_ - 0.5f * edge_apron - 0.5f * line_perp;
	V2 p1 = p0 + line_perp;
	V2 p2 = p1 + line + edge_apron;
	V2 p3 = p2 - line_perp;

	push_quad(info, color, priority, p0, p1, p2, p3, v2(0, 0),
			  v2(1, 0), v2(1, 1), v2(0, 1), dim_world, 0, 0, info->line_shader_index);
	//V2 uv = {0.5f, 0.5f};
	//push_quad(info, color, priority, p0, p1, p2, p3, uv, uv, uv, uv, dim_world, 0, 0, info->line_shader_index);
}

inline void push_disk(RenderInfo *info, V2 p, f32 r, f32 thickness,
					  V4 color = v4(1, 1, 1, 1), u16 priority = 0)
{
	V2 dim_world = v2(thickness, infinity());
	push_quad(info, color, priority, p + v2(-r, -r), p + v2(r, -r), p + v2(r, r), p + v2(-r, r), v2(0, 0),
			  v2(1, 0), v2(1, 1), v2(0, 1), dim_world, thickness/r, 0, info->line_shader_index);
}

inline void push_circle(RenderInfo *info, V2 p, f32 r, V4 color = v4(1, 1, 1, 1), u16 priority = 0)
{
	push_disk(info, p, r, r, color, priority);
}

inline void push_quad_outline(RenderInfo *info, V2 p0, V2 p1, V2 p2, V2 p3,
							  f32 thickness, V4 color = v4(1, 1, 1, 1), u16 priority = 0)
{
    push_line(info, p0, p1 - 0.5f * thickness * normalize(p1 - p0), thickness, color, priority);
    push_line(info, p1, p2 - 0.5f * thickness * normalize(p2 - p1), thickness, color, priority);
    push_line(info, p2, p3 - 0.5f * thickness * normalize(p3 - p2), thickness, color, priority);
    push_line(info, p3, p0 - 0.5f * thickness * normalize(p0 - p3), thickness, color, priority);
}

inline void push_rect_outline(RenderInfo *info, Rect2 rect, f32 thickness,
							  V4 color = v4(1, 1, 1, 1), u16 priority = 0)
{
    push_quad_outline(info, rect.min, v2(rect.max.x, rect.min.y),
					  rect.max, v2(rect.min.x, rect.max.y), thickness, color, priority);
}

inline void push_bitmap(RenderInfo *info, Rect2 rect,
						V4 color = v4(1, 1, 1, 1), u16 priority = 0)
{
	u8 texture_index = info->test_texture_index;
	push_quad(info, color, priority, rect.min, v2(rect.max.x, rect.min.y), rect.max,
			  v2(rect.min.x, rect.max.y), v2(0, 0), v2(1, 0), v2(1, 1), v2(0, 1), get_dim(rect),
			  0, texture_index, info->texture_shader_index);
	push_rect_outline(info, rect, 0.01f * get_width(rect), color, priority);
}

f32 get_text_width(Font *font, String text, f32 line_height)
{
	f32 default_character_width = 0.4f * line_height;
	f32 scale = line_height / font->height;

	f32 current_width = 0, max_width = 0;
	for(u32 i = 0; i < text.length; ++i)
	{
		char character = text.data[i];
		if((character >= font->first_character && character <= font->last_character) && character != ' ')
		{
			u8 character_index = character - font->first_character;
			FontGlyph *glyph = font->glyphs + character_index;
			current_width += scale * glyph->x_advance;
		}
		else
		{
			current_width += default_character_width;
		}
		
		if(current_width > max_width)
		{
			max_width = current_width;
		}
	}
	return max_width;
}

f32 get_text_width(Font *font, char *text, f32 line_height)
{
	f32 result = get_text_width(font, string(text), line_height);
	return result;
}

// TODO: consider the x offset at the beginning of the line
Rect2 get_text_rect(Font *font, String text, V2 cursor_p, f32 line_height)
{
	f32 default_character_width = 0.4f * line_height;
	f32 scale = line_height / font->height;
	f32 line_step = scale * font->line_step;

	u32 line_count = 1;
	f32 max_descent = 0;
	f32 current_width = 0, max_width = 0;
	for(u32 i = 0; i < text.length; ++i)
	{
		char character = text.data[i];
		if(character == '\n')
		{
			++line_count;
			max_descent = 0;
		}
		else if((character >= font->first_character && character <= font->last_character) && character != ' ')
		{
			u8 character_index = character - font->first_character;
			FontGlyph *glyph = font->glyphs + character_index;
			current_width += scale * glyph->x_advance;
			f32 descent = scale * (glyph->pixel_dim.y - glyph->offset.y);
			if(descent > max_descent)
			{
				max_descent = descent;
			}
		}
		else
		{
			current_width += default_character_width;
		}
		
		if(current_width > max_width)
		{
			max_width = current_width;
		}
	}

	f32 min_x = cursor_p.x;
	f32 max_x = min_x + max_width;
	f32 max_y = cursor_p.y + line_step;
	f32 min_y = max_y - (line_count * line_step + max_descent);
	Rect2 result = {min_x, min_y, max_x, max_y};

	return result;
}

Rect2 get_text_rect(Font *font, char *text, V2 cursor_p, f32 line_height)
{
	Rect2 result = get_text_rect(font, string(text), cursor_p, line_height);
	return result;
}

V2 get_cursor_p_for_alignment(Font *font, String text, V2 p, f32 line_height,
							  HorizontalAlignment h_align = horizontal_alignment_left,
							  VerticalAlignment v_align = vertical_alignment_bottom)
{
	Rect2 current_rect = get_text_rect(font, text, p, line_height);
	V2 dim = get_dim(current_rect);
	V2 target_min = current_rect.min;
	if(h_align == horizontal_alignment_center)
	{
		target_min.x -= 0.5f * dim.x;
	}
	else if(h_align == horizontal_alignment_right)
	{
		target_min.x -= dim.x;
	}
	if(v_align == vertical_alignment_center)
	{
		target_min.y -= 0.5f * dim.y;
	}
	else if(v_align == vertical_alignment_top)
	{
		target_min.y -= dim.y;
	}

	V2 offset = current_rect.min - target_min;
	V2 result = p - offset;

	return result;
}

void push_string(RenderInfo *info, String string, V2 min_corner, f32 line_height,
				 V4 color = v4(1, 1, 1, 1), u16 priority = 0)
// NOTE: vertical_glyph_alignment specifies _only_ alignment of individual glyphs.
// At the moment there's no way to align the entire string. Potential TODO if we'll need it
{
	V2 default_character_dim = {0.4f * line_height, line_height};

	Font *font = &info->assets.debug_font;
	f32 scale = line_height / font->height;

	f32 line_step = scale * font->line_step;
	
	f32 at_x = min_corner.x;
	f32 at_y = min_corner.y;
	for(u32 i = 0; i < string.length; ++i)
	{
		char character = string.data[i];
		switch(character)
		{
			case '\n':
			{
				at_y -= line_step;
				at_x = min_corner.x;
				break;
			}
			case ' ':
			{
				at_x += default_character_dim.x;
				break;
			}
			default:
			{
				if(character >= font->first_character && character <= font->last_character)
				{
					u8 character_index = character - font->first_character;
					FontGlyph *glyph = font->glyphs + character_index;
					f32 h = scale * glyph->pixel_dim.y;
					f32 w = scale * glyph->pixel_dim.x;
					V2 min = v2(at_x + scale * glyph->offset.x,
								at_y - scale * (glyph->pixel_dim.y - glyph->offset.y));

					push_quad(info, color, priority, min, min + v2(w, 0), min + v2(w, h), min + v2(0, h),
							  glyph->uv_min, v2(glyph->uv_max.x, glyph->uv_min.y), glyph->uv_max,
							  v2(glyph->uv_min.x, glyph->uv_max.y), v2(w, h), 0, glyph->texture_handle_index,
							  info->texture_shader_index);

					at_x += scale * glyph->x_advance;
				}
				else
				{
					at_x += default_character_dim.x;
				}
				
				break;
			}
		}
	}
}

inline void push_string(RenderInfo *info, char *input, V2 min_corner, f32 line_height,
					    V4 color = v4(1, 1, 1, 1), u16 priority = 0)
{
    push_string(info, string(input), min_corner, line_height, color, priority);
}

inline void set_clear_color(RenderInfo *info, V4 color)
{
	info->clear_color = color;
}

void gl_allocate_texture(Bitmap *bitmap)
{
    if(bitmap->gl_handle)
    {
        glDeleteTextures(1, &bitmap->gl_handle);
        bitmap->gl_handle = 0;
    }
    glGenTextures(1, &bitmap->gl_handle);
	assert(bitmap->gl_handle);
	glBindTexture(GL_TEXTURE_2D, bitmap->gl_handle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
				 bitmap->width, bitmap->height, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, bitmap->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

u32 gl_create_shader(u32 type, char **source, u32 source_count)
{
    u32 result = glCreateShader(type);
    
    glShaderSource(result, source_count, source, 0);
    glCompileShader(result);

    s32 status = 0;
    glGetShaderiv(result, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE)
    {
        char log[1024] = {};
        glGetShaderInfoLog(result, sizeof(log), 0, log);
        crash();
    }

    return result;
}

u32 gl_create_program(GLInfo *info, char *shared_source, char *vertex_source, char *fragment_source)
{
	u32 result = 0;
	
    char *full_vertex_source[] = {shared_source, vertex_source};
    char *full_fragment_source[] = {shared_source, fragment_source};
    
    GLuint vertex_shader = gl_create_shader(GL_VERTEX_SHADER, full_vertex_source, array_count(full_vertex_source));
    GLuint fragment_shader = gl_create_shader(GL_FRAGMENT_SHADER, full_fragment_source, array_count(full_fragment_source));

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    glLinkProgram(program);
    glValidateProgram(program);
    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    
    if(status == GL_TRUE)
    {
		result = program;
    }
    else
    {
        char program_log[1024];
        char vertex_log[1024];
        char fragment_log[1024];
        glGetProgramInfoLog(program, sizeof(program_log), 0, program_log);
        glGetShaderInfoLog(vertex_shader, sizeof(vertex_log), 0, vertex_log);
        glGetShaderInfoLog(fragment_shader, sizeof(fragment_log), 0, fragment_log);
        
        crash();
    }

    return result;
}

void gl_use_program(GLInfo *info, u32 program)
{
	// TODO: is all this fuss even necessary?
	// Vertex structure remains the same in all the shaders.
	// The only thing that changes is uniforms, but we could keep them consistent as well.
	
	glDeleteSamplers(1, &info->sampler);

	info->sampler = glGetUniformLocation(program, "sampler");
	info->transform = glGetUniformLocation(program, "transform");
	//info->shifts = glGetUniformLocation(program, "shifts");
	
	info->p_attrib = glGetAttribLocation(program, "p_in");
	info->uv_attrib = glGetAttribLocation(program, "uv_in");
	info->c_attrib = glGetAttribLocation(program, "c_in");
	info->smoothing_uv_attrib = glGetAttribLocation(program, "smoothing_uv_in");
	info->thickness_attrib = glGetAttribLocation(program, "thickness_in");

	glEnableVertexAttribArray(info->p_attrib);
	glEnableVertexAttribArray(info->uv_attrib);
	glEnableVertexAttribArray(info->c_attrib);
	glEnableVertexAttribArray(info->smoothing_uv_attrib);
	glEnableVertexAttribArray(info->thickness_attrib);

	glVertexAttribPointer(info->p_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), member_offset(Vertex, p));
	glVertexAttribPointer(info->uv_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), member_offset(Vertex, uv));
	glVertexAttribIPointer(info->c_attrib, 1, GL_UNSIGNED_INT, sizeof(Vertex), member_offset(Vertex, c));
	glVertexAttribPointer(info->smoothing_uv_attrib, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), member_offset(Vertex, smoothing_uv));
	glVertexAttribPointer(info->thickness_attrib, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), member_offset(Vertex, thickness));

    GLuint sampler;
	glGenSamplers(1, &sampler);
	glUniform1i(info->sampler, sampler);

	//GLuint shifts[4] = {RED_SHIFT, GREEN_SHIFT, BLUE_SHIFT, ALPHA_SHIFT};
	//glUniform1uiv(info->shifts, array_count(shifts), shifts);

	glUseProgram(program);
}

inline void gl_init(RenderInfo *info,
                    u32 window_width, u32 window_height,
                    u32 buffer_width, u32 buffer_height)
{
	// NOTE: the first handles are invalid and are not supposed to be used
	info->assets.shader_handles[0] = info->assets.texture_handles[0] = 0;
	info->assets.shader_count = info->assets.texture_count = 1;
	info->current_shader_handle_index = 0;
	info->current_texture_handle_index = 0;
	info->smoothing_distance_coefficient = 0.00065f;
	
    info->window_viewport_min = v2(0, 0);
    info->window_viewport_dim = v2((f32)window_width, (f32)window_height);

	info->framebuffer_color_texture.width = buffer_width;
	info->framebuffer_color_texture.height = buffer_height;
    u32 pixel_count = buffer_height * buffer_width;
    pixel_count += (4 - (pixel_count & 3)) & 3;
    info->framebuffer_color_texture.pixels = 0;
	gl_allocate_texture(&info->framebuffer_color_texture);
	glGenFramebuffers(1, &info->framebuffer_handle);
	glBindFramebuffer(GL_FRAMEBUFFER, info->framebuffer_handle);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						   GL_TEXTURE_2D, info->framebuffer_color_texture.gl_handle, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    
    glViewport(0, 0, window_width, window_height);
    glScissor(0, 0, window_width, window_height);

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	char *shared_source = "\
    #version 130\n\
    #define f32 float\n\
    #define u32 uint\n\
    #define s32 int\n\
    #define v2 vec2\n\
    #define v2i ivec2\n\
    #define v4 vec4\n\
    #define m4x4 mat4x4\n";
	
	char *vertex_source = "\
    uniform m4x4 transform;\n\
    \n\
    in v2 p_in;\n\
    in v2 uv_in;\n\
    in u32 c_in;\n\
    in f32 smoothing_uv_in;\n\
    in f32 thickness_in;\n\
    smooth out v2 uv;\n\
    smooth out v4 c;\n\
    flat out f32 smoothing_uv;\n\
    flat out f32 thickness;\n\
    \n\
    v4 unpack_color(u32 color_packed)\n\
    {\n\
        // NOTE: Will be giving erroneous results if color packing format changes.\n\
        u32 mask = u32(255);\n\
    	v4 result = (1/255.0f) * v4(f32((color_packed >>  0) & mask), \n\
    								f32((color_packed >>  8) & mask), \n\
    								f32((color_packed >> 16) & mask), \n\
    								f32((color_packed >> 24) & mask));\n\
    	return result;\n\
    }\n\
    \n\
    void main()\n\
    {\n\
    	gl_Position = transform * v4(p_in, 0, 1);\n\
    	\n\
    	uv = uv_in;\n\
    	c = unpack_color(c_in);\n\
    	smoothing_uv = smoothing_uv_in;\n\
    	thickness = thickness_in;\n\
    }\n";
	
	char *line_fragment_source = "\
    smooth in v2 uv;\n\
    smooth in v4 c;\n\
    flat in f32 smoothing_uv;\n\
    flat in f32 thickness;\n\
    out v4 frag_color;\n\
    #define epsilon 0.0000001\n\
    v2 center = v2(0.5, 0.5);\n\
    \n\
    void main()\n\
    {\n\
    	f32 distance = length(uv - center);\n\
    	f32 disk_relative = (distance - (0.5 - thickness)) / max(thickness, epsilon);\n\
    	f32 x = mix(uv.x, disk_relative, f32(thickness > 0.0));\n\
    	x = clamp(0.5 - abs(x - 0.5), 0, smoothing_uv);\n\
    	frag_color.rgb = c.rgb;\n\
    	frag_color.a = c.a * x / smoothing_uv;\n\
    }\n";
	
	char *texture_fragment_source = "\
    uniform sampler2D sampler;\n\
    \n\
    smooth in v2 uv;\n\
    smooth in v4 c;\n\
    out v4 frag_color;\n\
    \n\
    void main()\n\
    {\n\
        frag_color = c * texture(sampler, uv);\n\
    }\n";

    GLuint line_program = gl_create_program(&info->gl_info, shared_source,
											vertex_source, line_fragment_source);
	
	u8 line_shader_index = info->assets.shader_count++;
	info->assets.shader_handles[line_shader_index] = line_program;
	info->line_shader_index = line_shader_index;
	
    GLuint texture_program = gl_create_program(&info->gl_info, shared_source,
											   vertex_source, texture_fragment_source);
	
	u8 texture_shader_index = info->assets.shader_count++;
	info->assets.shader_handles[texture_shader_index] = texture_program;
	info->texture_shader_index = texture_shader_index;

	GLuint buffer_handles[2];
	glGenBuffers(2, buffer_handles);

	glBindBuffer(GL_ARRAY_BUFFER, buffer_handles[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_handles[1]);

	glBufferData(GL_ARRAY_BUFFER, MAX_RENDER_VERTEX_COUNT * sizeof(Vertex), 0, GL_STREAM_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_RENDER_INDEX_COUNT * sizeof(u16), 0, GL_STREAM_DRAW);
	info->vertex_array = push_array(&info->memory, Vertex, MAX_RENDER_VERTEX_COUNT, sizeof(Vertex));
	info->batch_array = push_array(&info->memory, RenderBatch, MAX_RENDER_ENTRY_COUNT, sizeof(RenderBatch));
}

void set_current_camera(RenderInfo *info, Camera2 *camera)
{
    info->current_camera = camera;
	info->target_camera_p = camera->p;
	info->target_camera_view_dim = camera->view_dim;
}

void renderer_init(RenderInfo *info, Camera2 *camera, V4 clear_color)
{
	set_current_camera(info, camera);
	set_clear_color(info, clear_color);
}

void sort_batches(RenderInfo *info)
{
    begin_tmp_memory(&info->memory);

	u32 quad_count = info->vertex_count >> 2;

	RenderBatch *batches_a = info->batch_array;
	RenderBatch *batches_b = push_array(&info->memory, RenderBatch, quad_count, sizeof(RenderBatch));
	
    for(u32 shift = 0; shift < 32; shift += 8)
    {
        u32 offsets[256] = {};
        
        for(u32 index = 0; index < quad_count; ++index)
        {
            u32 byte_value = (batches_a[index].sorting_key >> shift) & 0xff;
            ++offsets[byte_value];
        }
        
        u32 accumulation_offset = 0;
        for(u32 offset_index = 0; offset_index < array_count(offsets); ++offset_index)
        {
            u32 offset = offsets[offset_index];
            offsets[offset_index] = accumulation_offset;
            accumulation_offset += offset;
        }
        
        for(u32 index = 0; index < quad_count; ++index)
        {
            u32 byte_value = (batches_a[index].sorting_key >> shift) & 0xff;
			batches_b[offsets[byte_value]++] = batches_a[index];
        }

		ptr_swap(&batches_a, &batches_b);
    }
}

void gl_draw_entries(RenderInfo *info)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, info->framebuffer_handle);
    glViewport(0, 0, info->framebuffer_color_texture.width, info->framebuffer_color_texture.height);
    glScissor(0, 0, info->framebuffer_color_texture.width, info->framebuffer_color_texture.height);

	glClearColor(info->clear_color.r, info->clear_color.g, info->clear_color.b, info->clear_color.a);
	glClear(GL_COLOR_BUFFER_BIT);
	
	sort_batches(info);
	u16 quad_count = (u16)info->vertex_count >> 2;

	begin_tmp_memory(&info->memory);
	u32 index_count = 6 * (u32)quad_count;
	u16 *indices = push_array(&info->memory, u16, index_count, sizeof(u16));

	u16 current_batch_index = 0;
	u16 current_batch_vertex_count = 0;
	u16 last_asset_id = info->batch_array[0].asset_id;
	
	for(u16 batch_index = 0; batch_index < quad_count; ++batch_index)
	{
		RenderBatch batch = info->batch_array[batch_index];
		u32 offset = 6 * batch_index;
		assert(batch.offset + 3 <= U16_MAX);
		u16 batch_offset = (u16)batch.offset;
		indices[offset + 0] = batch_offset + 0;
		indices[offset + 1] = batch_offset + 1;
		indices[offset + 2] = batch_offset + 3;
		indices[offset + 3] = batch_offset + 3;
		indices[offset + 4] = batch_offset + 1;
		indices[offset + 5] = batch_offset + 2;
		if(batch.asset_id != last_asset_id)
		{
			info->batch_array[current_batch_index].vertex_count = current_batch_vertex_count;
			info->batch_array[current_batch_index].asset_id = last_asset_id;
			last_asset_id = batch.asset_id;
			++current_batch_index;
			current_batch_vertex_count = 0;
		}
		current_batch_vertex_count += 6;
	}
	info->batch_array[current_batch_index].vertex_count = current_batch_vertex_count;
	info->batch_array[current_batch_index].asset_id = last_asset_id;
	u16 batch_count = current_batch_index + 1;

	f32 a = 2/info->current_camera->view_dim.x;
	f32 b = 2/info->current_camera->view_dim.y;
	f32 c = info->current_camera->p.x;
	f32 d = info->current_camera->p.y;
	f32 proj[4][4] =
	{
		a, 0, 0, -a * c,
		0, b, 0, -b * d,
		0, 0, 0, 0,
		0, 0, 0, 1,		
	};

	glBufferSubData(GL_ARRAY_BUFFER, 0, info->vertex_count * sizeof(Vertex), info->vertex_array);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, 6 * quad_count * sizeof(u16), indices);

	u32 cumulative_offset = 0;
	for(u32 batch_index = 0; batch_index < batch_count; ++batch_index)
	{
		RenderBatch *batch = info->batch_array + batch_index;

		if(batch->shader_handle_index && batch->shader_handle_index != info->current_shader_handle_index)
		{
			gl_use_program(&info->gl_info, info->assets.shader_handles[batch->shader_handle_index]);
			info->current_shader_handle_index = batch->shader_handle_index;
		}
		
		if(batch->texture_handle_index && batch->texture_handle_index != info->current_texture_handle_index)
		{
			glBindTexture(GL_TEXTURE_2D, info->assets.texture_handles[batch->texture_handle_index]);
			info->current_texture_handle_index = batch->texture_handle_index;
		}
		
		glUniformMatrix4fv(info->gl_info.transform, 1, GL_TRUE, proj[0]);
		glDrawElements(GL_TRIANGLES, batch->vertex_count, GL_UNSIGNED_SHORT, (void *)(cumulative_offset * sizeof(indices[0])));
		cumulative_offset += batch->vertex_count;
	}

	info->vertex_count = 0;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, info->framebuffer_handle);
	
    s32 dst_minx = (s32)info->window_viewport_min.x;
    s32 dst_miny = (s32)info->window_viewport_min.y;
    s32 dst_dimx = (s32)info->window_viewport_dim.x;
    s32 dst_dimy = (s32)info->window_viewport_dim.y;
    s32 dst_maxx = dst_minx + dst_dimx;
    s32 dst_maxy = dst_miny + dst_dimy;
    
    s32 src_minx = 0;
    s32 src_miny = 0;
    s32 src_dimx = info->framebuffer_color_texture.width;
    s32 src_dimy = info->framebuffer_color_texture.height;
    s32 src_maxx = src_minx + src_dimx;
    s32 src_maxy = src_miny + src_dimy;
    
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(dst_minx, dst_miny, dst_dimx, dst_dimy);
    glScissor(dst_minx, dst_miny, dst_dimx, dst_dimy);
    glBlitFramebuffer(src_minx, src_miny, src_maxx, src_maxy,
                      dst_minx, dst_miny, dst_maxx, dst_maxy,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

	info->current_camera->p = info->target_camera_p;
	info->current_camera->view_dim = info->target_camera_view_dim;
}
