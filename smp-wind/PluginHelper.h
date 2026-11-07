#pragma once

#include "PluginAPI.h"

namespace hdt
{
	constexpr bool operator<(const PluginInterface::Version& lhs, const PluginInterface::Version& rhs)
	{
		if (lhs.major != rhs.major) {
			return lhs.major < rhs.major;
		}
		if (lhs.minor != rhs.minor) {
			return lhs.minor < rhs.minor;
		}
		return lhs.patch < rhs.patch;
	}

	constexpr bool operator>(const PluginInterface::Version& lhs, const PluginInterface::Version& rhs) { return rhs < lhs; }
	constexpr bool operator>=(const PluginInterface::Version& lhs, const PluginInterface::Version& rhs) { return !(lhs < rhs); }
	constexpr bool operator<=(const PluginInterface::Version& lhs, const PluginInterface::Version& rhs) { return !(rhs < lhs); }

	// Connects to HDT-SMP (FSMP) through the SKSE messaging interface.
	// T must provide interfaceMin/Max, bulletMin/Max and onConnect(PluginInterface*).
	template <typename T>
	class PluginHelper
	{
	public:
		static bool tryConnect()
		{
			auto messaging = SKSE::GetMessagingInterface();
			if (!messaging) {
				_ERROR("ERROR: Failed to get the SKSE messaging interface.");
				return false;
			}
			return messaging->RegisterListener("SKSE", &skseCallback);
		}

	private:
		static void skseCallback(SKSE::MessagingInterface::Message* msg)
		{
			if (msg && msg->type == SKSE::MessagingInterface::kPostLoad) {
				if (SKSE::GetMessagingInterface()->RegisterListener("hdtSMP64", &smpCallback)) {
					_MESSAGE("Connecting to HDT-SMP...");
				} else {
					_ERROR("ERROR: HDT-SMP is not loaded.");
				}
			}
		}

		static void smpCallback(SKSE::MessagingInterface::Message* msg)
		{
			if (msg && msg->type == PluginInterface::MSG_STARTUP && msg->data) {
				auto smp = static_cast<PluginInterface*>(msg->data);
				auto&& info = smp->getVersionInfo();

				_MESSAGE("HDT-SMP interface %d.%d.%d, Bullet %d.%d.%d",
					info.interfaceVersion.major, info.interfaceVersion.minor, info.interfaceVersion.patch,
					info.bulletVersion.major, info.bulletVersion.minor, info.bulletVersion.patch);

				if (info.interfaceVersion >= T::interfaceMin && info.interfaceVersion < T::interfaceMax) {
					if (info.bulletVersion >= T::bulletMin && info.bulletVersion < T::bulletMax) {
						_MESSAGE("Connection established.");
						T::onConnect(smp);
					} else {
						_ERROR("ERROR: Incompatible Bullet version.");
					}
				} else {
					_ERROR("ERROR: Incompatible HDT-SMP interface.");
				}
			}
		}
	};
}
