#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus;

// export import gal.prometheus.chars;
export import gal.prometheus.concurrency;
export import gal.prometheus.coroutine;
export import gal.prometheus.error;
export import gal.prometheus.functional;
export import gal.prometheus.infrastructure;
export import gal.prometheus.memory;
export import gal.prometheus.meta;
export import gal.prometheus.numeric;
export import gal.prometheus.primitive;
export import gal.prometheus.string;

#else
#pragma once

#include <concurrency/concurrency.ixx>
#include <coroutine/coroutine.ixx>
#include <error/error.ixx>
#include <functional/functional.ixx>
#include <infrastructure/infrastructure.ixx>
#include <memory/memory.ixx>
#include <meta/meta.ixx>
#include <numeric/numeric.ixx>
#include <primitive/primitive.ixx>
#include <string/string.ixx>

#endif
