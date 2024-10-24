#if GAL_PROMETHEUS_USE_MODULE
import gal.prometheus;
#else
#include <prometheus.ixx>
#endif

#include <GLFW/glfw3.h>

#include <print>
#include <cstdio>

namespace
{
	using namespace gal::prometheus;

	GLFWwindowfocusfun g_glfw_callback_window_focus;
	GLFWcursorenterfun g_glfw_callback_window_cursor_enter;
	GLFWcursorposfun g_glfw_callback_window_cursor_position;
	GLFWmousebuttonfun g_glfw_callback_window_mouse_button;
	GLFWscrollfun g_glfw_callback_window_scroll;
	GLFWkeyfun g_glfw_callback_window_key;
	GLFWcharfun g_glfw_callback_window_char;
	GLFWmonitorfun g_glfw_callback_window_monitor;

	enum class MouseButton
	{
		LEFT = GLFW_MOUSE_BUTTON_LEFT,
		RIGHT = GLFW_MOUSE_BUTTON_RIGHT,
		MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
		X1,
		X2,
	};

	enum class MouseAction
	{
		RELEASE = GLFW_RELEASE,
		PRESS = GLFW_PRESS,
	};

	enum class MouseMod
	{
		NONE = 0,
		SHIFT = GLFW_MOD_SHIFT,
		CONTROL = GLFW_MOD_CONTROL,
		ALT = GLFW_MOD_ALT,
		SUPER = GLFW_MOD_SUPER,
		CAPS_LOCK = GLFW_MOD_CAPS_LOCK,
		NUM_LOCK = GLFW_MOD_NUM_LOCK,
	};

	enum class KeyboardKeyCode
	{
		NUM_0 = GLFW_KEY_0,
		NUM_1 = GLFW_KEY_1,
		NUM_2 = GLFW_KEY_2,
		NUM_3 = GLFW_KEY_3,
		NUM_4 = GLFW_KEY_4,
		NUM_5 = GLFW_KEY_5,
		NUM_6 = GLFW_KEY_6,
		NUM_7 = GLFW_KEY_7,
		NUM_8 = GLFW_KEY_8,
		NUM_9 = GLFW_KEY_9,
		A = GLFW_KEY_A,
		B = GLFW_KEY_B,
		C = GLFW_KEY_C,
		D = GLFW_KEY_D,
		E = GLFW_KEY_E,
		F = GLFW_KEY_F,
		G = GLFW_KEY_G,
		H = GLFW_KEY_H,
		I = GLFW_KEY_I,
		J = GLFW_KEY_J,
		K = GLFW_KEY_K,
		L = GLFW_KEY_L,
		M = GLFW_KEY_M,
		N = GLFW_KEY_N,
		O = GLFW_KEY_O,
		P = GLFW_KEY_P,
		Q = GLFW_KEY_Q,
		R = GLFW_KEY_R,
		S = GLFW_KEY_S,
		T = GLFW_KEY_T,
		U = GLFW_KEY_U,
		V = GLFW_KEY_V,
		W = GLFW_KEY_W,
		X = GLFW_KEY_X,
		Y = GLFW_KEY_Y,
		Z = GLFW_KEY_Z,
	};

	enum class KeyboardAction
	{
		RELEASE = GLFW_RELEASE,
		PRESS = GLFW_PRESS,
		REPEAT = GLFW_REPEAT,
	};

	enum class KeyboardMod
	{
		NONE = 0,
		SHIFT = GLFW_MOD_SHIFT,
		CONTROL = GLFW_MOD_CONTROL,
		ALT = GLFW_MOD_ALT,
		SUPER = GLFW_MOD_SUPER,
		CAPS_LOCK = GLFW_MOD_CAPS_LOCK,
		NUM_LOCK = GLFW_MOD_NUM_LOCK,
	};
}

template<>
struct meta::user_defined::enum_name_policy<MouseButton>
{
	constexpr static auto value = EnumNamePolicy::WITH_SCOPED_NAME;
};

template<>
struct meta::user_defined::enum_name_policy<MouseAction>
{
	constexpr static auto value = EnumNamePolicy::WITH_SCOPED_NAME;
};

template<>
struct meta::user_defined::enum_name_policy<MouseMod>
{
	constexpr static auto value = EnumNamePolicy::WITH_SCOPED_NAME;
};

template<>
struct meta::user_defined::enum_name_policy<KeyboardKeyCode>
{
	constexpr static auto value = EnumNamePolicy::WITH_SCOPED_NAME;
};

template<>
struct meta::user_defined::enum_name_policy<KeyboardAction>
{
	constexpr static auto value = EnumNamePolicy::WITH_SCOPED_NAME;
};

template<>
struct meta::user_defined::enum_name_policy<KeyboardMod>
{
	constexpr static auto value = EnumNamePolicy::WITH_SCOPED_NAME;
};

io::DeviceEventQueue g_device_event_queue;

