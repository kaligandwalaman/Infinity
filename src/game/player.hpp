#pragma once
#include "game.hpp"

namespace player {
    inline Vector3 position(uintptr_t player) {
        auto headTF = Read<uintptr_t>(player + offsets::Player_HeadTF);
        return GetNodePosition(headTF);
    }
    
    inline Vector3 foot_position(uintptr_t player) {
        auto footTF = Read<uintptr_t>(player + offsets::Player_FootTF);
        return GetNodePosition(footTF);
    }
    
    inline int health(uintptr_t player) {
        auto hpMgr = Read<uintptr_t>(player + offsets::Player_HealthMgr);
        if (!hpMgr) return 0;
        auto hpComp = Read<uintptr_t>(hpMgr + 0x10);
        if (!hpComp) return 0;
        auto hpData = Read<uintptr_t>(hpComp + 0x20);
        if (!hpData) return 0;
        return Read<int>(hpData + 0x18);
    }
    
    inline bool is_bot(uintptr_t player) {
        return Read<bool>(player + offsets::Player_IsBot);
    }
    
    inline std::string name(uintptr_t player) {
        if (is_bot(player)) return "Bot";
        uintptr_t namePtr = Read<uintptr_t>(player + offsets::Player_NamePtr);
        if (namePtr <= 0x10000000) return "Enemy";
        std::string realName = ReadUnityString(namePtr);
        if (!realName.empty() && realName.length() > 1) return realName;
        return "Enemy";
    }
    
    inline bool is_visible(uintptr_t player) {
        auto AvatarManager = Read<uintptr_t>(player + offsets::Player_AvatarManager);
        if (!AvatarManager) return false;
        auto UmaAvatarSimple = Read<uintptr_t>(AvatarManager + 0x138);
        if (!UmaAvatarSimple) return false;
        return Read<bool>(UmaAvatarSimple + 0x101);
    }
                    
    // CORRECTED: Knock check (from main64.cpp)
    inline int get_knock_state(uintptr_t player) {
        auto deadState = Read<uintptr_t>(player + offsets::Player_DeadState);
        if (!deadState) {
            return -1;
        }
        auto maybeDead = Read<uintptr_t>(deadState + 0x20);
        if (!maybeDead) {
            return -1;
        }
        int knockValue = Read<int>(maybeDead + offsets::DeadState_Knock);
        return knockValue;
    }
    
    inline bool is_knocked(uintptr_t player) {
        return get_knock_state(player) == 8;  // 8 = knocked
    }
    
    // Simplified: Just check knock state, don't combine with health here
    inline bool is_really_dead(uintptr_t player) {
        // Health check
        int hp = health(player);
        if (hp <= 0) return true;
        
        // Knock check
        int knock = get_knock_state(player);
        return knock == 8;  // 8 = knocked/dead state
    }
    
    // MODIFIED: Original is_dead rename/update
    // inline bool is_dead_old(uintptr_t player) {
        // if (Read<bool>(player + 0x74)) return true;
        // return is_fully_dead(player);
    // }
    
    inline D3DMatrix view_matrix(uintptr_t localPlayer) {
        return GetViewMatrix(localPlayer);
    }
}
