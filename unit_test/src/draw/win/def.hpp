#pragma once

#include <source_location>
#include <print>
#include <cstdio>

#include <draw/draw.hpp>

#include <comdef.h>

struct d3d_vertex_type
{
	float position[2];
	float uv[2];
	std::uint32_t color;
};

using d3d_index_type = gal::prometheus::draw::DrawList::index_type;
using d3d_projection_matrix_type = float[4][4];

static_assert(sizeof(gal::prometheus::draw::DrawList::vertex_type) == sizeof(d3d_vertex_type));
static_assert(sizeof(gal::prometheus::draw::DrawList::index_type) == sizeof(d3d_index_type));

template<bool Abort = true>
auto check_hr_error(
	const HRESULT hr,
	const std::source_location& location = std::source_location::current()
) -> std::conditional_t<Abort, void, bool>
{
	if (SUCCEEDED(hr))
	{
		if constexpr (Abort)
		{
			return;
		}
		else
		{
			return true;
		}
	}

	const _com_error err(hr);
	std::println(stderr, "Error: {} --- at {}:{}", err.ErrorMessage(), location.file_name(), location.line());

	if constexpr (Abort)
	{
		#if defined(DEBUG) or defined(_DEBUG)
		__debugbreak();
		#endif
		std::abort();
	}
	else
	{
		return false;
	}
}
