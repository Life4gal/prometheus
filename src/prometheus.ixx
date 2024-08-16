#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus;

export import gal.prometheus.chars;
export import gal.prometheus.command_line_parser;
export import gal.prometheus.concurrency;
export import gal.prometheus.coroutine;
export import gal.prometheus.error;
export import gal.prometheus.functional;
export import gal.prometheus.gui;
export import gal.prometheus.memory;
export import gal.prometheus.meta;
export import gal.prometheus.numeric;
export import gal.prometheus.primitive;
export import gal.prometheus.state_machine;
export import gal.prometheus.string;
export import gal.prometheus.unit_test;
export import gal.prometheus.wildcard;

#else
#pragma once

#include <chars/chars.ixx>
#include <command_line_parser/command_line_parser.ixx>
#include <concurrency/concurrency.ixx>
#include <coroutine/coroutine.ixx>
#include <error/error.ixx>
#include <functional/functional.ixx>
#include <gui/gui.ixx>
#include <memory/memory.ixx>
#include <meta/meta.ixx>
#include <numeric/numeric.ixx>
#include <primitive/primitive.ixx>
#include <state_machine/state_machine.ixx>
#include <string/string.ixx>
#include <unit_test/unit_test.ixx>
#include <wildcard/wildcard.ixx>

#endif
