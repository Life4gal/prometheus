// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:icelake;

export import :icelake.ascii;
export import :icelake.utf8;
export import :icelake.utf16;
export import :icelake.utf32;

#else
#include <chars/icelake_ascii.hpp>
#include <chars/icelake_utf8.hpp>
#include <chars/icelake_utf16.hpp>
#include <chars/icelake_utf32.hpp>
#endif
