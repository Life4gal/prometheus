// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.meta;

export import :name;
export import :string;
export import :enumeration;
export import :member;
export import :to_string;

#else
#pragma once

#include <meta/name.ixx>
#include <meta/string.ixx>
#include <meta/enumeration.ixx>
#include <meta/member.ixx>
#include <meta/to_string.ixx>

#endif
