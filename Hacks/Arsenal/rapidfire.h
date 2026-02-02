#pragma once

#include <thread>
#include <chrono>
#include <Windows.h>

#include "../../Globals.h"
#include "../../Options.h"
// skidded but ill make my own one later
void ArsenalRapidFire() {
    while (true) {
        if (!Options::Arsenal::RapidFire) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        if (Options::Arsenal::RapidFire) {
            try {
                auto character = Globals::Roblox::LocalPlayer.Character();
                if (character.address == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                auto children = character.GetChildren();
                for (auto& child : children) {
                    if (child.Class() == "Tool") {
                        RobloxInstance gun_script = child.FindFirstChild("GunScript_Server");
                        if (gun_script.address != 0) {
                            RobloxInstance firerate = gun_script.FindFirstChild("Firerate");
                            if (firerate.address != 0) {
								Memory->write<double>(firerate.address + offsets::Value, Options::Arsenal::RapidFireDelay);
                                std::cout << "holy tuff rapid fire" << std::endl;
                            }
                        }
                    }
                }
            }
            catch (...) {}
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

