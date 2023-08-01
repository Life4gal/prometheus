// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)

#define NOMINMAX

#ifndef WIN32_NO_STATUS
#define WIN32_NO_STATUS
#endif

#include <windows.h>
// #undef WIN32_NO_STATUS
// #include <ntstatus.h>
// #define WIN32_NO_STATUS
// #include <debugapi.h>
// #include <intrin.h>
// #include <shellapi.h>
// #include <winuser.h>
//
// // Window's registry.
// #include <Uxtheme.h>
// #include <winreg.h>
//
// // Cryptography
// #include <bcrypt.h>
//
// // Threading
// #include <synchapi.h>
//
// // File IO
// #include <ShlObj_core.h>
//
// // Networking
// #define IN
// #define OUT
// #include <WinSock2.h>
//
// // DirectX.
// #include <ddraw.h>
// #include <dwmapi.h>
// #include <dxgi.h>
// #include <windowsx.h>
//
// // initguid allows some of the header files to define actual implementations of the GUID.
// // However this is incompatible with other headers which causes some values to become undefined.
// #include <initguid.h>
//
// // Multimedia and audio.
// #include <audioclient.h>
// #include <endpointvolume.h>
// #include <functiondiscoverykeys_devpkey.h>
// #include <ks.h>
// #include <ksmedia.h>
// #include <mmddk.h>
// #include <mmdeviceapi.h>
// #include <mmeapi.h>
// #include <mmreg.h>
// #include <mmsystem.h>
// #include <propsys.h>
// #include <winioctl.h>
//
// // The windows headers create all sort of insane macros.
// #undef IN
// #undef OUT
// #undef small

#include <prometheus/infrastructure/cast/numeric.hpp>

namespace gal::prometheus::platform
{
	template<typename WString>
		requires requires(WString& s)
		{
			s.resize(typename WString::size_type{1});
			s.data();
		}
	[[nodiscard]] auto string_to_wstring(const std::string_view string) -> WString
	{
		const auto in_length  = cast::narrow_cast<int>(string.size());
		const auto out_length = MultiByteToWideChar(CP_UTF8, 0, string.data(), in_length, nullptr, 0);

		if (out_length == 0) { throw debug::RuntimeError{"string_to_wstring failed!"}; }

		WString result{};
		result.resize(cast::narrow_cast<typename WString::size_type>(out_length));
		MultiByteToWideChar(CP_UTF8, 0, string.data(), in_length, result.data(), out_length);
		return result;
	}

	template<typename String>
		requires requires(String& s)
		{
			s.resize(typename String::size_type{1});
			s.data();
		}
	[[nodiscard]] auto wstring_to_string(const std::wstring_view string) -> String
	{
		const auto in_length  = cast::narrow_cast<int>(string.size());
		const auto out_length = WideCharToMultiByte(CP_UTF8, 0, string.data(), in_length, nullptr, 0, nullptr, nullptr);

		if (out_length == 0) { throw debug::RuntimeError{"wstring_to_string failed!"}; }

		String result{};
		result.resize(cast::narrow_cast<typename String::size_type>(out_length));
		WideCharToMultiByte(CP_UTF8, 0, string.data(), in_length, result.data(), out_length, nullptr, nullptr);
		return result;
	}
}

#endif
