// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.functional;

export import :type_list;
export import :value_list;
export import :functor;
export import :aligned_union;
export import :function_ref;
export import :math;

#else
#include <prometheus/macro.hpp>
#include <functional/type_list.hpp>
#include <functional/value_list.hpp>
#include <functional/functor.hpp>
#include <functional/aligned_union.hpp>
#include <functional/function_ref.hpp>
#include <functional/math.hpp>
#endif
