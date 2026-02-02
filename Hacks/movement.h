#pragma once

#include "../options.h"
#include "../globals.h"
#include "../Memory/MemoryManager.h"

#include "../OverlayHook/imgui/Keybind.h"

#include <thread>
#include <iostream>

void Fly()
{
    if (KeyBind::IsPressed(Options::Misc::FlyKey, true))
    {
        Options::Misc::FlyToggled = !Options::Misc::FlyToggled;
    }

    if (Options::Misc::FlyToggled)
    {
        RobloxInstance workspace = Globals::Roblox::DataModel.FindFirstChild("Workspace");
        RobloxInstance camera = Globals::Roblox::Camera;
        auto character = Globals::Roblox::LocalPlayer.Character();
        auto hrp = character.FindFirstChild("HumanoidRootPart");

        float flySpeed{ Options::Misc::FlySpeed };
        auto primitive = Memory->read<uintptr_t>(hrp.address + offsets::Primitive);

        Vectors::Vector3 camPos{ Memory->read<Vectors::Vector3>(camera.address + offsets::CameraPos) };
        Matrixes::RMatrix3 camRot{ Memory->read<Matrixes::RMatrix3>(camera.address + offsets::CameraRotation) };
        Vectors::Vector3 pos{ Memory->read<Vectors::Vector3>(primitive + offsets::Position) };

        Vectors::Vector3 lookVector{ -camRot.data[2], -camRot.data[5], -camRot.data[8] };
        Vectors::Vector3 rightVector{ camRot.data[0], camRot.data[3], camRot.data[6] };

        Vectors::Vector3 moveDirection{};

        if (GetAsyncKeyState('W') & 0x8000)
        {
            moveDirection.x += lookVector.x;
            moveDirection.y += lookVector.y;
            moveDirection.z += lookVector.z;
        }
        if (GetAsyncKeyState('S') & 0x8000)
        {
            moveDirection.x -= lookVector.x;
            moveDirection.y -= lookVector.y;
            moveDirection.z -= lookVector.z;
        }
        if (GetAsyncKeyState('A') & 0x8000)
        {
            moveDirection.x -= rightVector.x;
            moveDirection.y -= rightVector.y;
            moveDirection.z -= rightVector.z;
        }
        if (GetAsyncKeyState('D') & 0x8000)
        {
            moveDirection.x += rightVector.x;
            moveDirection.y += rightVector.y;
            moveDirection.z += rightVector.z;
        }
        if (GetAsyncKeyState(VK_SPACE) & 0x8000)
        {
            moveDirection.y += 1.0f;
        }
        if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
        {
            moveDirection.y -= 1.0f;
        }

        if (moveDirection.x != 0 && moveDirection.y != 0 && moveDirection.z != 0)
        {
            float len{ std::sqrt(moveDirection.x * moveDirection.x + moveDirection.y * moveDirection.y) };
            if (len == 0) return;
            moveDirection.x /= len;
            moveDirection.y /= len;
            moveDirection.z /= len;

            moveDirection.x *= flySpeed;
            moveDirection.y *= flySpeed;
            moveDirection.z *= flySpeed;
        }

        Vectors::Vector3 newPos{ pos.x + moveDirection.x, pos.y + moveDirection.y, pos.z + moveDirection.z };

        Memory->write<Vectors::Vector3>(primitive + offsets::Position, newPos);
        Memory->write<Vectors::Vector3>(primitive + offsets::Velocity, { 0.0f, 0.0f, 0.0f });
    }
}