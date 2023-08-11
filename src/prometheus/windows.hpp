// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#define NOMINMAX

#ifndef WIN32_NO_STATUS
	#define WIN32_NO_STATUS
#endif

#include <Windows.h>
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
