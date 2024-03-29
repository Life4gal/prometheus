#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.meta;

export import :string;
export import :name;
export import :enum_name;
export import :member_name;

#else
#include <meta/string.hpp>
#include <meta/name.hpp>
#include <meta/enum_name.hpp>
#include <meta/member_name.hpp>
#endif
