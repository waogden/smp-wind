#include "pch.h"

#include "PluginHelper.h"

#include "Config.h"
#include "Wind.h"

namespace wind
{
	Config g_config;
	Config g_configDefault;
	Wind g_wind;

	bool registerFunctions(RE::BSScript::IVirtualMachine* vm);
}

namespace
{
	class MyHelper
	{
	public:
		// FSMP's CommonLib-based plugin interface (RE::BSTEventSink listeners)
		inline static hdt::PluginInterface::Version interfaceMin{ 2, 0, 0 };
		inline static hdt::PluginInterface::Version interfaceMax{ 3, 0, 0 };

		inline static hdt::PluginInterface::Version bulletMin{ hdt::PluginInterface::BULLET_VERSION };
		inline static hdt::PluginInterface::Version bulletMax{ hdt::PluginInterface::BULLET_VERSION.major + 1, 0, 0 };

		static void onConnect(hdt::PluginInterface* smp)
		{
			if (auto dir = logger::log_directory()) {
				_MESSAGE("Loading settings...");
				if (wind::g_config.load(*dir / "SMP Wind.ini")) {
					_MESSAGE("Settings loaded.");
				} else {
					_WARNING("WARNING: Failed to load config file. Settings will not be saved.");
				}
			} else {
				_WARNING("WARNING: SKSE log directory not found. Settings will not be saved.");
			}

			wind::g_wind.init(wind::g_config);

			smp->addListener(&wind::g_wind);

			_MESSAGE("Initialisation complete.");
		}
	};

	void initLog()
	{
		auto dir = logger::log_directory();
		if (!dir) {
			return;
		}

		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((*dir / "SMP Wind.log").string(), true);
		auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));
#ifndef NDEBUG
		log->set_level(spdlog::level::debug);
#else
		log->set_level(spdlog::level::info);
#endif
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("%v");
	}
}

namespace Plugin
{
	inline constexpr auto NAME = "SMP Wind"sv;
	inline constexpr REL::Version VERSION{ 2, 4, 0 };
}

// Declared by hand rather than via SKSEPluginInfo/add_commonlibsse_plugin: PluginVersionData
// defaults versionIndependenceEx to kVersionIndependentEx_AddressLibraryV5, which SKSE 2.3+
// (Skyrim 1.7.99+) requires; the declarative path at this CommonLib commit does not set it.
extern "C" __declspec(dllexport) constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);
	v.AuthorName("jgernandt"sv);
	v.UsesAddressLibrary();
	v.UsesNoStructs();  // RE::Sky layout used here is identical pre/post 1.6.629
	return v;
}();

// Pre-AE (1.5.97) SKSE loads plugins via Query
extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* skse, SKSE::PluginInfo* info)
{
	info->infoVersion = SKSE::PluginInfo::kVersion;
	info->name = Plugin::NAME.data();
	info->version = Plugin::VERSION.pack();
	return !skse->IsEditor();
}

extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* skse)
{
	SKSE::Init(skse);
	initLog();

	logger::info("{} v{}", Plugin::NAME, Plugin::VERSION.string());
	logger::info("Skyrim version:\t{}", skse->RuntimeVersion().string());
	logger::info("SKSE version:\t{}\n", REL::Version::unpack(skse->SKSEVersion()).string());

	auto papyrus = SKSE::GetPapyrusInterface();
	if (!papyrus || !papyrus->Register(wind::registerFunctions)) {
		_FATALERROR("Function registration failed. Plugin loading aborted.");
		return false;
	}

	hdt::PluginHelper<MyHelper>::tryConnect();

	return true;
}
