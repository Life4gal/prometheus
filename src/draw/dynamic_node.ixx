// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:dynamic_node;

import std;
import gal.prometheus.io;
GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

import :node;
import :animation;

#else
#pragma once

#include <memory>
#include <vector>
#include <algorithm>

#include <prometheus/macro.hpp>
#include <io/io.ixx>
#include <draw/node.ixx>
#include <draw/animation.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::draw)
{
	namespace impl
	{
		class Mouse
		{
		public:
			Mouse(const Mouse&) noexcept = default;
			auto operator=(const Mouse&) noexcept -> Mouse& = default;
			Mouse(Mouse&&) noexcept = default;
			auto operator=(Mouse&&) noexcept -> Mouse& = default;

			Mouse() noexcept = default;

			virtual ~Mouse() noexcept = 0;
		};

		class DynamicNode;
	}

	using mouse_type = std::unique_ptr<impl::Mouse>;

	using component_type = std::shared_ptr<impl::DynamicNode>;
	using components_type = std::vector<component_type>;

	namespace impl
	{
		class DynamicNode
		{
		public:
			using size_type = components_type::size_type;
			using event_type = io::DeviceEvent;

		protected:
			components_type children_;

		private:
			DynamicNode* parent_;

		public:
			DynamicNode(const DynamicNode&) noexcept = delete;
			auto operator=(const DynamicNode&) noexcept -> DynamicNode& = delete;
			DynamicNode(DynamicNode&&) noexcept = delete;
			auto operator=(DynamicNode&&) noexcept -> DynamicNode& = delete;

			explicit DynamicNode() noexcept
				: parent_{nullptr} {}

			explicit DynamicNode(components_type children) noexcept
				: children_{std::move(children)},
				  parent_{nullptr} {}

			virtual ~DynamicNode() noexcept = 0;

			// =================================

			[[nodiscard]] auto parent(this auto&& self) noexcept -> auto*
			{
				return std::forward<decltype(self)>(self).parent_;
			}

			[[nodiscard]] auto children_size() const noexcept -> size_type
			{
				return children_.size();
			}

			[[nodiscard]] auto child_at(this auto&& self, const size_type index) noexcept -> component_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < children_size());
				return std::forward<decltype(self)>(self).children_[index];
			}

			auto detach_from_parent(DynamicNode* new_parent = nullptr) noexcept -> void
			{
				if (auto* old_parent = std::exchange(parent_, new_parent);
					old_parent)
				{
					const auto it = std::ranges::find(old_parent->children_, this, &component_type::get);
					old_parent->children_.erase(it);
				}
			}

			auto detach_children() noexcept -> void
			{
				std::ranges::for_each(children_, [](auto& child) noexcept -> void { child->parent_ = nullptr; });
				children_.clear();
			}

			auto attach_child(component_type child) noexcept -> void
			{
				child->detach_from_parent(this);
				children_.push_back(std::move(child));
			}

			// =================================

			[[nodiscard]] virtual auto render() noexcept -> element_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(children_.size() == 1);

				return children_.front()->render();
			}

			// =================================

			[[nodiscard]] virtual auto on_event(const event_type event) noexcept -> bool
			{
				return std::ranges::any_of(
					children_,
					[event](auto* child) noexcept -> bool { return child->on_event(event); },
					&component_type::get
				);
			}

			virtual auto on_animation(const animation_parameter_type parameter) noexcept -> void
			{
				std::ranges::for_each(
					children_,
					[parameter](auto* child) noexcept -> void { child->on_animation(parameter); },
					&component_type::get
				);
			}

			// =================================

			[[nodiscard]] virtual auto focusable() const noexcept -> bool
			{
				return std::ranges::any_of(
					children_,
					[](auto* child) noexcept -> bool { return child->focusable(); },
					&component_type::get
				);
			}

			[[nodiscard]] auto focused() const noexcept -> bool
			{
				const auto* current = this;
				while (current and current->activated())
				{
					current = current->parent_;
				}

				return current == nullptr and focusable();
			}

			auto take_focus() noexcept -> void
			{
				auto* child = this;
				while (auto* parent = child->parent_)
				{
					parent->active_child(*child);
					child = parent;
				}
			}

			[[nodiscard]] auto activated() const noexcept -> bool
			{
				return parent_ == nullptr or parent_->activated_child().get() == this;
			}

			[[nodiscard]] virtual auto activated_child() noexcept -> component_type
			{
				if (const auto it = std::ranges::find_if(
						children_,
						[](auto* child) noexcept -> bool { return child->focusable(); },
						&component_type::get
					);
					it != std::ranges::end(children_))
				{
					return it.operator*();
				}

				return nullptr;
			}

			virtual auto active_child(DynamicNode& child) noexcept -> void
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(std::ranges::contains(children_, &child, &component_type::get));
			}

			auto active_child(const component_type& child) noexcept -> void
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(child != nullptr);
				active_child(*child);
			}

		protected:
			[[nodiscard]] auto try_capture_mouse(event_type event) noexcept -> mouse_type;
		};
	}
}
