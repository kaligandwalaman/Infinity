#pragma once
#include "../other/memory.hpp"
#include "../other/vector3.h"
#include "../other/Quaternion.h"
#include "../game/math.hpp"
#include "../ui/cfg.hpp"
#include <string>
#include <cstdio>  // print ke liye

extern int abs_ScreenX;
extern int abs_ScreenY;

// namespace cfg { namespace esp { inline extern bool skeleton; } }

// ========== OFFSETS (FFM) ==========
namespace offsets {
    inline uint64_t GameFacade = 0xB4C41A0; // 0xC2132B8
    inline uint64_t BaseGame_P2 = 0xB8;
    inline uint64_t BaseGame_Self = 0x0;
    inline uint64_t Match = 0x90;
    inline uint64_t Match_State = 0xcc;
    inline uint64_t LocalPlayer = 0xD8;
    
    inline uint64_t Player_HeadTF = 0x640;
    inline uint64_t Player_FootTF = 0x668;
    inline uint64_t Player_FollowCamera = 0x630;
    inline uint64_t Player_HealthMgr = 0x70; // 0x68
    inline uint64_t Player_IsBot = 0x450;
    inline uint64_t Player_NamePtr = 0x58;
    inline uint64_t Player_AvatarManager = 0x710;
    inline uint64_t Player_IsFiring = 0x7D8;
    inline uint64_t Player_Weapon = 0x4C8;
    inline uint64_t Player_HeadCollider = 0x6D8;
    inline uint64_t Player_MainCamera = 0x388;
    inline uint64_t Player_Rotation = 0x5B4;
    
    inline uint64_t Camera_Camera = 0x30;
    inline uint64_t Camera_IntPtr = 0x10;
    inline uint64_t Camera_ViewMatrix = 0x100;
    
    inline uint64_t BaseGame_Dictionary = 0xC0;
    inline uint64_t Dictionary_NumValues = 0x38;
    inline uint64_t Dictionary_Values = 0x28;
    
    inline uint64_t Transform_Obj = 0x10;
    inline uint64_t Transform_Index = 0x40;
    inline uint64_t Transform_Matrix = 0x38;
    inline uint64_t Transform_MatrixList = 0x18;
    inline uint64_t Transform_MatrixIndices = 0x20;
    inline uint64_t Player_DeadState = 0x1B98;
    inline uint64_t DeadState_Knock = 0x10;
}

// ========== DEBUG COLORS ==========
namespace debug {
    inline const char* RED = "\033[1;31m";
    inline const char* GREEN = "\033[1;32m";
    inline const char* YELLOW = "\033[1;33m";
    inline const char* CYAN = "\033[1;36m";
    inline const char* RESET = "\033[0m";
}

// ========== MONO DICTIONARY (with debug) ==========
class MonoDictionary {
public:
    uintptr_t address;
    MonoDictionary(uintptr_t addr) : address(addr) {}

    int getNumValues() {
        // Friend's Offset: ENTRIES_COUNT = 0x20
        return Read<int>(address + 0x20); 
    }
    
    uintptr_t getValues() {
        // Friend's Offset: ENTRIES_LIST = 0x18
        uintptr_t list = Read<uintptr_t>(address + 0x18);
        if (list > 0x10000000) {
            // Hum entries ki shuruat return karenge
            return list + 0x20; 
        }
        return 0;
    }
};

