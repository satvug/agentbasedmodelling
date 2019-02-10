#define gl_global_func(name) static name##_func *name
#define wgl_get_func(name) name = (name##_func *)wglGetProcAddress(#name)

#define WGL_SAMPLES_ARB                           0x2042
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB               0x2093
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

typedef HGLRC wglCreateContextAttribsARB_func(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL wglSwapIntervalEXT_func(int interval);

gl_global_func(wglCreateContextAttribsARB);
gl_global_func(wglSwapIntervalEXT);

#define GL_INVALID_FRAMEBUFFER_OPERATION  0x0506
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_FRAMEBUFFER                    0x8D40
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STREAM_DRAW                    0x88E0

typedef signed   long long int GLsizeiptr; // platform specific
typedef signed   long long int GLintptr; // platform specific
typedef char GLchar;

typedef   void APIENTRY glGenFramebuffers_func(GLsizei n, GLuint *framebuffers);
typedef   void APIENTRY glBindFramebuffer_func(GLenum target, GLuint framebuffer);
typedef   void APIENTRY glDeleteFramebuffers_func(GLsizei n, const GLuint *framebuffers);
typedef   void APIENTRY glFramebufferTexture2D_func(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef   void APIENTRY glBlitFramebuffer_func(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef GLuint APIENTRY glCreateProgram_func(void);
typedef GLuint APIENTRY glCreateShader_func(GLenum type);
typedef   void APIENTRY glShaderSource_func(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef   void APIENTRY glCompileShader_func(GLuint shader);
typedef   void APIENTRY glAttachShader_func(GLuint program, GLuint shader);
typedef   void APIENTRY glLinkProgram_func(GLuint program);
typedef   void APIENTRY glUseProgram_func(GLuint program);
typedef   void APIENTRY glGenSamplers_func(GLsizei count, GLuint *samplers);
typedef   void APIENTRY glDeleteSamplers_func(GLsizei count, const GLuint *samplers);
typedef   void APIENTRY glValidateProgram_func(GLuint program);
typedef   void APIENTRY glGetProgramiv_func(GLuint program, GLenum pname, GLint *params);
typedef   void APIENTRY glGetProgramInfoLog_func(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef   void APIENTRY glGetShaderiv_func(GLuint shader, GLenum pname, GLint *params);
typedef   void APIENTRY glGetShaderInfoLog_func(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef  GLint APIENTRY glGetUniformLocation_func(GLuint program, const GLchar *name);
typedef   void APIENTRY glUniform1i_func(GLint location, GLint v0);
typedef   void APIENTRY glUniform2i_func(GLint location, GLint v0, GLint v1);
typedef   void APIENTRY glUniform3i_func(GLint location, GLint v0, GLint v1, GLint v2);
typedef   void APIENTRY glUniform4i_func(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef   void APIENTRY glUniform1ui_func(GLint location, GLuint v0);
typedef   void APIENTRY glUniform2ui_func(GLint location, GLuint v0, GLuint v1);
typedef   void APIENTRY glUniform3ui_func(GLint location, GLuint v0, GLuint v1, GLuint v2);
typedef   void APIENTRY glUniform4ui_func(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
typedef   void APIENTRY glUniform1f_func(GLint location, GLfloat v0);
typedef   void APIENTRY glUniform1uiv_func(GLint location, GLsizei count, const GLuint *value);
typedef   void APIENTRY glUniformMatrix4fv_func(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef  GLint APIENTRY glGetAttribLocation_func(GLuint program, const GLchar *name);
typedef   void APIENTRY glGenBuffers_func(GLsizei n, GLuint *buffers);
typedef   void APIENTRY glBindBuffer_func(GLenum target, GLuint buffer);
typedef   void APIENTRY glEnableVertexAttribArray_func(GLuint index);
typedef   void APIENTRY glVertexAttribPointer_func(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef   void APIENTRY glVertexAttribIPointer_func(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef   void APIENTRY glBufferData_func(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef   void APIENTRY glBufferSubData_func(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);

gl_global_func(glGenFramebuffers);
gl_global_func(glDeleteFramebuffers);
gl_global_func(glBindFramebuffer);
gl_global_func(glFramebufferTexture2D);
gl_global_func(glBlitFramebuffer);
gl_global_func(glCreateProgram);
gl_global_func(glCreateShader);
gl_global_func(glShaderSource);
gl_global_func(glCompileShader);
gl_global_func(glAttachShader);
gl_global_func(glLinkProgram);
gl_global_func(glUseProgram);
gl_global_func(glGenSamplers);
gl_global_func(glDeleteSamplers);
gl_global_func(glValidateProgram);
gl_global_func(glGetProgramiv);
gl_global_func(glGetProgramInfoLog);
gl_global_func(glGetShaderiv);
gl_global_func(glGetShaderInfoLog);
gl_global_func(glGetUniformLocation);
gl_global_func(glUniform1i);
gl_global_func(glUniform2i);
gl_global_func(glUniform3i);
gl_global_func(glUniform4i);
gl_global_func(glUniform1ui);
gl_global_func(glUniform2ui);
gl_global_func(glUniform3ui);
gl_global_func(glUniform4ui);
gl_global_func(glUniform1uiv);
gl_global_func(glUniform1f);
gl_global_func(glUniformMatrix4fv);
gl_global_func(glGetAttribLocation);
gl_global_func(glGenBuffers);
gl_global_func(glBindBuffer);
gl_global_func(glEnableVertexAttribArray);
gl_global_func(glVertexAttribPointer);
gl_global_func(glVertexAttribIPointer);
gl_global_func(glBufferData);
gl_global_func(glBufferSubData);


void check_opengl_errors_professionally()
{
	GLenum error = glGetError();
	switch(error)
	{
		case GL_NO_ERROR:
		{
			break;
		}
		case GL_INVALID_ENUM:
		{
			crash();
		}
		case GL_INVALID_VALUE:
		{
			crash();
		}
		case GL_INVALID_OPERATION:
		{
			crash();
		}
		case GL_INVALID_FRAMEBUFFER_OPERATION:
		{
			crash();
		}
		case GL_OUT_OF_MEMORY:
		{
			crash();
		}
		default:
		{
			crash();
		}
	};
}

void wgl_init(HDC dc, s32 swap_interval)
{
	PIXELFORMATDESCRIPTOR target_pixel_format = {};
	target_pixel_format.nSize = sizeof(target_pixel_format);
	target_pixel_format.nVersion = 1;
	target_pixel_format.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	target_pixel_format.iPixelType = PFD_TYPE_RGBA;
	target_pixel_format.cColorBits = 32;
	target_pixel_format.cAlphaBits = 8;
	target_pixel_format.iLayerType = PFD_MAIN_PLANE;
	s32 pixel_format_index = ChoosePixelFormat(dc, &target_pixel_format);
	PIXELFORMATDESCRIPTOR pixel_format;
	DescribePixelFormat(dc, pixel_format_index, sizeof(pixel_format), &pixel_format);
	SetPixelFormat(dc, pixel_format_index, &pixel_format);
	
	HGLRC gl_rc = wglCreateContext(dc);
	assert(wglMakeCurrent(dc, gl_rc));
			
	char *extensions = (char *)glGetString(GL_EXTENSIONS);
	//char *gl_version = (char *)glGetString(GL_VERSION);
	//char *glsl_version = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);

	// TODO: handle it more gracefully	
	bool framebuffers_supported = string_contains(extensions, "GL_ARB_framebuffer_object");
	assert(framebuffers_supported);

	wgl_get_func(wglCreateContextAttribsARB);
	int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 0,
			WGL_CONTEXT_FLAGS_ARB, 0,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};
	HGLRC cooler_gl_rc = wglCreateContextAttribsARB(dc, 0, attribs);
	if(cooler_gl_rc)
	{
		wglDeleteContext(gl_rc);
		assert(wglMakeCurrent(dc, cooler_gl_rc));
	}

	wgl_get_func(wglSwapIntervalEXT);
	wglSwapIntervalEXT(swap_interval);

	wgl_get_func(glCreateProgram);
	wgl_get_func(glCreateShader);
	wgl_get_func(glShaderSource);
	wgl_get_func(glCompileShader);
	wgl_get_func(glAttachShader);
	wgl_get_func(glLinkProgram);
	wgl_get_func(glUseProgram);
	wgl_get_func(glGenSamplers);
	wgl_get_func(glDeleteSamplers);
	wgl_get_func(glValidateProgram);
	wgl_get_func(glGetProgramiv);
	wgl_get_func(glGetProgramInfoLog);
	wgl_get_func(glGetShaderiv);
	wgl_get_func(glGetShaderInfoLog);
	wgl_get_func(glGetUniformLocation);
	wgl_get_func(glUniform1i);
	wgl_get_func(glUniform2i);
	wgl_get_func(glUniform3i);
	wgl_get_func(glUniform4i);
	wgl_get_func(glUniform1ui);
	wgl_get_func(glUniform2ui);
	wgl_get_func(glUniform3ui);
	wgl_get_func(glUniform4ui);
    wgl_get_func(glUniform1uiv);
	wgl_get_func(glUniform1f);
	wgl_get_func(glUniformMatrix4fv);
	wgl_get_func(glGenFramebuffers);
	wgl_get_func(glBindFramebuffer);
	wgl_get_func(glDeleteFramebuffers);
	wgl_get_func(glFramebufferTexture2D);
	wgl_get_func(glBlitFramebuffer);
	wgl_get_func(glGetAttribLocation);
	wgl_get_func(glGenBuffers);
	wgl_get_func(glBindBuffer);
	wgl_get_func(glEnableVertexAttribArray);
	wgl_get_func(glVertexAttribPointer);
	wgl_get_func(glVertexAttribIPointer);
	wgl_get_func(glBufferData);
	wgl_get_func(glBufferSubData);

	check_opengl_errors_professionally();
}

inline void wgl_enable_v_sync()
{
	wglSwapIntervalEXT(1);
}

inline void wgl_disable_v_sync()
{
	wglSwapIntervalEXT(0);
}
