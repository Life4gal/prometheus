// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.error:debug;

import std;

export namespace gal::prometheus::error
{
	auto debug_break(const char* message) noexcept -> void;
} // namespace gal::prometheus::error
