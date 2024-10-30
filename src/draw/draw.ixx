// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

export module gal.prometheus:draw;

export import :draw.def;
export import :draw.font;
export import :draw.draw_list;
export import :draw.theme;
export import :draw.window;
export import :draw.context;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <draw/def.ixx>
#include <draw/font.ixx>
#include <draw/draw_list.ixx>
#include <draw/theme.ixx>
#include <draw/window.ixx>
#include <draw/context.ixx>

#endif
