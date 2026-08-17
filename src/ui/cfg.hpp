#pragma once
#include "imgui.h"

namespace cfg {

namespace esp {
    inline bool box = false;
    inline bool name = false;
    inline bool health = false;
    inline bool health_text = false;
    inline bool distance = false;
    inline bool line = false;
    inline bool weapon = false;
    inline ImVec4 weapon_col = ImVec4(1.f, 1.f, 1.f, 1.f); // Default White
    inline float weapon_size = 12.0f; // Default Size
    inline ImVec4 line_col = ImVec4(1.f, 1.f, 1.f, 1.f);  // Neon Green
    inline int line_type = 0;  // Default: Top
    inline int box_type = 0;
    inline float name_size = 10.0f;
    inline ImVec4 box_col = ImVec4(1.f, 1.f, 1.f, 1.f);  // Neon Green
    inline ImVec4 name_col = ImVec4(1.f, 1.f, 1.f, 1.f);  // Neon Green
    inline ImVec4 distance_col = ImVec4(1.f, 1.f, 1.f, 1.f);  // Neon Green
    inline bool aimbot = false;          // Aimbot while firing
    inline int aimbot_type = 1;           // 0=Head, 1=Neck, 2=Chest
    inline bool aimlock = false;         // Aimlock (silent lock)
    inline float aim_fov = 60.0f;        // FOV slider (10 to 360)
    inline bool show_fov = false;      // NEW: Show FOV circle on screen
    inline ImVec4 fov_col = ImVec4(1.f, 1.f, 1.f, 1.f);  // Neon Green NEW: FOV circle color
    inline bool ignore_knock = false;  // NEW: Aimbot ignore knocked players
    inline bool aimline = false;
    inline bool skeleton = false;
    inline bool head_dot = false;
    inline float head_dot_radius = 5.0f;
    inline ImVec4 dot_col = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    inline ImVec4 skeleton_col = ImVec4(1.f, 1.f, 1.f, 1.f); // Default Light Blue
    inline ImVec4 aimline_col   = ImVec4(1.f, 1.f, 1.f, 1.f); // Aimline color
    }
}