// ========== GET POSITION (with debug) ==========
inline Vector3 GetPosition(uintptr_t Transform) {
    
    Vector3 pos = {0, 0, 0};
    if (Transform == 0) {
        return pos;
    }

    auto transformObjValue = Read<uintptr_t>(Transform + offsets::Transform_Obj);
    
    if (transformObjValue == 0) {
        return pos;
    }

    auto indexValue = Read<uintptr_t>(transformObjValue + offsets::Transform_Index);
    auto matrixValue = Read<uintptr_t>(transformObjValue + offsets::Transform_Matrix);
    auto matrixListValue = Read<uintptr_t>(matrixValue + offsets::Transform_MatrixList);
    auto matrixIndicesValue = Read<uintptr_t>(matrixValue + offsets::Transform_MatrixIndices);

    auto resultValue = Read<Vector3>(matrixListValue + (indexValue * 0x30));
    
    int maxTries = 50;
    int tries = 0;
    int transformIndexValue = Read<int>(matrixIndicesValue + (indexValue * 4));

    while (transformIndexValue >= 0 && tries < maxTries) {
        tries++;
        
        auto tMatrixValue = Read<TransformMatrix>(matrixListValue + (transformIndexValue * 0x30));
        
        // Use lowercase properties for Quaternion
        float rotX = tMatrixValue.Rotation.x;
        float rotY = tMatrixValue.Rotation.y;
        float rotZ = tMatrixValue.Rotation.z;
        float rotW = tMatrixValue.Rotation.w;

        // Use lowercase for Vector3 (resultValue) and Vector4 (Scale)
        float scaleX = resultValue.x * tMatrixValue.Scale.x;
        float scaleY = resultValue.y * tMatrixValue.Scale.y;
        float scaleZ = resultValue.z * tMatrixValue.Scale.z;

        // Apply Rotation and Scale to Position
        resultValue.x = tMatrixValue.Position.x + scaleX + 
                        (scaleX * ((rotY * rotY * -2.0) - (rotZ * rotZ * 2.0))) + 
                        (scaleY * ((rotW * rotZ * -2.0) - (rotY * rotX * -2.0))) + 
                        (scaleZ * ((rotZ * rotX * 2.0) - (rotW * rotY * -2.0)));

        resultValue.y = tMatrixValue.Position.y + scaleY + 
                        (scaleX * ((rotX * rotY * 2.0) - (rotW * rotZ * -2.0))) + 
                        (scaleY * ((rotZ * rotZ * -2.0) - (rotX * rotX * 2.0))) + 
                        (scaleZ * ((rotW * rotX * -2.0) - (rotZ * rotY * -2.0)));

        resultValue.z = tMatrixValue.Position.z + scaleZ + 
                        (scaleX * ((rotW * rotY * -2.0) - (rotX * rotZ * -2.0))) + 
                        (scaleY * ((rotY * rotZ * 2.0) - (rotW * rotX * -2.0))) + 
                        (scaleZ * ((rotX * rotX * -2.0) - (rotY * rotY * 2.0)));
        
        transformIndexValue = Read<int>(matrixIndicesValue + (transformIndexValue * 4));
    }

    if (tries < maxTries) {
        pos = resultValue;
    } else {
        }
    return pos;
}

    // if (tries < maxTries) pos = resultValue;
    // return pos;
// }


