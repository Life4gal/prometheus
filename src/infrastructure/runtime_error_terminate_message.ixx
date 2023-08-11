// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.infrastructure:runtime_error.terminate_message;

import std;

export namespace gal::prometheus::infrastructure
{
	inline std::atomic<const char*> g_terminate_reason{nullptr};
}// namespace gal::prometheus::infrastructure
