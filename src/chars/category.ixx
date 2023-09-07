// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.chars:category;

import gal.prometheus.infrastructure;

export namespace gal::prometheus::chars
{
	constexpr infrastructure::basic_fixed_string char_map_category_ascii{"ascii"};
	constexpr infrastructure::basic_fixed_string char_map_category_cp_1252{"cp-1252"};
	constexpr infrastructure::basic_fixed_string char_map_category_utf_8{"utf-8"};
	constexpr infrastructure::basic_fixed_string char_map_category_utf_16{"utf-16"};
}
