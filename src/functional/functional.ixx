// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.functional;

export import :type_list;
export import :value_list;
export import :functor;
export import :aligned_union;
export import :function_ref;
export import :math;
export import :flag;
export import :function_signature;

#else
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
