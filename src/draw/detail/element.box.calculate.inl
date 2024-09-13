#pragma once

struct element_size
{
	float min_size = 0;
	float flex_grow = 0;
	float flex_shrink = 0;

	float size = 0;
};

template<std::ranges::output_range<element_size> Range>
constexpr auto calculate_grow(Range& range, float extra_space, float flex_grow_sum) noexcept -> void
{
	std::ranges::for_each(
		range,
		[extra_space, flex_grow_sum](element_size& element) mutable noexcept -> void
		{
			const auto added_space = extra_space * element.flex_grow / std::ranges::max(flex_grow_sum, 1.f);

			extra_space -= added_space;
			flex_grow_sum -= element.flex_grow;
			element.size = element.min_size + added_space;
		}
	);
}

template<std::ranges::output_range<element_size> Range>
constexpr auto calculate_shrink_easy(Range& range, float extra_space, float flex_shrink_sum) noexcept -> void
{
	std::ranges::for_each(
		range,
		[extra_space, flex_shrink_sum](element_size& element) mutable noexcept -> void
		{
			const auto added_space = extra_space * element.min_size * element.flex_shrink / std::ranges::max(flex_shrink_sum, 1.f);

			extra_space -= added_space;
			flex_shrink_sum -= element.flex_shrink * element.min_size;
			element.size = element.min_size + added_space;
		}
	);
}

template<std::ranges::output_range<element_size> Range>
constexpr auto calculate_shrink_hard(Range& range, float extra_space, float size) noexcept -> void
{
	std::ranges::for_each(
		range,
		[extra_space, size](element_size& element) mutable noexcept -> void
		{
			if (element.flex_shrink != 0)
			{
				element.size = 0;
				return;
			}

			const auto added_space = extra_space * element.min_size / std::ranges::max(size, 1.f);

			extra_space -= added_space;
			size -= element.min_size;
			element.size = element.min_size + added_space;
		}
	);
}

template<std::ranges::output_range<element_size> Range>
constexpr auto calculate(Range& range, const float target_size) noexcept -> void
{
	float size = 0;
	float flex_grow_sum = 0;
	float flex_shrink_sum = 0;
	float flex_shrink_size = 0;

	std::ranges::for_each(
		range,
		[&](const element_size& element) noexcept -> void
		{
			size += element.min_size;
			flex_grow_sum += element.flex_grow;
			if (element.flex_shrink != 0)
			{
				flex_shrink_sum += element.min_size * element.flex_shrink;
				flex_shrink_size += element.min_size;
			}
		}
	);

	if (const auto extra_space = target_size - size;
		extra_space >= 0)
	{
		detail::calculate_grow(range, extra_space, flex_grow_sum);
	}
	else if (flex_shrink_size + extra_space >= 0)
	{
		detail::calculate_shrink_easy(range, extra_space, flex_shrink_sum);
	}
	else
	{
		detail::calculate_shrink_hard(range, extra_space + flex_shrink_size, size - flex_shrink_size);
	}
}
