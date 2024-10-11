// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

export module gal.prometheus;

export import :meta;
export import :error;
export import :functional;
export import :memory;
export import :numeric;
export import :primitive;
export import :concurrency;
export import :coroutine;
export import :string;
export import :io;
export import :chars;
export import :command_line_parser;
export import :unit_test;
export import :wildcard;
export import :state_machine;
export import :draw;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <chars/chars.ixx>
#include <command_line_parser/command_line_parser.ixx>
#include <concurrency/concurrency.ixx>
#include <coroutine/coroutine.ixx>
#include <draw/draw.ixx>
#include <error/error.ixx>
#include <functional/functional.ixx>
#include <io/io.ixx>
#include <memory/memory.ixx>
#include <meta/meta.ixx>
#include <numeric/numeric.ixx>
#include <primitive/primitive.ixx>
#include <state_machine/state_machine.ixx>
#include <string/string.ixx>
#include <unit_test/unit_test.ixx>
#include <wildcard/wildcard.ixx>

#endif
