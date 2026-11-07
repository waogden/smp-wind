#include "pch.h"
#include "Config.h"
#include "Wind.h"

namespace wind
{
	extern Config g_config;
	extern Config g_configDefault;
	extern Wind g_wind;

	static bool getBool(RE::StaticFunctionTag*, std::int32_t id)
	{
		if (id >= 0 && id < Config::BOOL_COUNT) {
			return g_config.getb(id);
		}
		else {
			return false;
		}
	}

	static bool getBoolDefault(RE::StaticFunctionTag*, std::int32_t id)
	{
		if (id >= 0 && id < Config::BOOL_COUNT) {
			return g_configDefault.getb(id);
		}
		else {
			return false;
		}
	}

	static void setBool(RE::StaticFunctionTag*, std::int32_t id, bool b)
	{
		if (id >= 0 && id < Config::BOOL_COUNT) {
			g_config.set(id, b);
		}
	}

	static float getFloat(RE::StaticFunctionTag*, std::int32_t id)
	{
		if (id >= 0 && id < Config::FLOAT_COUNT) {
			return g_config.getf(id);
		}
		else {
			return 0.0f;
		}
	}

	static float getFloatDefault(RE::StaticFunctionTag*, std::int32_t id)
	{
		if (id >= 0 && id < Config::FLOAT_COUNT) {
			return g_configDefault.getf(id);
		}
		else {
			return 0.0f;
		}
	}

	static void setFloat(RE::StaticFunctionTag*, std::int32_t id, float f)
	{
		if (id >= 0 && id < Config::FLOAT_COUNT) {
			g_config.set(id, f);
		}
	}

	static std::int32_t getInt(RE::StaticFunctionTag*, std::int32_t id)
	{
		if (id >= 0 && id < Config::INT_COUNT) {
			return g_config.geti(id);
		}
		else {
			return 0;
		}
	}

	static std::int32_t getIntDefault(RE::StaticFunctionTag*, std::int32_t id)
	{
		if (id >= 0 && id < Config::INT_COUNT) {
			return g_configDefault.geti(id);
		}
		else {
			return 0;
		}
	}

	static void setInt(RE::StaticFunctionTag*, std::int32_t id, std::int32_t i)
	{
		if (id >= 0 && id < Config::INT_COUNT) {
			g_config.set(id, static_cast<int>(i));
		}

		if (id == Config::THREADS) {
			wind::g_wind.updateThreadCount();
		}
	}


	bool registerFunctions(RE::BSScript::IVirtualMachine* vm)
	{
		if (!vm) {
			return false;
		}

		constexpr auto CLASS = "JGWD_MCM"sv;

		// Getters were flagged NoWait in the original (callable from tasklets)
		vm->RegisterFunction("GetBool"sv, CLASS, getBool, true);
		vm->RegisterFunction("GetBoolDefault"sv, CLASS, getBoolDefault, true);
		vm->RegisterFunction("SetBool"sv, CLASS, setBool);

		vm->RegisterFunction("GetFloat"sv, CLASS, getFloat, true);
		vm->RegisterFunction("GetFloatDefault"sv, CLASS, getFloatDefault, true);
		vm->RegisterFunction("SetFloat"sv, CLASS, setFloat);

		vm->RegisterFunction("GetInt"sv, CLASS, getInt, true);
		vm->RegisterFunction("GetIntDefault"sv, CLASS, getIntDefault, true);
		vm->RegisterFunction("SetInt"sv, CLASS, setInt);

		return true;
	}
}
