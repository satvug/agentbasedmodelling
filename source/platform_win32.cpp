#include <windows.h>

#include <stdint.h>
#include <stdarg.h>
#include <immintrin.h>

#include <gl/gl.h>

#include "types.h"
#include "math.h"
#include "memory.h"
#include "random.h"
#include "shared.h"

#define MAX_PATH_LENGTH 1024

void free_file(File *file)
{
	VirtualFree(file->data, 0, MEM_RELEASE);
	file->data = 0;
	file->size = 0;
}

File read_entire_file(String path_string)
{
	char path[MAX_PATH_LENGTH] = {};
	for(u32 i = 0; i < path_string.length; ++i)
	{
		path[i] = path_string.data[i];
	}
	
    File result = {};
	
	HANDLE file_handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(file_handle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER large_integer;
		GetFileSizeEx(file_handle, &large_integer);
		LONGLONG file_size = large_integer.QuadPart;
		result.data = VirtualAlloc(0, file_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if(result.data)
		{
			bool read_successful = ReadFile(file_handle, result.data, (DWORD)file_size, (DWORD *)&result.size, 0);
			if(!read_successful)
			{
				free_file(&result);
			}
		}
	}
	else
	{
		DeleteFileA(path);
	}
	CloseHandle(file_handle);
	return result;
}

File read_entire_file(char *path)
{
	File result = read_entire_file(string(path));
	return result;
}

bool write_file(void *data, u32 data_size, String path_string)
{
	bool result = false;
	char path[MAX_PATH_LENGTH] = {};
	for(u32 i = 0; i < path_string.length; ++i)
	{
		path[i] = path_string.data[i];
	}
	HANDLE file_handle = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(file_handle != INVALID_HANDLE_VALUE)
	{
		DWORD written;
		result = WriteFile(file_handle, data, data_size, &written, 0) && written == data_size;
	}
	CloseHandle(file_handle);
	return result;
}

bool write_file(void *data, u32 data_size, char *path)
{
	bool result = write_file(data, data_size, string(path));
	return result;
}

#include "constants.h"
#include "wgl.h"
#include "input.h"
#include "renderer.h"
#include "spacial_partition.h"
#include "obstacle.h"
#include "sim.h"
#include "agent.h"
#include "file_io.h"
#include "editor.h"
#include "console.h"
#include "main.h"

#include "renderer.cpp"
#include "spacial_partition.cpp"
#include "obstacle.cpp"
#include "agent.cpp"
#include "sim.cpp"
#include "file_io.cpp"
#include "editor.cpp"
#include "console.cpp"
#include "main.cpp"

static s32 current_swap_interval = 1;
static bool rolling = true;
static RenderInfo render_info;
static Input input;

// shamelessly stolen from https://blogs.msdn.microsoft.com/oldnewthing/20100412-00/?p=14353
static WINDOWPLACEMENT prev_window_placement = {sizeof(prev_window_placement)};
void toggle_fullscreen(HWND window)
{
	DWORD style = GetWindowLong(window, GWL_STYLE);
	if(style & WS_OVERLAPPEDWINDOW)
	{
		HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO monitor_info = {sizeof(monitor_info)};
		if(GetWindowPlacement(window, &prev_window_placement) && GetMonitorInfo(monitor, &monitor_info))
		{
			RECT m_rect = monitor_info.rcMonitor;
			SetWindowLong(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(window, HWND_TOP, m_rect.left, m_rect.top, m_rect.right - m_rect.left,
						 m_rect.bottom - m_rect.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}
	else
	{
		SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(window, &prev_window_placement);
		SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
					 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

LRESULT CALLBACK main_callback(HWND window, UINT msg, WPARAM w_param, LPARAM l_param)
{
	LRESULT result = 0;

	switch (msg)
	{
        case WM_DESTROY:
        case WM_CLOSE:
        case WM_QUIT:
        {
            rolling = false;
            break;
        }

		case WM_CHAR:
		{
			u32 symbol = (u32)w_param;
			input.last_pressed_key = symbol;
			break;
		}
		
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			bool is_down = ((l_param >> 31) & 1) == 0;
			u32 symbol = (u32)w_param;
			switch(symbol)
			{
				case VK_ESCAPE:
				{
				    rolling = false;
					break;
				}
				case VK_SHIFT:
				{
					input.buttons[button_code_shift].is_down = is_down;
					break;
				}
				case VK_CONTROL:
				{
					input.buttons[button_code_control].is_down = is_down;
					break;
				}
				case VK_BACK:
				{
					input.buttons[button_code_backspace].is_down = is_down;
					break;
				}
				case VK_DELETE:
				{
					input.buttons[button_code_delete].is_down = is_down;
					break;
				}
				case VK_RIGHT:
				{
					input.buttons[button_code_arrow_right].is_down = is_down;
					break;
				}
				case VK_UP:
				{
					input.buttons[button_code_arrow_up].is_down = is_down;
					break;
				}
				case VK_LEFT:
				{
					input.buttons[button_code_arrow_left].is_down = is_down;
					break;
				}
				case VK_DOWN:
				{
					input.buttons[button_code_arrow_down].is_down = is_down;
					break;
				}
				default:
				{
					if(symbol >= ' ' && symbol <= 'Z')
					{
						input.buttons[symbol].is_down = is_down;
						if(is_down && symbol == 'F' && is_pressed(&input, control))
						{
							toggle_fullscreen(window);
						}
					}
					break;
				}
			}
			break;
		}

		case WM_MOUSEWHEEL:
		{
			s16 wheel_delta = (s16)((w_param & 0xffff0000) >> 16);
			input.mouse_wheel_delta = wheel_delta / (f32)WHEEL_DELTA;
			break;
		}
		
		case WM_LBUTTONDOWN:
		{
			SetCapture(window);
			input.buttons[button_code_mouse_left].is_down = true;
			break;
		}
		case WM_LBUTTONUP:
		{
			ReleaseCapture();
			input.buttons[button_code_mouse_left].is_down = false;
			break;
		}
		case WM_RBUTTONDOWN:
		{
			SetCapture(window);
			input.buttons[button_code_mouse_right].is_down = true;
			break;
		}
		case WM_RBUTTONUP:
		{
			ReleaseCapture();
			input.buttons[button_code_mouse_right].is_down = false;
			break;
		}

		case WM_MOUSEMOVE:
		{
			s32 x = (s32)(s16)(l_param & 0xffff);
			s32 y = (s32)(s16)((l_param >> 16) & 0xffff);
			RECT client_rect;
			GetClientRect(window, &client_rect);

			input.mouse_dp = v2((f32)x, (f32)(client_rect.bottom - y)) - input.mouse_p;
			input.mouse_p += input.mouse_dp;
			
			break;
		}
		
		case WM_SIZE:
		{
			u32 width = l_param & 0xffff;
			u32 height = (l_param >> 16) & 0xffff;
			
			glScissor(0, 0, width, height);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
                        
			u32 new_width;
			u32 new_height;
			f32 target_aspect_ratio = DEFAULT_ASPECT_RATIO;
			if(render_info.current_camera)
			{
				target_aspect_ratio = get_width_over_height(render_info.current_camera);
			}
			
			f32 aspect_ratio = (f32)width / (f32)height;
			if(!almost_equal(aspect_ratio, target_aspect_ratio))
			{
				if(aspect_ratio < target_aspect_ratio)
				{
					new_width = width;
					new_height = (u32)(width / target_aspect_ratio);
				}
				else
				{
					new_height = height;
					new_width = (u32)(height * target_aspect_ratio);
				}
			}
			else
			{
				new_width = width;
				new_height = height;
			}
			
			f32 minx = 0.5f * (width - new_width);
			f32 miny = 0.5f * (height - new_height);
			Rect2 vp = min_dim_rect(minx, miny, (f32)new_width, (f32)new_height);
                        
			render_info.window_viewport_min = v2(minx, miny);
			render_info.window_viewport_dim = v2((f32)new_width, (f32)new_height);
		}

		default:
		{
			result = DefWindowProc(window, msg, w_param, l_param);
			break;
		}
	}
    
	return result;
}

s32 CALLBACK WinMain(HINSTANCE h_instance, HINSTANCE h_prev_instance,
					 LPSTR lp_cmd_line, s32 n_cmd_show)
{
	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = main_callback;
	window_class.hInstance = h_instance;
	window_class.hCursor = LoadCursor(0, IDC_ARROW);
	window_class.lpszClassName = "abm_window_class";
    
    u32 buffer_width = 1920;//1280;
    u32 buffer_height = 1080;//720;

	if(RegisterClass(&window_class))
	{
		DWORD window_style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
		HWND window = CreateWindowEx(0, window_class.lpszClassName, "abm", window_style, CW_USEDEFAULT,
									 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, h_instance, 0);
		if(window)
		{
			RECT client_rect;
			GetClientRect(window, &client_rect);
			u32 window_width = client_rect.right;
			u32 window_height = client_rect.bottom;
			
			MemoryBlock block = {};
			block.size = RENDER_MEMORY_SIZE + PERSISTENT_MEMORY_SIZE + SCRATCH_MEMORY_SIZE + STATS_MEMORY_SIZE;
			block.base = VirtualAlloc(0, block.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            // OpenGL initialization

			HDC dc = GetDC(window);
			wgl_init(dc, current_swap_interval);

			//
			// OpenGL initialization
			// Platform independent part

			render_info.memory = create_subblock(&block, RENDER_MEMORY_SIZE);
			gl_init(&render_info, window_width, window_height,
					buffer_width, buffer_height);

			//
			//

			SimStorage storage = {};
			storage.scratch_memory = create_subblock(&block, SCRATCH_MEMORY_SIZE);
		    storage.persistent_memory = create_subblock(&block, PERSISTENT_MEMORY_SIZE);
		    storage.stats_memory = create_subblock(&block, STATS_MEMORY_SIZE);
			char executable_path[128] = {};
			GetModuleFileNameA(0, executable_path, sizeof(executable_path));
			init(&storage, &render_info, executable_path);

			//
			//

			LARGE_INTEGER counter_frequency;
			QueryPerformanceFrequency(&counter_frequency);
		    u64 frequency = counter_frequency.QuadPart;

			LARGE_INTEGER counter_current;
			LARGE_INTEGER counter_last;
			QueryPerformanceCounter(&counter_current);

            while(rolling)
            {
				LARGE_INTEGER counter_tmp;
				
				counter_last = counter_current;
				QueryPerformanceCounter(&counter_current);
				u64 counter_elapsed = counter_current.QuadPart - counter_last.QuadPart;
				f32 dt = counter_elapsed / (f32)frequency;

				for(u32 i = 0; i < button_code_count; ++i)
				{
					input.buttons[i].was_down = input.buttons[i].is_down;
				}
				input.mouse_dp = v2(0, 0);
				input.mouse_wheel_delta = 0.0f;
				input.last_pressed_key = 0;

				MSG message;
				while(PeekMessage(&message, window, 0, 0, PM_REMOVE))
				{
					DispatchMessage(&message);
					TranslateMessage(&message);
				}

				char log[128] = {};
				String log_string = {log, 0};

				if(storage.state.real_time_sim && current_swap_interval == 0)
				{
					current_swap_interval = 1;
					wglSwapIntervalEXT(current_swap_interval);
				}
				else if(!storage.state.real_time_sim && current_swap_interval == 1)
				{
					current_swap_interval = 0;
					wglSwapIntervalEXT(current_swap_interval);
				}
				
				update(&storage, &render_info, &input, dt);
				gl_draw_entries(&render_info);

				QueryPerformanceCounter(&counter_tmp);
				counter_elapsed = counter_tmp.QuadPart - counter_current.QuadPart;
				dt = counter_elapsed / (f32)frequency;
				append_formatted(&log_string, sizeof(log) - 1, "simulation and rendering time [%d agents]: %f\n",
								 storage.agent_count, dt);
				OutputDebugStringA(log);

                SwapBuffers(dc);
            }
		}
		else
		{
			OutputDebugStringA("Failed to create window\n");
		}
	}
	else
	{
		OutputDebugStringA("WinClass registration failed\n");
	}

	return 0;
}
