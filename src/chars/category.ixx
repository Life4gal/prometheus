// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:category;

import gal.prometheus.infrastructure;

export namespace gal::prometheus::chars
{
#if GAL_PROMETHEUS_WORKAROUND_MODULE_EXPORT_CTAD
	constexpr infrastructure::fixed_string char_map_category_ascii{"ascii"};
	constexpr infrastructure::fixed_string char_map_category_cp_1252{"cp-1252"};
	constexpr infrastructure::fixed_string char_map_category_utf_8{"utf-8"};
#else
	constexpr infrastructure::fixed_string<sizeof("ascii")> char_map_category_ascii{"ascii"};
	constexpr infrastructure::fixed_string<sizeof("cp-1252")> char_map_category_cp_1252{"cp-1252"};
	constexpr infrastructure::fixed_string<sizeof("utf-8")>	  char_map_category_utf_8{"utf-8"};
#endif
}
