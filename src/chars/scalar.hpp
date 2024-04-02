// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:scalar;

export import :scalar.ascii;
export import :scalar.utf8;
export import :scalar.utf16;
export import :scalar.utf32;

#else
#include <chars/scalar_ascii.hpp>
#include <chars/scalar_utf8.hpp>
#include <chars/scalar_utf16.hpp>
#include <chars/scalar_utf32.hpp>
#endif
