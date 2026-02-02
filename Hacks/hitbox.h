#pragma once

#include "../options.h"
#include "../globals.h"
#include <cmath>
#include <algorithm>
#include "../OverlayHook/imgui/imgui.h"

#pragma comment(lib, "winmm.lib")

void RunHitBoxExpand()
{
	if (Globals::Caches::CachedPlayerObjects.empty())
		return;

	if (!Options::HitboxExpander::Expand) {
		return;
	}

	auto datamodel = Globals::Roblox::DataModel;

    if (Options::HitboxExpander::Expand)
    {
        for (auto& player : Globals::Caches::CachedPlayerObjects)
        {

            auto hrp = player.HumanoidRootPart;

            if (!hrp.address)
            {
                std::cout << "hrp is null\n";
                continue;
            }



            auto size = Memory->read<Vectors::Vector3>(Memory->read<uintptr_t>(hrp.address + offsets::Primitive) + offsets::PartSize);
            std::cout << "Old Size: " << size.x << ", " << size.y << ", " << size.z << std::endl;

                Vectors::Vector3 newsize = { 100.f, 100.f, 100.f };
                Memory->write<Vectors::Vector3>(Memory->read<uintptr_t>(hrp.address + offsets::Primitive) + offsets::PartSize, newsize);
                std::cout << "New Size: " << size.x << ", " << size.y << ", " << size.z << std::endl;
        }
    }
}
