// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if not defined(CLP_USE_EXPECTED)
#if defined(GAL_PROMETHEUS_COMPILER_DEBUG)
#define CLP_USE_EXPECTED 1
#else
#define CLP_USE_EXPECTED 0
#endif
#endif

#include <command_line_parser/option.hpp>
#include <command_line_parser/parser.hpp>
