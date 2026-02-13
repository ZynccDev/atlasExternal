#pragma once

#include "../options.h"
#include "../globals.h"
#include "../Memory/MemoryManager.h"

#include <thread>
#include <iostream>
inline void MiscLoop()
{
	static auto character = Globals::Roblox::LocalPlayer.Character();
	static auto humanoid = character.FindFirstChildWhichIsA("Humanoid");

	while (true)
	{
		character = Globals::Roblox::LocalPlayer.Character();
		humanoid = character.FindFirstChildWhichIsA("Humanoid");

		humanoid.SetWalkspeed(Options::Misc::Walkspeed);
		humanoid.SetJumpPower(Options::Misc::JumpPower);
		//Globals::Roblox::Camera.SetFOV(Options::Misc::FOV);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

inline void AntiAFK()
{
	while (true)
	{
		if (Options::Misc::AntiAFK)
		{
			Memory->write<float>(offsets::ForceNewAFKDuration, 9999.f);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

	}
}

inline void Noclip() 
{
	if (!Options::Misc::Noclip) 
		return;

	if (Options::Misc::Noclip) 
	{
		static auto character = Globals::Roblox::LocalPlayer.Character();
		auto children = character.GetChildren();
		for (auto& part : children) {

			std::string className = RobloxInstance::GetTheClassName(part.address);
			if (className == "Part" || className == "MeshPart") {
				uintptr_t primitive = Memory->read<uintptr_t>(part.address + offsets::Primitive);
				if (!primitive) continue;

				BYTE val = Memory->read<BYTE>(primitive + offsets::CanCollide);
				if (Options::Misc::Noclip)
					val &= ~0x08;
				else
					val |= 0x08;
				Memory->write<BYTE>(primitive + offsets::CanCollide, val);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

inline void Desync()
{
	if (!Options::Misc::Desync)
		return;

	if (Options::Misc::Desync)
	{
		uintptr_t base = (uintptr_t)GetModuleHandleA("RobloxPlayerBeta.exe");
		//BYTE val = Memory->read<BYTE>(base + offsets::fastflags::NextGenReplicatorEnabledWrite4);
		//if (Options::Misc::Noclip)
			//val &= ~0x08;
		//else
			//val |= 0x08;
		//Memory->write<BYTE>(base + offsets::fastflags::NextGenReplicatorEnabledWrite4, val);
	}
}
