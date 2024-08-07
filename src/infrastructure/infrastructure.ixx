// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.infrastructure;

export import :state_machine;
export import :unit_test;
export import :command_line_parser;

#else
#pragma once

#include <infrastructure/state_machine.ixx>
#include <infrastructure/unit_test.ixx>
#include <infrastructure/command_line_parser.ixx>

#endif
