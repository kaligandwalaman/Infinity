#include "visuals.hpp"
#include "../game/game.hpp"
#include "../game/math.hpp"
#include "../game/player.hpp"
#include "../ui/cfg.hpp"
#include "imgui.h"
#include <cmath>
#include <algorithm>  // std::max ke liye

extern ImFont* espFont;
extern int abs_ScreenX, abs_ScreenY;
extern int game_pid;
extern uint64_t lib_base;

// Printf Signals
const char* G = "\033[1;32m"; // Green
const char* R = "\033[1;31m"; // Red
const char* N = "\033[0m";    // Reset

void visuals::draw() {
        
    if (!cfg::esp::box && !cfg::esp::name && !cfg::esp::health && 
        !cfg::esp::distance && !cfg::esp::aimbot && !cfg::esp::aimlock && !cfg::esp::show_fov) return;

    if (game_pid <= 0 || lib_base == 0) return;
    // Visuals mein bhi check karo
    if (!game::initialized) return;

/*
    // Gamefacade Chain
    auto GameFacade_c = Read<uintptr_t>(lib_base + 0xB4C41A0);
    if (!GameFacade_c) return;
    
    auto P2 = Read<uintptr_t>(GameFacade_c + 0xB8);
    if (!P2) return;
    
    auto BaseGame = Read<uintptr_t>(P2);
    if (!BaseGame) return;
    
    auto m_Match = Read<uintptr_t>(BaseGame + 0x90);
    if (!m_Match || Read<int>(m_Match + 0xcc) != 1) return;
    
    auto localPlayer = Read<uintptr_t>(m_Match + 0xD8);
    if (!localPlayer) return;
*/
    // Gamefacade Chain - Debugging Start
  //  printf("[DEBUG] lib_base: 0x%lx\n", (unsigned long)lib_base);
    
    auto GameFacade_c = Read<uintptr_t>(lib_base + 0xB4C41A0);
  //  printf("[DEBUG] GameFacade_c: 0x%lx\n", (unsigned long)GameFacade_c);
    if (!GameFacade_c) {
    //    printf("[!] FAILED: GameFacade_c is null (Check Offset 0xB4C41A0)\n");
        return;
    }
    
    auto P2 = Read<uintptr_t>(GameFacade_c + 0xB8);
//    printf("[DEBUG] P2: 0x%lx\n", (unsigned long)P2);
    if (!P2) {
  //      printf("[!] FAILED: P2 is null (Check Offset 0xB8)\n");
        return;
    }
    
    auto BaseGame = Read<uintptr_t>(P2);
 //   printf("[DEBUG] BaseGame: 0x%lx\n", (unsigned long)BaseGame);
    if (!BaseGame) {
 //       printf("[!] FAILED: BaseGame is null (Check pointer at P2)\n");
        return;
    }
    
    auto m_Match = Read<uintptr_t>(BaseGame + 0x90);
 //   printf("[DEBUG] m_Match: 0x%lx\n", (unsigned long)m_Match);
    if (!m_Match) {
   //     printf("[!] FAILED: m_Match is null (Check Offset 0x90)\n");
        return;
    }
    
    // Special check for match state/validation
    int match_state = Read<int>(m_Match + 0xcc);
//    printf("[DEBUG] match_state (at m_Match + 0xcc): %d\n", match_state);
    if (match_state != 1) {
   //     printf("[!] FAILED: match_state is not 1 (Current: %d). Game probably not started.\n", match_state);
        return;
    }
    
    auto localPlayer = Read<uintptr_t>(m_Match + 0xD8);
  //  printf("[DEBUG] localPlayer: 0x%lx\n", (unsigned long)localPlayer);
    if (!localPlayer) {
  //      printf("[!] FAILED: localPlayer is null (Check Offset 0xD8)\n");
        return;
    }
    
 //   printf("[SUCCESS] Entire chain read successfully! LocalPlayer: 0x%lx\n", (unsigned long)localPlayer);
    // Gamefacade Chain - Debugging End


    // ========== PLAYER ALIVE CHECK (LOOP SE PEHLE) ==========
    int localHp = player::health(localPlayer);
    bool localDead = (localHp <= 0);
    
    // Agar local player dead hai, toh kuch mat draw karo (ESP + FOV dono band)
    if (localDead) return;

    Vector3 localPos = player::position(localPlayer);
    if (localPos.x == 0 && localPos.y == 0) return;

    D3DMatrix viewMatrix = player::view_matrix(localPlayer);
    if (viewMatrix._11 == 0 && viewMatrix._22 == 0) return;

    ImDrawList* draw = ImGui::GetBackgroundDrawList();

    // ========== SHOW FOV CIRCLE ==========
    if (cfg::esp::show_fov && cfg::esp::aimbot) {
        float centerX = abs_ScreenX / 2.0f;
        float centerY = abs_ScreenY / 2.0f;
        float fovRadius = cfg::esp::aim_fov * 2.0f;  
        
        // Use selected color from color picker
        draw->AddCircle(
            ImVec2(centerX, centerY),
            fovRadius,
            IM_COL32(
                (int)(cfg::esp::fov_col.x * 255),
                (int)(cfg::esp::fov_col.y * 255),
                (int)(cfg::esp::fov_col.z * 255),
                (int)(cfg::esp::fov_col.w * 255)
            ),
            64,
            2.0f
        );
    }

    // Dictionary 
    auto dictionaryPtr = Read<uintptr_t>(BaseGame + 0xC0);
    if (!dictionaryPtr) return;
    
    auto entitylist = Read<uintptr_t>(dictionaryPtr + 0x18);
    if (!entitylist) return;

    int drawnCount = 0;
    const float MAX_DISTANCE = 120.0f;
    Vector3 cameraPos = GetPosition(Read<uintptr_t>(localPlayer + offsets::Player_MainCamera));

    // Aimline ke liye — closest enemy track karna
    float    aimline_best_dist = FLT_MAX;
    ImVec2   aimline_target    = ImVec2(-1, -1);
    bool     aimline_knocked   = false;
    
    // Enemy Loop
    for (int i = 0; i < 1000; i++) {
        uintptr_t enemy = Read<uintptr_t>(entitylist + (i * 8));

        if (!enemy || enemy == localPlayer) continue;
        
        // Team check (from FFM)
        auto AvatarManager = Read<uintptr_t>(enemy + offsets::Player_AvatarManager);
        if (!AvatarManager) continue;
        auto UmaAvatarSimple = Read<uintptr_t>(AvatarManager + 0x138);
        if (!UmaAvatarSimple) continue;
        auto UmaData = Read<uintptr_t>(UmaAvatarSimple + 0x28);
        if (!UmaData) continue;
        if (Read<bool>(UmaData + 0x81)) continue;  // Skip teammate
        
        // Visibility check
        if (!player::is_visible(enemy)) continue;
                
        // Knock check (CORRECTED)
        int knockState = player::get_knock_state(enemy);
        bool knocked = (knockState == 8);
        bool fullyDead = (player::health(enemy) <= 0) || knocked;
                        
        // NEW (CORRECT - sirf HP check):
        if (player::health(enemy) <= 0) {
            continue;  // Dead player = no ESP
        }
        // Knocked players (state 8, HP>0) will show with RED color
        
        int hp = player::health(enemy);

        // --- Weapon Logic  ---
        char weaponDisplay[32] = "None";
        uintptr_t weaponMgr = Read<uintptr_t>(enemy + 0x70); 
        if (weaponMgr) {
            uintptr_t weaponPtr1 = Read<uintptr_t>(weaponMgr + 0x10);
            if (weaponPtr1) {
                uintptr_t weaponPtr2 = Read<uintptr_t>(weaponPtr1 + 0x40);
                if (weaponPtr2) {
                    int weaponID = Read<int>(weaponPtr2 + 0x18);
                    strcpy(weaponDisplay, GetWeaponName(weaponID));
                    
                    static int debug_frame = 0;
                    if (i == 0 && debug_frame++ % 100 == 0) {
                    }
                }
            }
        }

        Vector3 headPos = player::position(enemy);
        Vector3 feetPos = player::foot_position(enemy);
        if (headPos.x == 0 && headPos.y == 0) continue;
        headPos.y += 0.18f;

        float dist = Vector3::Distance(headPos, localPos);
        if (dist > MAX_DISTANCE) continue;

        Vector3 headScreen = WorldToScreen(viewMatrix, headPos, abs_ScreenX, abs_ScreenY);
        Vector3 feetScreen = WorldToScreen(viewMatrix, feetPos, abs_ScreenX, abs_ScreenY);

        if (headScreen.z < 0.01f || feetScreen.z < 0.01f) continue;
        if (headScreen.z > 800.0f) continue;

        // ========== AIMBOT LOGIC (with Ignore Knock filter) ==========
        bool canAimbot = true;
        
        // Ignore Knock filter
        if (cfg::esp::ignore_knock && knocked) {
            canAimbot = false;
        }
        
        // Aimlock (works on all except when ignored)
        if (cfg::esp::aimlock && canAimbot) {  // ✅ Check canAimbot here!
        auto headCollider = Read<uintptr_t>(enemy + offsets::Player_HeadCollider);
        if (headCollider) Write<uintptr_t>(enemy + 0x80, headCollider);
        auto weapon = Read<uintptr_t>(localPlayer + offsets::Player_Weapon);
        if (weapon) Write<float>(weapon + 0x4DC, 0.0f);
        }

        // Aimbot FOV (with ignore knock)
        int isFiring = Read<int>(localPlayer + offsets::Player_IsFiring);
        float screenDist = sqrt(pow(headScreen.x - abs_ScreenX/2.0f, 2) + 
                                pow(headScreen.y - abs_ScreenY/2.0f, 2));

        // ========== AIMBOT (Single with 3 Types) ==========
        if (cfg::esp::aimbot && canAimbot && screenDist < cfg::esp::aim_fov && isFiring > 0) {
            
            float offset;
            switch(cfg::esp::aimbot_type) {
                case 0: offset = -0.18f; break;    // Head (Aapka original value)
                case 1: offset = -0.22f; break;    // Neck  
                case 2: offset = -0.28f; break;    // Chest
                default: offset = -0.18f; break;   // Fallback bhi Head
            }
            
            Quaternion desiredRot = GetRotationToTheLocation(headPos, offset, cameraPos);
            Write<Quaternion>(localPlayer + offsets::Player_Rotation, desiredRot);
        }


        // ========== ESP DRAWING ==========
        float height = abs(headScreen.y - feetScreen.y);
        float width = height / 2.0f;
        float x = feetScreen.x - (width / 2.0f);
        float y = headScreen.y;

        // Aimline — sirf closest enemy track karo, draw baad mein
        if (cfg::esp::aimline) {
            float d = sqrt(pow(headScreen.x - abs_ScreenX / 2.0f, 2) +
                           pow(headScreen.y - abs_ScreenY / 2.0f, 2));
            if (d < aimline_best_dist) {
                aimline_best_dist = d;
                aimline_target    = ImVec2(headScreen.x, headScreen.y);
                // Is closest enemy ka knock state store karo
                aimline_knocked = player::is_knocked(enemy);
            }
        }
        
        if (cfg::esp::head_dot) {
            draw->AddCircleFilled(ImVec2(headScreen.x, headScreen.y), 
                                  cfg::esp::head_dot_radius, 
                                  ImGui::ColorConvertFloat4ToU32(cfg::esp::dot_col));
        }
        
        // DrawESP loop ke andar:
        if (cfg::esp::skeleton) {
            // viewMatrix aur enemy pehle se hi loop mein available hain
            DrawSkeletonESP(draw, enemy, viewMatrix); 
        }


        if (cfg::esp::weapon) {
            float boxHeight = abs(headScreen.y - feetScreen.y);
            float width = boxHeight / 2.0f;
            float boxRightEdge = feetScreen.x + (width / 2.0f);
        
            // --- Calculation for Right Side Center ---
            // Y position ko head se start karke box ki height ke center mein laane ke liye:
            float centerY = headScreen.y + (boxHeight / 2.0f);
            
            // Text ki height ka half minus karein taaki exact center alignment mile
            float textSize = cfg::esp::weapon_size;
            ImVec2 weaponPos = ImVec2(boxRightEdge + 5.0f, centerY - (textSize / 2.0f));
        
            // Color conversion from cfg
            ImU32 weaponCol = ImGui::ColorConvertFloat4ToU32(cfg::esp::weapon_col);
        
            // Draw using custom size and color
            // Agar aapka draw_text function size support karta hai toh:
            draw->AddText(NULL, textSize, weaponPos, weaponCol, weaponDisplay);
        }
        
        // ========== LINE FEATURE (3 Types: Top, Centre, Bottom) ==========
        if (cfg::esp::line) {
            float startX, startY, endX, endY;
            
            // Screen dimensions
            float screenCenterX = abs_ScreenX / 2.0f;
            float screenCenterY = abs_ScreenY / 2.0f;
            float screenBottomY = abs_ScreenY;
            
            // Box dimensions
            float boxTopCenterX = x + (width / 2.0f);
            float boxTopCenterY = y;
            float boxBottomCenterX = x + (width / 2.0f);
            float boxBottomCenterY = y + height;
            
            // Line Type ke hisaab se start/end points decide karo
            switch (cfg::esp::line_type) {
                case 0: // Top (Default) - Screen Top to Box Top
                    startX = screenCenterX;
                    startY = 0.0f;
                    endX = boxTopCenterX;
                    endY = boxTopCenterY;
                    break;
                    
                case 1: // Centre - Screen Center to Box Bottom Center
                    startX = screenCenterX;
                    startY = screenCenterY;
                    endX = boxBottomCenterX;
                    endY = boxBottomCenterY;
                    break;
                            
                case 2: // Bottom - Screen Bottom to Box Bottom
                    startX = screenCenterX;
                    startY = screenBottomY;
                    endX = boxBottomCenterX;
                    endY = boxBottomCenterY;
                    break;
                    
                default: // Fallback to Top
                    startX = screenCenterX;
                    startY = 0.0f;
                    endX = boxTopCenterX;
                    endY = boxTopCenterY;
                    break;
            }
            
            // FIXED: Line color with knocked check (Box/Name ki tarah)
            ImU32 lineColor;
            if (knocked) {
                lineColor = IM_COL32(255, 0, 0, 255);  // RED for knocked
            } else {
                lineColor = IM_COL32(
                    (int)(cfg::esp::line_col.x * 255),
                    (int)(cfg::esp::line_col.y * 255),
                    (int)(cfg::esp::line_col.z * 255),
                    (int)(cfg::esp::line_col.w * 255)
                );
            }
            
            // Draw line
            draw->AddLine(
                ImVec2(startX, startY),
                ImVec2(endX, endY),
                lineColor,
                1.5f
            );
        }
        
         // Box (4 Types: Normal, Filled, Corner, Rounded)
        if (cfg::esp::box) {
            // Color decide karo (knocked check)
            ImU32 boxColor;
            ImU32 filledColor;  // Filled ke liye transparent color
            
            if (knocked) {
                boxColor = IM_COL32(255, 0, 0, 255);  // RED outline
                filledColor = IM_COL32(255, 0, 0, 60);  // RED transparent fill
            } else {
                boxColor = IM_COL32(
                    (int)(cfg::esp::box_col.x * 255),
                    (int)(cfg::esp::box_col.y * 255),
                    (int)(cfg::esp::box_col.z * 255),
                    (int)(cfg::esp::box_col.w * 255)
                );
                // Filled color with transparency (original color ka 25% alpha)
                filledColor = IM_COL32(
                    (int)(cfg::esp::box_col.x * 255),
                    (int)(cfg::esp::box_col.y * 255),
                    (int)(cfg::esp::box_col.z * 255),
                    60  // 60/255 = ~25% opacity
                );
            }
            
            // Box dimensions
            ImVec2 boxMin(x, y);
            ImVec2 boxMax(x + width, y + height);
            float thickness = 2.0f;
            float cornerSize = width * 0.25f;  // Corner lines ki length (25% of width)
            
            // Box Type ke hisaab se draw karo
            switch (cfg::esp::box_type) {
                case 0: // Normal (Default) - Rectangle outline
                    draw->AddRect(boxMin, boxMax, boxColor, 0, 0, thickness);
                    break;
                    
                case 1: // Filled - Transparent filled rectangle with outline
                    draw->AddRectFilled(boxMin, boxMax, filledColor);
                    draw->AddRect(boxMin, boxMax, boxColor, 0, 0, thickness);
                    break;
                    
                case 2: // Corner - Sirf 4 corners
                    {
                        // Top-left corner
                        draw->AddLine(ImVec2(boxMin.x, boxMin.y), ImVec2(boxMin.x + cornerSize, boxMin.y), boxColor, thickness);
                        draw->AddLine(ImVec2(boxMin.x, boxMin.y), ImVec2(boxMin.x, boxMin.y + cornerSize), boxColor, thickness);
                        
                        // Top-right corner
                        draw->AddLine(ImVec2(boxMax.x - cornerSize, boxMin.y), ImVec2(boxMax.x, boxMin.y), boxColor, thickness);
                        draw->AddLine(ImVec2(boxMax.x, boxMin.y), ImVec2(boxMax.x, boxMin.y + cornerSize), boxColor, thickness);
                        
                        // Bottom-left corner
                        draw->AddLine(ImVec2(boxMin.x, boxMax.y - cornerSize), ImVec2(boxMin.x, boxMax.y), boxColor, thickness);
                        draw->AddLine(ImVec2(boxMin.x, boxMax.y), ImVec2(boxMin.x + cornerSize, boxMax.y), boxColor, thickness);
                        
                        // Bottom-right corner
                        draw->AddLine(ImVec2(boxMax.x - cornerSize, boxMax.y), ImVec2(boxMax.x, boxMax.y), boxColor, thickness);
                        draw->AddLine(ImVec2(boxMax.x, boxMax.y - cornerSize), ImVec2(boxMax.x, boxMax.y), boxColor, thickness);
                    }
                    break;
                    
                case 3: // Rounded - Rounded rectangle
                    draw->AddRect(boxMin, boxMax, boxColor, 8.0f, 0, thickness);  // 8.0f = corner radius
                    break;
                    
                default: // Fallback to Normal
                    draw->AddRect(boxMin, boxMax, boxColor, 0, 0, thickness);
                    break;
            }
        }

        // Name (RED if knocked) - with configurable size and position
        if (cfg::esp::name) {
            std::string name = player::name(enemy);
            ImU32 nameColor;
            if (knocked) {
                nameColor = IM_COL32(255, 0, 0, 255);  // RED for knocked
            } else {
                nameColor = IM_COL32(
                    (int)(cfg::esp::name_col.x * 255),
                    (int)(cfg::esp::name_col.y * 255),
                    (int)(cfg::esp::name_col.z * 255),
                    (int)(cfg::esp::name_col.w * 255)
                );
            }
            
            // NAYA: Font size ko clamp karo (minimum 1 for safety)
            float fontSize = std::max(1.0f, cfg::esp::name_size);
            
            // NAYA: Position aur upar kiya (pehle -18 tha, ab -25 aur fontSize ka factor)
            float nameY = y - 23.0f - (fontSize * 0.5f);
            
            // NAYA: Font size ke hisaab se text size calculate karo
            ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
            float scale = fontSize / ImGui::GetFont()->FontSize;  // Current font se ratio
            ImVec2 scaledSize = ImVec2(textSize.x * scale, textSize.y * scale);
            
            float nameX = x + (width - scaledSize.x) / 2.0f;
            
            // NAYA: Scaled font ke saath draw karo
            draw->AddText(ImGui::GetFont(), fontSize, ImVec2(nameX, nameY), nameColor, name.c_str());
        }

        // Health BAR only (text hataya)
        if (cfg::esp::health) {
            ImU32 hpColor;
            if (knocked) {
                hpColor = IM_COL32(255, 0, 0, 255);
            } else {
                hpColor = hp < 50 ? IM_COL32(255,0,0,255) : 
                         (hp < 100 ? IM_COL32(255,165,0,255) : IM_COL32(0,255,0,255));
            }
            
            float bh = height * (hp / 200.0f);
            draw->AddRectFilled(ImVec2(x-8, y), ImVec2(x-3, y+height), IM_COL32(0,0,0,255));
            draw->AddRectFilled(ImVec2(x-8, y+height-bh), ImVec2(x-3, y+height), hpColor);
        }
        
        // Health TEXT alag se (naya checkbox)
        if (cfg::esp::health_text) {
            ImU32 hpColor;
            if (knocked) {
                hpColor = IM_COL32(255, 0, 0, 255);
            } else {
                hpColor = hp < 50 ? IM_COL32(255,0,0,255) : 
                         (hp < 100 ? IM_COL32(255,165,0,255) : IM_COL32(0,255,0,255));
            }
            
            char buf[16]; 
            snprintf(buf, sizeof(buf), "%d", hp);
            draw->AddText(ImVec2(x, y+height+2), hpColor, buf);
        }
        
        // Distance
        if (cfg::esp::distance) {
            char buf[32]; snprintf(buf, sizeof(buf), "%.0fm", dist);
            ImU32 col = knocked ? IM_COL32(255,0,0,255) : IM_COL32(
                (int)(cfg::esp::distance_col.x * 255),
                (int)(cfg::esp::distance_col.y * 255),
                (int)(cfg::esp::distance_col.z * 255),
                (int)(cfg::esp::distance_col.w * 255)
            );
            draw->AddText(ImVec2(x + (width - ImGui::CalcTextSize(buf).x)/2, y+height+22), col, buf);
        }
        drawnCount++;
    }  
      // Aimline — sirf 1 closest enemy par draw karo
    if (cfg::esp::aimline && aimline_target.x > 0) {
        // Knocked ho toh RED lock, warna user ka chosen color
        ImU32 aline_col = aimline_knocked
            ? IM_COL32(255, 0, 0, 255)
            : ImGui::ColorConvertFloat4ToU32(cfg::esp::aimline_col);
        draw->AddLine(
            ImVec2(abs_ScreenX / 2.0f, abs_ScreenY / 2.0f),
            aimline_target,
            aline_col, 1.5f);
    }
}


// Stubs for compatibility
void visuals::dbox(const ImVec2&, const ImVec2&, float) {}
void visuals::dhp(int, const ImVec2&, const ImVec2&, float, float) {}
void visuals::dnick(const char*, const ImVec2&, float, float, float) {}
void visuals::ddist(float, float, float, float, float) {}
void visuals::draw_text_outlined(ImDrawList*, ImFont*, float, const ImVec2&, ImU32, const char*) {}
