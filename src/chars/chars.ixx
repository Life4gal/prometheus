// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars;

export import :encoding;
export import :scalar;

#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
export import :icelake;
#endif

#else
#include <chars/encoding.ixx>
#include <chars/scalar.ixx>

#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
#include <chars/icelake.ixx>
#endif

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::chars)
{
	
}
