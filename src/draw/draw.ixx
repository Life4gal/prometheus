// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

export module gal.prometheus:draw;

export import :draw.font;
export import :draw.draw_list;
export import :draw.window;

// export import :widgets;

// export import :surface;
// export import :style;
//
// export import :options;
//
// export import :element;
// export import :component;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <draw/font.ixx>
#include <draw/draw_list.ixx>
#include <draw/window.ixx>

// #include <draw/widgets.ixx>

// #include <draw/surface.ixx>
// #include <draw/style.ixx>
//
// #include <draw/detail/options.ixx>
//
// #include <draw/element.ixx>
// #include <draw/component.ixx>

#endif
