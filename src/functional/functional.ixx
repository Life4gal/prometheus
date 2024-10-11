// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:functional;

export import :functional.type_list;
export import :functional.value_list;
export import :functional.functor;
export import :functional.aligned_union;
export import :functional.function_ref;
export import :functional.math;
export import :functional.flag;
export import :functional.function_signature;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <functional/type_list.ixx>
#include <functional/value_list.ixx>
#include <functional/functor.ixx>
#include <functional/aligned_union.ixx>
#include <functional/function_ref.ixx>
#include <functional/math.ixx>
#include <functional/flag.ixx>
#include <functional/function_signature.ixx>

#endif
