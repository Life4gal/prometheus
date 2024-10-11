// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:meta;

export import :meta.name;
export import :meta.string;
export import :meta.enumeration;
export import :meta.member;
export import :meta.to_string;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <meta/name.ixx>
#include <meta/string.ixx>
#include <meta/enumeration.ixx>
#include <meta/member.ixx>
#include <meta/to_string.ixx>

#endif
