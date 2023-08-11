// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.infrastructure:runtime_error;

export import :runtime_error.terminate_message;
export import :runtime_error.exception;
export import :runtime_error.error_code;

import :runtime_error.impl;