// ========== STRING READING ==========
inline std::string ReadUnityString(uint64_t address) {
    if (!address) return "";

    int length = Read<int>(address + 0x10);
    if (length <= 0 || length > 64) return "";

    std::u16string utf16;
    utf16.resize(length);
    read_game_bytes(address + 0x14, game_pid, (uint8_t*)utf16.data(), length * sizeof(char16_t));

    std::string utf8;
    utf8.reserve(length * 2);

    for (size_t i = 0; i < utf16.size(); i++) {
        uint32_t ch = utf16[i];
        if (ch >= 0xD800 && ch <= 0xDBFF && i + 1 < utf16.size()) {
            uint32_t low = utf16[++i];
            ch = ((ch - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
        }

        if (ch <= 0x7F) utf8 += char(ch);
        else if (ch <= 0x7FF) {
            utf8 += char(0xC0 | ((ch >> 6) & 0x1F));
            utf8 += char(0x80 | (ch & 0x3F));
        }
        else if (ch <= 0xFFFF) {
            utf8 += char(0xE0 | ((ch >> 12) & 0x0F));
            utf8 += char(0x80 | ((ch >> 6) & 0x3F));
            utf8 += char(0x80 | (ch & 0x3F));
        }
        else {
            utf8 += char(0xF0 | ((ch >> 18) & 0x07));
            utf8 += char(0x80 | ((ch >> 12) & 0x3F));
            utf8 += char(0x80 | ((ch >> 6) & 0x3F));
            utf8 += char(0x80 | (ch & 0x3F));
        }
    }
    return utf8;
}

// Weapon ID to name mapping for Free Fire Max
inline const char* GetWeaponName(int wp) {
    switch (wp) {
        // Hand & Assault Rifles
        case 0: return "AK-47"; // Training Bot AK-47
        case 1: return "FIST";
        case 2: return "M4A1";
        case 80: return "M4A1-LV1";
        case 81: return "M4A1-LV2";
        case 82: return "M4A1-LV3";
        
        case 12: return "SCAR";
        case 178: return "SCAR-LV1";
        case 179: return "SCAR-LV2";
        case 180: return "SCAR-LV3";
        
        case 14: return "GROZA";
        case 70: return "GROZA-X";
        
        case 24: return "FAMAS";
        case 67: return "FAMAS-LV1";
        case 130: return "FAMAS-LV2";
        case 131: return "FAMAS-LV3";
        
        case 39: return "PLASMA";
        case 46: return "AUG";
        case 6: return "AK-47";
        case 28: return "XM8";
        case 33: return "AN94";
        case 47: return "PARAFAL";
        case 57: return "KINGFISHER";
        case 73: return "G36";
        case 99: return "SHIELD GUN";
        
        // Marksman Rifles
        case 11: return "M14";
        case 63: return "M14-LV1";
        case 126: return "M14-LV2";
        case 127: return "M14-LV3";
        case 18: return "SKS";
        case 26: return "SVD";
        case 72: return "SVD-Y";
        case 48: return "WOODPECKER";
        case 89: return "AC80";
        
        // LMGs
        case 19: return "M249";
        case 71: return "M249-X";
        case 30: return "M60";
        case 61: return "M60-LV1";
        case 122: return "M60-LV2";
        case 123: return "M60-LV3";
        case 54: return "KORD";
        
        // SMGs
        case 7: return "UMP";
        case 8: return "MP5";
        case 60: return "MP5-LV1";
        case 120: return "MP5-LV2";
        case 121: return "MP5-LV3";
        case 15: return "MP40";
        case 32: return "P90";
        case 35: return "CG15";
        case 43: return "THOMSON";
        case 49: return "VECTOR";
        case 88: return "MAC10";
        case 150: return "PP19";
        
        // Shotguns
        case 5: return "M1014";
        case 184: return "M1014-LV1";
        case 185: return "M1014-LV2";
        case 186: return "M1014-LV3";
        case 29: return "SPAS12";
        case 41: return "M1887";
        case 119: return "M1887-X";
        case 50: return "MAG-7";
        case 86: return "CHARGE BUSTER";
        case 181: return "TROGON";
        case 21002: return "M590";
        
        // Sniper Rifles
        case 4: return "AWM";
        case 65: return "AWM-Y";
        case 30060: return "AWM-INFINITE";
        case 21: return "KAR98K";
        case 64: return "KAR98K-LV1";
        case 128: return "KAR98K-LV2";
        case 129: return "KAR98K-LV3";
        case 45: return "M82B";
        case 75: return "M24";
        case 197: return "VSK94";
        case 13: return "VSS";
        
        // Pistols
        case 3: return "USP";
        case 9: return "DESERT EAGLE";
        case 10: return "G18";
        case 20: return "M1873";
        case 25: return "M500";
        case 55: return "M1917";
        case 56: return "USP-2";
        case 58: return "MINI UZI";
        
        // Melee
        case 16: return "PAN";
        case 17: return "PARANG";
        case 27: return "BAT";
        case 34: return "KATANA";
        case 51: return "SCYTHE";
        
        // Special
        case 93: return "HEALING PISTOL";
        case 21001: return "HEALING PISTOL-Y";
        case 78: return "HEALING SNIPER";
        case 100: return "FGL-24";
        case 1201: return "GLOO WALL";
        case 617: return "GLOO MELTER";
        case 601: return "GRENADE";
        
        default: return "Unknown";
    }
}

// ========== Player Transformation ==========
inline Vector3 GetNodePosition(uintptr_t nodeTransform) {
    
    auto transformValue = Read<uintptr_t>(nodeTransform + offsets::Transform_Obj);
    
    if (transformValue == 0) {
        return {0,0,0};
    }
    return GetPosition(transformValue);
}

inline Vector3 GetBonePosition(uintptr_t player, uintptr_t offset) {
    uintptr_t bone = Read<uintptr_t>(player + offset);
    if (!bone) return {0,0,0};
    return GetNodePosition(bone);
}


// ==================== SKELETON ESP ====================
inline void DrawSkeletonESP(ImDrawList* draw, uintptr_t enemy, D3DMatrix matrix) {
    if (!cfg::esp::skeleton) return;
    
    // Get all bone positions
    Vector3 head = GetBonePosition(enemy, 0x648);
    Vector3 chest = GetBonePosition(enemy, 0x658);
    Vector3 hip = GetBonePosition(enemy, 0x650);
    Vector3 leftShoulder = GetBonePosition(enemy, 0x6B0);
    Vector3 rightShoulder = GetBonePosition(enemy, 0x6B8);
    Vector3 leftHand = GetBonePosition(enemy, 0x6C8);
    Vector3 rightHand = GetBonePosition(enemy, 0x6C0);
    Vector3 leftAnkle = GetBonePosition(enemy, 0x680);
    Vector3 rightAnkle = GetBonePosition(enemy, 0x688);
    
    // Convert to screen
    Vector3 sHead = WorldToScreen(matrix, head, abs_ScreenX, abs_ScreenY);
    Vector3 sChest = WorldToScreen(matrix, chest, abs_ScreenX, abs_ScreenY);
    Vector3 sHip = WorldToScreen(matrix, hip, abs_ScreenX, abs_ScreenY);
    Vector3 sLeftShoulder = WorldToScreen(matrix, leftShoulder, abs_ScreenX, abs_ScreenY);
    Vector3 sRightShoulder = WorldToScreen(matrix, rightShoulder, abs_ScreenX, abs_ScreenY);
    Vector3 sLeftHand = WorldToScreen(matrix, leftHand, abs_ScreenX, abs_ScreenY);
    Vector3 sRightHand = WorldToScreen(matrix, rightHand, abs_ScreenX, abs_ScreenY);
    Vector3 sLeftAnkle = WorldToScreen(matrix, leftAnkle, abs_ScreenX, abs_ScreenY);
    Vector3 sRightAnkle = WorldToScreen(matrix, rightAnkle, abs_ScreenX, abs_ScreenY);
    
    // Color convert karein
    // Knocked ho toh RED, warna normal color
    auto  _deadState = Read<uintptr_t>(enemy + offsets::Player_DeadState);
    bool  knocked    = false;
    if (_deadState) {
        auto _maybeDead = Read<uintptr_t>(_deadState + 0x20);
        if (_maybeDead) {
            knocked = (Read<int>(_maybeDead + offsets::DeadState_Knock) == 8);
        }
    }
    ImU32 skColor    = knocked
                       ? IM_COL32(255, 0, 0, 255)
                       : ImGui::ColorConvertFloat4ToU32(cfg::esp::skeleton_col);
    float thick = 1.8f;
    
    // Spine
    if (sHead.x > 0 && sChest.x > 0) draw->AddLine(ImVec2(sHead.x, sHead.y), ImVec2(sChest.x, sChest.y), skColor, thick);
    if (sChest.x > 0 && sHip.x > 0) draw->AddLine(ImVec2(sChest.x, sChest.y), ImVec2(sHip.x, sHip.y), skColor, thick);
    
    // Arms
    if (sChest.x > 0 && sLeftShoulder.x > 0) draw->AddLine(ImVec2(sChest.x, sChest.y), ImVec2(sLeftShoulder.x, sLeftShoulder.y), skColor, thick);
    if (sChest.x > 0 && sRightShoulder.x > 0) draw->AddLine(ImVec2(sChest.x, sChest.y), ImVec2(sRightShoulder.x, sRightShoulder.y), skColor, thick);
    if (sLeftShoulder.x > 0 && sLeftHand.x > 0) draw->AddLine(ImVec2(sLeftShoulder.x, sLeftShoulder.y), ImVec2(sLeftHand.x, sLeftHand.y), skColor, thick);
    if (sRightShoulder.x > 0 && sRightHand.x > 0) draw->AddLine(ImVec2(sRightShoulder.x, sRightShoulder.y), ImVec2(sRightHand.x, sRightHand.y), skColor, thick);
    
    // Legs
    if (sHip.x > 0 && sLeftAnkle.x > 0) draw->AddLine(ImVec2(sHip.x, sHip.y), ImVec2(sLeftAnkle.x, sLeftAnkle.y), skColor, thick);
    if (sHip.x > 0 && sRightAnkle.x > 0) draw->AddLine(ImVec2(sHip.x, sHip.y), ImVec2(sRightAnkle.x, sRightAnkle.y), skColor, thick);
}


// ========== GAME STATE (heavy debug) ==========
inline bool IsInMatch() {
    
    if (lib_base == 0) {
        return false;
    }
    
    auto GameFacade_c = Read<uintptr_t>(lib_base + offsets::GameFacade);    
    if (!GameFacade_c) {
        return false;
    }
    
    auto P2 = Read<uintptr_t>(GameFacade_c + offsets::BaseGame_P2);    
    if (!P2) {
        return false;
    }
    
    auto BaseGame = Read<uintptr_t>(P2);    
    if (!BaseGame) {
        return false;
    }
    
    auto m_Match = Read<uintptr_t>(BaseGame + offsets::Match);    
    if (!m_Match) {
        return false;
    }
    
    int state = Read<int>(m_Match + offsets::Match_State);
    
    bool inMatch = (state == 1);    
    return inMatch;
}

inline uintptr_t GetLocalPlayer() {
    
    if (lib_base == 0) {
        return 0;
    }
    
    auto GameFacade_c = Read<uintptr_t>(lib_base + offsets::GameFacade);
    if (!GameFacade_c) return 0;
    
    auto P2 = Read<uintptr_t>(GameFacade_c + offsets::BaseGame_P2);
    if (!P2) return 0;
    
    auto BaseGame = Read<uintptr_t>(P2);
    if (!BaseGame) return 0;
    
    auto m_Match = Read<uintptr_t>(BaseGame + offsets::Match);
    if (!m_Match) return 0;
    
    auto localPlayer = Read<uintptr_t>(m_Match + offsets::LocalPlayer);    
    return localPlayer;
}

inline D3DMatrix GetViewMatrix(uintptr_t localPlayer) {
    
    D3DMatrix empty = {};
    if (!localPlayer) {
        return empty;
    }
    
    auto FollowCamera = Read<uintptr_t>(localPlayer + offsets::Player_FollowCamera);
    if (!FollowCamera) return empty;
    
    auto Camera = Read<uintptr_t>(FollowCamera + offsets::Camera_Camera);
    if (!Camera) return empty;
    
    auto IntPtrCam = Read<uintptr_t>(Camera + offsets::Camera_IntPtr);
    if (!IntPtrCam) return empty;
    
    auto vm = Read<D3DMatrix>(IntPtrCam + offsets::Camera_ViewMatrix);    
    return vm;
}

// ========== GAME CACHE (with debug) ==========
namespace game {
    inline bool initialized = false;
    inline uintptr_t cached_GameFacade = 0;
    inline uintptr_t cached_BaseGame = 0;
    inline uintptr_t cached_Match = 0;
    
    inline bool init() {        
        if (lib_base == 0) {
            return false;
        }
        
        auto GameFacade_c = Read<uintptr_t>(lib_base + offsets::GameFacade);
        if (!GameFacade_c) return false;
        
        auto P2 = Read<uintptr_t>(GameFacade_c + offsets::BaseGame_P2);
        if (!P2) return false;
        
        auto BaseGame = Read<uintptr_t>(P2);
        if (!BaseGame) return false;
        
        cached_GameFacade = GameFacade_c;
        cached_BaseGame = BaseGame;
        
        initialized = true;
        return true;
    }
    
    inline bool valid() {
        if (!initialized || lib_base == 0) {
            return false;
        }
        
        auto check = Read<uintptr_t>(lib_base + offsets::GameFacade);
        bool ok = (check == cached_GameFacade);
                
        return ok;
    }
    
    inline void reset() {
        initialized = false;
        cached_GameFacade = 0;
        cached_BaseGame = 0;
        cached_Match = 0;
    }
}
