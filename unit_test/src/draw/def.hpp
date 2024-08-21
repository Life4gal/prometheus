#pragma once

#if GAL_PROMETHEUS_USE_MODULE
import gal.prometheus;
#else
#include <prometheus.ixx>
#endif

#include <source_location>
#include <chrono>
#include <print>
#include <cstdio>

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

inline auto print_time(const std::source_location& location = std::source_location::current()) noexcept -> void
{
	std::println(stdout, "[{:%r}] {}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()), location.function_name());
}
