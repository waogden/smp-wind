#pragma once

#pragma warning(push)
#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include <spdlog/sinks/basic_file_sink.h>
#pragma warning(pop)

#include <array>
#include <atomic>
#include <barrier>
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <semaphore>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <Windows.h>
#undef min
#undef max

// Must match the define FSMP is built with, since we share Bullet objects with it.
#ifndef BT_USE_SSE_IN_API
#	define BT_USE_SSE_IN_API
#endif
#include "BulletDynamics/Dynamics/btDynamicsWorld.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

using namespace std::literals;

namespace logger = SKSE::log;

// printf-style shims for the original SKSE logging macros
namespace wind::detail
{
	inline std::string vformat(const char* fmt, ...)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		std::vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
		return buf;
	}
}

#define _MESSAGE(...) ::logger::info("{}", ::wind::detail::vformat(__VA_ARGS__))
#define _WARNING(...) ::logger::warn("{}", ::wind::detail::vformat(__VA_ARGS__))
#define _ERROR(...) ::logger::error("{}", ::wind::detail::vformat(__VA_ARGS__))
#define _FATALERROR(...) ::logger::critical("{}", ::wind::detail::vformat(__VA_ARGS__))
