#pragma once
#include "../globals.h"
#include "../Misc/Console.h"

#include <thread>
#include <vector>

// ass tp handler, if it works it works ig:sob:

// 5.0
inline void TPHandler()
{
    const uintptr_t base = Memory->getBaseAddress();
    static uintptr_t lastdmaddy = 0;
    static int lastplaceid = 0;

    while (true)
    {
        Sleep(300 + (rand() % 400));

        try
        {
            uintptr_t fakiedatamodel = Memory->read<uintptr_t>(base + offsets::FakeDataModelPointer);
            if (!fakiedatamodel) continue;

            uintptr_t datamodeladdy = Memory->read<uintptr_t>(fakiedatamodel + offsets::FakeDataModelToDataModel);
            if (!datamodeladdy) continue;

            RobloxInstance dataModel(datamodeladdy);
            std::string datamodelname = dataModel.Name();
            int placeid = Memory->read<int>(datamodeladdy + offsets::PlaceId);

            if (datamodeladdy != lastdmaddy || placeid != lastplaceid)
            {
                //log("[TP Handler] PlaceId: " + std::to_string(placeid), 0); // just the game id 

                lastdmaddy = datamodeladdy;
                lastplaceid = placeid;

                Globals::Roblox::DataModel = dataModel;
                Globals::Roblox::lastPlaceID = placeid;
                Globals::Roblox::VisualEngine = 0;
                Globals::Roblox::Workspace = RobloxInstance(0);
                Globals::Roblox::Players = RobloxInstance(0);
                Globals::Roblox::Camera = RobloxInstance(0);
                Globals::Roblox::LocalPlayer = RobloxInstance(0);
                Globals::Caches::CachedPlayers.clear();
                Globals::Caches::CachedPlayerObjects.clear();

                Sleep(1800 + (rand() % 1200));  // 1.8–3s max
            }

            if (Globals::Roblox::Workspace.address == 0)
                Globals::Roblox::Workspace = dataModel.FindFirstChildWhichIsA("Workspace");

            if (Globals::Roblox::Players.address == 0)
                Globals::Roblox::Players = dataModel.FindFirstChildWhichIsA("Players");

            if (Globals::Roblox::Camera.address == 0 && Globals::Roblox::Workspace.address)
                Globals::Roblox::Camera = Globals::Roblox::Workspace.FindFirstChildWhichIsA("Camera");

            if (Globals::Roblox::VisualEngine == 0)
            {
                uintptr_t ve = Memory->read<uintptr_t>(base + offsets::VisualEnginePointer);
                if (ve && ve != 0xFFFFFFFFFFFFFFFF)
                    Globals::Roblox::VisualEngine = ve;
            }

            if (Globals::Roblox::Players.address && Globals::Roblox::LocalPlayer.address == 0)
            {
                uintptr_t lppointer = Memory->read<uintptr_t>(Globals::Roblox::Players.address + offsets::LocalPlayer);
                if (lppointer && lppointer != 0xFFFFFFFFFFFFFFFF)
                {
                    RobloxInstance lp(lppointer);
                    std::string name = lp.Name();
                    if (lp.address && !name.empty() && name != "Players")
                    {
                        Globals::Roblox::LocalPlayer = lp;
                        //log("LocalPlayer: " + name, 0); // this is only to see if we actually get lp
                    }
                }
            }
        }
        catch (...)
        {
            Sleep(1000);
        }
    }
}