auto glfw_callback_setup(GLFWwindow& w) -> void
{
	static auto callback_window_focus = [](GLFWwindow* window, const int focused)
	{
		std::println(stdout, "[FOCUS]: window: 0x{:x}, focused: {}", reinterpret_cast<std::uintptr_t>(window), focused != 0);

		if (g_glfw_callback_window_focus)
		{
			g_glfw_callback_window_focus(window, focused);
		}
	};
	static auto callback_window_cursor_enter = [](GLFWwindow* window, const int entered)
	{
		std::println(
			stdout,
			"[CURSOR]: window: 0x{:x}, entered: {}",
			reinterpret_cast<std::uintptr_t>(window),
			entered != 0
		);

		if (g_glfw_callback_window_cursor_enter)
		{
			g_glfw_callback_window_cursor_enter(window, entered);
		}
	};
	static auto callback_window_cursor_position = [](GLFWwindow* window, const double x, const double y)
	{
		std::println(stdout, "[CURSOR]: window: 0x{:x}, x: {}, y: {}", reinterpret_cast<std::uintptr_t>(window), x, y);

		g_device_event_queue.mouse_move(static_cast<float>(x), static_cast<float>(y));

		if (g_glfw_callback_window_cursor_position)
		{
			g_glfw_callback_window_cursor_position(window, x, y);
		}
	};
	static auto callback_window_mouse_button = [](GLFWwindow* window, const int button, const int action, const int mods)
	{
		std::println(
			stdout,
			"[MOUSE]: window: 0x{:x}, button: [{}], action: [{}], mods: [{}]",
			reinterpret_cast<std::uintptr_t>(window),
			meta::to_string(static_cast<MouseButton>(button)),
			meta::to_string(static_cast<MouseAction>(action)),
			meta::to_string(static_cast<MouseMod>(mods))
		);

		const auto status = static_cast<MouseAction>(action) == MouseAction::PRESS ? io::MouseButtonStatus::PRESS : io::MouseButtonStatus::RELEASE;
		switch (static_cast<MouseButton>(button))
		{
			case MouseButton::LEFT:
			{
				g_device_event_queue.mouse_button(io::MouseButton::LEFT, status);
				break;
			}
			case MouseButton::MIDDLE:
			{
				g_device_event_queue.mouse_button(io::MouseButton::MIDDLE, status);
				break;
			}
			case MouseButton::RIGHT:
			{
				g_device_event_queue.mouse_button(io::MouseButton::RIGHT, status);
				break;
			}
			case MouseButton::X1:
			{
				g_device_event_queue.mouse_button(io::MouseButton::X1, status);
				break;
			}
			case MouseButton::X2:
			{
				g_device_event_queue.mouse_button(io::MouseButton::X2, status);
				break;
			}
			default:
			{
				break;
			}
		}

		if (g_glfw_callback_window_mouse_button)
		{
			g_glfw_callback_window_mouse_button(window, button, action, mods);
		}
	};
	static auto callback_window_scroll = [](GLFWwindow* window, const double x, const double y)
	{
		std::println(stdout, "[MOUSE SCROLL]: window: 0x{:x}, x: {}, y: {}", reinterpret_cast<std::uintptr_t>(window), x, y);

		g_device_event_queue.mouse_wheel(static_cast<float>(x), static_cast<float>(y));

		if (g_glfw_callback_window_scroll)
		{
			g_glfw_callback_window_scroll(window, x, y);
		}
	};
	static auto callback_window_key = [](GLFWwindow* window, const int key_code, const int scan_code, const int action, const int mods)
	{
		std::println(
			stdout,
			"[KEYBOARD]: window: 0x{:x}, key_code: [{}]({}), scan_code: {}, action: {}, mods: {}",
			reinterpret_cast<std::uintptr_t>(window),
			meta::to_string(static_cast<KeyboardKeyCode>(key_code)),
			key_code,
			scan_code,
			meta::to_string(static_cast<KeyboardAction>(action)),
			meta::to_string(static_cast<KeyboardMod>(mods))
		);

		if (g_glfw_callback_window_key)
		{
			g_glfw_callback_window_key(window, key_code, scan_code, action, mods);
		}
	};
	static auto callback_window_char = [](GLFWwindow* window, const unsigned int codepoint)
	{
		std::println(stdout, "[KEYBOARD]: window: 0x{:x}, codepoint: 0x{:x}", reinterpret_cast<std::uintptr_t>(window), codepoint);

		if (g_glfw_callback_window_char)
		{
			g_glfw_callback_window_char(window, codepoint);
		}
	};
	static auto callback_window_monitor = [](GLFWmonitor* monitor, const int event)
	{
		std::println(stdout, "[MONITOR]: monitor: 0x{:x}, event: {}", reinterpret_cast<std::uintptr_t>(monitor), event);

		if (g_glfw_callback_window_monitor)
		{
			g_glfw_callback_window_monitor(monitor, event);
		}
	};

	g_glfw_callback_window_focus = glfwSetWindowFocusCallback(&w, callback_window_focus);
	g_glfw_callback_window_cursor_enter = glfwSetCursorEnterCallback(&w, callback_window_cursor_enter);
	g_glfw_callback_window_cursor_position = glfwSetCursorPosCallback(&w, callback_window_cursor_position);
	g_glfw_callback_window_mouse_button = glfwSetMouseButtonCallback(&w, callback_window_mouse_button);
	g_glfw_callback_window_scroll = glfwSetScrollCallback(&w, callback_window_scroll);
	g_glfw_callback_window_key = glfwSetKeyCallback(&w, callback_window_key);
	g_glfw_callback_window_char = glfwSetCharCallback(&w, callback_window_char);
	g_glfw_callback_window_monitor = glfwSetMonitorCallback(callback_window_monitor);
}
