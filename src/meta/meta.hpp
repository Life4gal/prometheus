// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.meta;

export import :string;
export import :name;
export import :enum_name;
export import :member_name;
export import :to_string;

#else
#include <meta/string.hpp>
#include <meta/name.hpp>
#include <meta/enum_name.hpp>
#include <meta/member_name.hpp>
#include <meta/to_string.hpp>
#endif
