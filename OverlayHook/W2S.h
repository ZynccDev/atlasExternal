#pragma once
#include "../Misc/structures.h"
#include "../Offsets/offsets.h"
#include "../Globals.h"

inline Vectors::Vector2 WorldToScreen(const Vectors::Vector3& world) {
    Matrixes::Matrix4 view = Memory->read<Matrixes::Matrix4>(Globals::Roblox::VisualEngine + offsets::viewmatrix);

    Vectors::Vector4 clip;
    clip.x = world.x * view.data[0] + world.y * view.data[1] + world.z * view.data[2] + view.data[3];
    clip.y = world.x * view.data[4] + world.y * view.data[5] + world.z * view.data[6] + view.data[7];
    clip.z = world.x * view.data[8] + world.y * view.data[9] + world.z * view.data[10] + view.data[11];
    clip.w = world.x * view.data[12] + world.y * view.data[13] + world.z * view.data[14] + view.data[15];

    if (clip.w < 0.1f)
        return { -1.f, -1.f };

    float invW = 1.0f / clip.w;
    Vectors::Vector3 ndc = { clip.x * invW, clip.y * invW, clip.z * invW };

    HWND hwnd = FindWindowA(NULL, "Roblox");
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    POINT point = { 0, 0 };
    ClientToScreen(hwnd, &point);
    float width = static_cast<float>(clientRect.right);
    float height = static_cast<float>(clientRect.bottom);

    return {
        (width / 2.0f) * ndc.x + (width / 2.0f) + point.x,
        -(height / 2.0f) * ndc.y + (height / 2.0f) + point.y
    };
}