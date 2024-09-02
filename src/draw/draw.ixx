// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.draw;

export import :font;
export import :draw_list;

export import :surface;
export import :style;

export import :element;

#else
#pragma once

#include <draw/font.ixx>
#include <draw/draw_list.ixx>

#include <draw/surface.ixx>
#include <draw/style.ixx>

#include <draw/element.ixx>

#endif
