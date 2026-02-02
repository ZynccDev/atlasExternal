#pragma once
#include <chrono>
#include <string>
#include "../RPC/Discord-SDK/include/discord_rpc.h"
#include <cstdio>

#include "../Globals.h"

static void handleReady(const DiscordUser* user) {
}

static void handleError(int err, const char* msg) {
    printf("[Discord] Error %d: %s\n", err, msg);
}

static void handleDisconnected(int err, const char* msg) {
    printf("[Discord] Disconnected %d: %s\n", err, msg);
}

class DiscordRPC {
public:
    DiscordRPC(const std::string& appId)
        : appId_(appId),
        startTime_(std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()))
    {
        DiscordEventHandlers handlers{};
        handlers.ready = handleReady;
        handlers.errored = handleError;
        handlers.disconnected = handleDisconnected;

        Discord_Initialize(appId_.c_str(), &handlers, 1, nullptr);
    }

    ~DiscordRPC() {
        Discord_Shutdown();
    }

    void updatePresence(bool enabled = true) {
        if (!enabled) {
            Discord_ClearPresence();
            return;
        }

        std::string player = Globals::Roblox::LocalPlayer.Name();
        std::string state = "Playing as: " + player;

        if (player.empty())
        {
            state = "Player not In-Game!";
        }

        DiscordRichPresence p{};
        
        p.state = state.c_str();
        p.details = "Using Atlas";
        p.startTimestamp = startTime_;
        p.endTimestamp = std::time(nullptr) + 5 * 60;

        Discord_UpdatePresence(&p);
    }

    void pumpCallbacks() {
        Discord_RunCallbacks();
    }

private:
    std::string appId_;
    std::time_t startTime_;
};
