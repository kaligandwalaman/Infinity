#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "../login/struct.h"
#include "bar.hpp"
#include "cfg.hpp"
#include "theme/theme.hpp"
#include "widgets/widgets.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "Android_draw/draw.h"
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <functional>   // ADDED for std::function

static float* g_menu = ::menu;

namespace ui::menu {
    using namespace style;
    using namespace widgets;

    static float ma    = 0.f;
    static int   tab   = 0;
    static int   stab  = 0;
    static bool  drag  = false;
    static ImVec2 doff = ImVec2(0, 0);
    static float scr_tgt = 0.f;
    static float scr_cur = 0.f;
    static float ta    = 1.f;
    static bool  tsw   = false;
    static int   ttab  = 0;
    static int   tstab = 0;

    static float mw = 1120.f;   // width +115
    static float mh = 700.f;   // height +80
    static float sw  = 190.f;
    static float hh  = 52.f;
    static float fh  = 38.f;
    static float sth = 50.f;

    // ── 4 tabs: Aimbot | Visuals | Config | Settings ──
    // Visuals has subtabs (ESP only now)
    static const char* tabs[]   = {"Aimbot", "Visuals", "Config", "Settings"};
    static const char* tab_icons[] = {"O", "A", "a", "M"};
    static constexpr int tc     = 4;
    static const char* vstabs[] = {"ESP"};
    static constexpr int vstc   = 1;

    static float lrp(float a, float b, float t) { return a + (b - a) * t; }

    // ── Save / Load helpers (copied from refrence.cpp) ──
    static void do_save() {
        FILE* f = fopen("/data/local/tmp/sx_config.bin","wb");
        if (!f) return;
        fwrite(&cfg::esp::box,            sizeof(bool),   1, f);
        fwrite(&cfg::esp::line,           sizeof(bool),   1, f);
        fwrite(&cfg::esp::skeleton,       sizeof(bool),   1, f);
        fwrite(&cfg::esp::name,           sizeof(bool),   1, f);
        fwrite(&cfg::esp::weapon,         sizeof(bool),   1, f);
        fwrite(&cfg::esp::health,         sizeof(bool),   1, f);
        fwrite(&cfg::esp::health_text,    sizeof(bool),   1, f);
        fwrite(&cfg::esp::distance,       sizeof(bool),   1, f);
        fwrite(&cfg::esp::aimlock,        sizeof(bool),   1, f);
        fwrite(&cfg::esp::aimbot,         sizeof(bool),   1, f);
        fwrite(&cfg::esp::aimline,        sizeof(bool),   1, f);
        fwrite(&cfg::esp::ignore_knock,   sizeof(bool),   1, f);
        fwrite(&cfg::esp::head_dot,       sizeof(bool),   1, f);
        fwrite(&cfg::esp::show_fov,       sizeof(bool),   1, f);
        fwrite(&cfg::esp::name_size,      sizeof(float),  1, f);
        fwrite(&cfg::esp::weapon_size,    sizeof(float),  1, f);
        fwrite(&cfg::esp::aim_fov,        sizeof(float),  1, f);
        fwrite(&cfg::esp::head_dot_radius,sizeof(float),  1, f);
        fwrite(&cfg::esp::aimbot_type,    sizeof(int),    1, f);
        fwrite(&cfg::esp::line_type,      sizeof(int),    1, f);
        fwrite(&cfg::esp::box_type,       sizeof(int),    1, f);
        fwrite(&cfg::esp::box_col,        sizeof(ImVec4), 1, f);
        fwrite(&cfg::esp::line_col,       sizeof(ImVec4), 1, f);
        fwrite(&cfg::esp::skeleton_col,   sizeof(ImVec4), 1, f);
        fwrite(&cfg::esp::aimline_col,    sizeof(ImVec4), 1, f);
        fwrite(&cfg::esp::weapon_col,     sizeof(ImVec4), 1, f);
        fwrite(&cfg::esp::distance_col,   sizeof(ImVec4), 1, f);
        fwrite(&cfg::esp::fov_col,        sizeof(ImVec4), 1, f);
        fwrite(&cfg::esp::dot_col,        sizeof(ImVec4), 1, f);
        fclose(f);
    }

    static void do_load() {
        FILE* f = fopen("/data/local/tmp/sx_config.bin","rb");
        if (!f) return;
        fread(&cfg::esp::box,             sizeof(bool),   1, f);
        fread(&cfg::esp::line,            sizeof(bool),   1, f);
        fread(&cfg::esp::skeleton,        sizeof(bool),   1, f);
        fread(&cfg::esp::name,            sizeof(bool),   1, f);
        fread(&cfg::esp::weapon,          sizeof(bool),   1, f);
        fread(&cfg::esp::health,          sizeof(bool),   1, f);
        fread(&cfg::esp::health_text,     sizeof(bool),   1, f);
        fread(&cfg::esp::distance,        sizeof(bool),   1, f);
        fread(&cfg::esp::aimlock,         sizeof(bool),   1, f);
        fread(&cfg::esp::aimbot,          sizeof(bool),   1, f);
        fread(&cfg::esp::aimline,         sizeof(bool),   1, f);
        fread(&cfg::esp::ignore_knock,    sizeof(bool),   1, f);
        fread(&cfg::esp::head_dot,        sizeof(bool),   1, f);
        fread(&cfg::esp::show_fov,        sizeof(bool),   1, f);
        fread(&cfg::esp::name_size,       sizeof(float),  1, f);
        fread(&cfg::esp::weapon_size,     sizeof(float),  1, f);
        fread(&cfg::esp::aim_fov,         sizeof(float),  1, f);
        fread(&cfg::esp::head_dot_radius, sizeof(float),  1, f);
        fread(&cfg::esp::aimbot_type,     sizeof(int),    1, f);
        fread(&cfg::esp::line_type,       sizeof(int),    1, f);
        fread(&cfg::esp::box_type,        sizeof(int),    1, f);
        fread(&cfg::esp::box_col,         sizeof(ImVec4), 1, f);
        fread(&cfg::esp::line_col,        sizeof(ImVec4), 1, f);
        fread(&cfg::esp::skeleton_col,    sizeof(ImVec4), 1, f);
        fread(&cfg::esp::aimline_col,     sizeof(ImVec4), 1, f);
        fread(&cfg::esp::weapon_col,      sizeof(ImVec4), 1, f);
        fread(&cfg::esp::distance_col,    sizeof(ImVec4), 1, f);
        fread(&cfg::esp::fov_col,         sizeof(ImVec4), 1, f);
        fread(&cfg::esp::dot_col,         sizeof(ImVec4), 1, f);
        fclose(f);
    }

    // ════════════════════════════════════════════════════
    //   TAB 1: Visuals > ESP
    // ════════════════════════════════════════════════════
    static void esp_tab(float a) {
        ImGui::Checkbox("Show  Box",         &cfg::esp::box);
        ImGui::Checkbox("Show  Line",        &cfg::esp::line);
        ImGui::Checkbox("Show  Skeleton",    &cfg::esp::skeleton);
        ImGui::Checkbox("Show  Name",        &cfg::esp::name);
        ImGui::Checkbox("Show  Weapon",      &cfg::esp::weapon);
        ImGui::Checkbox("Show  Health bar",  &cfg::esp::health);
        ImGui::Checkbox("Show  Health Text", &cfg::esp::health_text);
        ImGui::Checkbox("Show  Distance",    &cfg::esp::distance);
    }

    // ════════════════════════════════════════════════════
    //   TAB 2: Config
    // ════════════════════════════════════════════════════
    static void config_tab(float a) {
        slider("Name  size",     &cfg::esp::name_size, 0.0f, 15.0f, a, "%.0f");
        separator(a);
        slider("Weapon  size",   &cfg::esp::weapon_size, 5.0f, 30.0f, a, "%.0f");
        separator(a);
        colorpick("Line  color",     &cfg::esp::line_col,     a);
        separator(a);
        colorpick("Box  color",      &cfg::esp::box_col,      a);
        separator(a);
        colorpick("Skeleton  color", &cfg::esp::skeleton_col, a);
        separator(a);
        colorpick("Name  color",     &cfg::esp::name_col,     a);
        separator(a);
        colorpick("Weapon  color",   &cfg::esp::weapon_col,   a);
        separator(a);
        colorpick("Distance  color", &cfg::esp::distance_col, a);
        separator(a);
        combo("Line  type", &cfg::esp::line_type,
              {"Top", "Centre", "Bottom"}, a);
        separator(a);
        combo("Box  type",  &cfg::esp::box_type,
              {"Normal", "Filled", "Corner", "Rounded"}, a);
    }

    // ════════════════════════════════════════════════════
    //   TAB 0: Aimbot
    // ════════════════════════════════════════════════════
    static void aimbot_tab(float a) {
        ImGui::Checkbox("Aimlock",                &cfg::esp::aimlock);
        ImGui::Checkbox("Aimbot  (while firing)", &cfg::esp::aimbot);
        if (cfg::esp::aimbot) {
            separator(a);
            combo("Aim  Position", &cfg::esp::aimbot_type,
                  {"Head", "Neck", "Chest"}, a);
        }
        separator(a);
        slider("Aim  FOV", &cfg::esp::aim_fov, 10.f, 360.f, a, "%.0f");
        separator(a);
        ImGui::Checkbox("Ignore  Knock", &cfg::esp::ignore_knock);
        ImGui::Checkbox("Aim Line",       &cfg::esp::aimline);
        if (cfg::esp::aimline) {
            separator(a);
            colorpick("Aimline  color",  &cfg::esp::aimline_col,  a);
        }
        ImGui::Checkbox("Head Dot",       &cfg::esp::head_dot);
        if (cfg::esp::head_dot) {
            separator(a);
            slider("Dot Radius", &cfg::esp::head_dot_radius, 1.0f, 20.0f, a, "%.0f");
            separator(a);
            colorpick("Dot Color", &cfg::esp::dot_col, a);
        }
        separator(a);
        ImGui::Checkbox("Show  FOV",     &cfg::esp::show_fov);
        if (cfg::esp::show_fov) {
            separator(a);
            colorpick("FOV  Color", &cfg::esp::fov_col, a);
        }
    }

    // ════════════════════════════════════════════════════
    //   TAB 3: Settings — NEW (without scrollbar)
    //   Features: Build info, Screen, FPS, Dev, Save/Load,
    //   Screenshot, Record, Exit
    // ════════════════════════════════════════════════════
    static void settings_tab(ImDrawList* dl, ImVec2 base, float pw, float a) {
        float pad = 20.f;
        float gap = 8.f;
        float y = base.y + 8.f;

        // 1. Build info
        char buf[128];
        snprintf(buf, sizeof(buf), "Build: %s %s", __DATE__, __TIME__);
        dl->AddText(fontMedium, 24.f,
            ImVec2(base.x + pad, y),
            ImColor(g_menu[0], g_menu[1], g_menu[2], a), buf);
        y += 30.f;

        // 2. Screen resolution
        snprintf(buf, sizeof(buf), "Screen: %.0f x %.0f", g_sw, g_sh);
        dl->AddText(fontMedium, 22.f,
            ImVec2(base.x + pad, y),
            ImColor(200, 200, 200, (int)(255 * a)), buf);
        y += 28.f;

        // 3. FPS
        snprintf(buf, sizeof(buf), "FPS: %.0f", ImGui::GetIO().Framerate);
        dl->AddText(fontMedium, 22.f,
            ImVec2(base.x + pad, y),
            ImColor(200, 200, 200, (int)(255 * a)), buf);
        y += 28.f;

        // Separator
        dl->AddLine(
            ImVec2(base.x + pad, y),
            ImVec2(base.x + pw - pad, y),
            ImColor(60, 60, 60, (int)(200 * a)), 1.f);
        y += 16.f;

        // 4. Dev By Triple Boys
        dl->AddText(fontMedium, 22.f,
            ImVec2(base.x + pad, y),
            ImColor(170, 130, 255, (int)(255 * a)), "Dev By Triple Boys");
        y += 34.f;

        // ---- Buttons ----
        float bw = pw - pad * 2.f;
        float bh = 50.f;
        float by = y;

        auto draw_btn = [&](const char* label, bool accent, std::function<void()> action) {
            ImVec2 b1(base.x + pad, by);
            ImVec2 b2(base.x + pad + bw, by + bh);
            bool hov = ImGui::IsMouseHoveringRect(b1, b2);
            bool clk = hov && ImGui::IsMouseClicked(0);

            if (accent) {
                ImColor fill = hov ? ImColor(g_menu[0] * 1.2f, g_menu[1] * 1.2f, g_menu[2] * 1.2f, a)
                                   : ImColor(g_menu[0], g_menu[1], g_menu[2], a);
                dl->AddRectFilled(b1, b2, fill, 8.f);
            } else {
                dl->AddRectFilled(b1, b2,
                    hov ? ImColor(30, 30, 30, (int)(230 * a))
                        : ImColor(14, 14, 14, (int)(200 * a)), 8.f);
                dl->AddRect(b1, b2,
                    hov ? ImColor(g_menu[0], g_menu[1], g_menu[2], a)
                        : ImColor(55, 55, 55, (int)(200 * a)),
                    8.f, 0, 1.4f);
            }

            ImVec2 lsz = ImGui::CalcTextSize(label);
            dl->AddText(fontMedium, 26.f,
                ImVec2(b1.x + (bw - lsz.x) * 0.5f, b1.y + (bh - 26.f) * 0.5f),
                ImColor(255, 255, 255, (int)(230 * a)), label);

            by += bh + gap;
            if (clk && action) action();
            return clk;
        };

        // 5. Save Settings
        draw_btn("Save Settings", true, []() { do_save(); });

        // 6. Load Settings
        draw_btn("Load Settings", true, []() { do_load(); });

        // 7. Take Screenshot
        draw_btn("Take Screenshot", false, []() {
            system("su -c '"
                   "mkdir -p /sdcard/Download && "
                   "screencap -p /sdcard/Download/Infinity_$(date +%Y%m%d_%H%M%S).png"
                   "'");
        });

        // 8. Record Screen (toggle with timer & red dot)
        {
            static bool  is_recording   = false;
            static float rec_start_time = 0.f;

            const char* rec_label = is_recording ? "Stop Recording" : "Record Screen";
            ImVec2 b1(base.x + pad, by);
            ImVec2 b2(base.x + pad + bw, by + bh);
            bool hov = ImGui::IsMouseHoveringRect(b1, b2);
            bool clk = hov && ImGui::IsMouseClicked(0);

            if (is_recording) {
                ImColor rc = hov ? ImColor(200, 30, 30, (int)(230 * a))
                                 : ImColor(160, 20, 20, (int)(220 * a));
                dl->AddRectFilled(b1, b2, rc, 8.f);
                dl->AddRect(b1, b2, ImColor(220, 60, 60, (int)(255 * a)), 8.f, 0, 1.4f);
            } else {
                dl->AddRectFilled(b1, b2,
                    hov ? ImColor(30, 30, 30, (int)(230 * a))
                        : ImColor(14, 14, 14, (int)(200 * a)), 8.f);
                dl->AddRect(b1, b2,
                    hov ? ImColor(g_menu[0], g_menu[1], g_menu[2], a)
                        : ImColor(55, 55, 55, (int)(200 * a)),
                    8.f, 0, 1.4f);
            }

            if (is_recording) {
                float blink = (sinf(ImGui::GetTime() * 5.f) + 1.f) * 0.5f;
                dl->AddCircleFilled(
                    ImVec2(b1.x + 30.f, b1.y + bh * 0.5f),
                    7.f,
                    ImColor(255, 60, 60, (int)(255 * blink * a)));

                float elapsed = ImGui::GetTime() - rec_start_time;
                int mins = (int)(elapsed / 60.f);
                int secs = (int)(elapsed) % 60;
                char tbuf[16];
                snprintf(tbuf, sizeof(tbuf), " %02d:%02d", mins, secs);

                ImVec2 lsz  = ImGui::CalcTextSize(rec_label);
                ImVec2 tsz2 = ImGui::CalcTextSize(tbuf);
                float total_w = lsz.x + tsz2.x;
                float base_x  = b1.x + (bw - total_w) * 0.5f;

                dl->AddText(fontMedium, 26.f,
                    ImVec2(base_x, b1.y + (bh - 26.f) * 0.5f),
                    ImColor(255, 255, 255, (int)(230 * a)), rec_label);
                dl->AddText(fontMedium, 26.f,
                    ImVec2(base_x + lsz.x, b1.y + (bh - 26.f) * 0.5f),
                    ImColor(255, 120, 120, (int)(230 * a)), tbuf);
            } else {
                ImVec2 lsz = ImGui::CalcTextSize(rec_label);
                dl->AddText(fontMedium, 26.f,
                    ImVec2(b1.x + (bw - lsz.x) * 0.5f, b1.y + (bh - 26.f) * 0.5f),
                    ImColor(255, 255, 255, (int)(230 * a)), rec_label);
            }

            by += bh + gap;
            if (clk) {
                if (!is_recording) {
                    is_recording   = true;
                    rec_start_time = ImGui::GetTime();
                    system("su -c '"
                           "mkdir -p /sdcard/Download && "
                           "screenrecord /sdcard/Download/Infinity_$(date +%Y%m%d_%H%M%S).mp4 &"
                           "'");
                } else {
                    is_recording = false;
                    system("su -c 'pkill -SIGINT screenrecord'");
                }
            }
        }

        // 9. Exit
        draw_btn("Exit", false, []() { exit(0); });
    }

    // ── Subtab renderer (Visuals tab only) ──
    static void draw_subtabs(ImDrawList* dl, ImVec2 amin, ImVec2 amax, float a) {
        if (tab != 1) return;  // Visuals = tab 1

        float pad = 8.f;
        float tw  = (amax.x-amin.x-pad*2)/vstc;

        for (int i=0; i<vstc; i++) {
            float tx=amin.x+pad+i*tw;
            float ty=amin.y+4.f;
            float w =tw-4.f;
            float h =sth-8.f;
            ImVec2 tmin(tx,ty), tmax(tx+w,ty+h);
            bool sel=(stab==i);
            bool hov=ImGui::IsMouseHoveringRect(tmin,tmax);

            float sa=anim("st_"+std::to_string(i),sel?1.f:0.f,14.f);
            float ha=anim("sth_"+std::to_string(i),hov&&!sel?0.4f:0.f,14.f);

            if (sa>0.01f) {
                dl->AddRectFilled(tmin,tmax,IM_COL32(34,33,34,(int)(255*sa*a)));
                dl->AddRect(tmin,tmax,IM_COL32(255,255,255,(int)(30*sa*a)));
                dl->AddLine(
                    ImVec2(tmin.x+w*0.2f,tmax.y),
                    ImVec2(tmax.x-w*0.2f,tmax.y),
                    ImColor(g_menu[0],g_menu[1],g_menu[2],sa*a),2.f);
            } else if (ha>0.01f) {
                dl->AddRectFilled(tmin,tmax,IM_COL32(26,26,26,(int)(255*ha*a)));
            }

            ImVec4 tc2=anim_col("stt_"+std::to_string(i),
                sel?ImVec4(g_menu[0],g_menu[1],g_menu[2],1.f)
                   :ImVec4(0.35f,0.35f,0.35f,1.f),14.f);

            ImGui::PushFont(fontMedium);
            ImVec2 tsz=ImGui::CalcTextSize(vstabs[i]);
            dl->AddText(ImVec2(tx+(w-tsz.x)*0.5f,ty+(h-tsz.y)*0.5f),col(tc2,a),vstabs[i]);
            ImGui::PopFont();

            if (hov&&ImGui::IsMouseClicked(0)&&!popup()&&!tsw&&stab!=i) {
                ttab=tab; tstab=i; tsw=true;
                scr_tgt=0.f; scr_cur=0.f;
            }
        }
    }

    // ════════════════════════════════════════════════════
    //   MAIN RENDER
    // ════════════════════════════════════════════════════
    void render() {
        bar::render();

        float dt=ImGui::GetIO().DeltaTime;

        // ── ma: menu open/close alpha (0→1) ──
        ma=lrp(ma,bar::g_open?1.f:0.f,ImClamp(12.f*dt,0.f,1.f));
        if (ma<0.01f) return;

        tick();

        if (tsw) {
            ta=lrp(ta,0.f,ImClamp(18.f*dt,0.f,1.f));
            if (ta<0.05f) {
                tab=ttab; stab=tstab; tsw=false;
                scr_tgt=0.f; scr_cur=0.f;
            }
        } else {
            ta=lrp(ta,1.f,ImClamp(14.f*dt,0.f,1.f));
        }

        float ca=ma*ta;
        content_alpha=1.f;

        // ── SCALE/ZOOM ANIMATION ──
        auto ease_zoom = [](float t) -> float {
            return -(cosf(3.14159265f * t) - 1.f) / 2.f;
        };
        float ez    = ease_zoom(ma);
        float scale = 0.82f + ez * 0.18f;

        float wsz_w = mw * scale;
        float wsz_h = mh * scale;
        float wx = (g_sw - wsz_w) * 0.5f;
        float wy = (g_sh - wsz_h) * 0.5f;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ma);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0,0,0,0));

        ImVec2 wsz(wsz_w, wsz_h);
        ImGui::SetNextWindowSize(wsz, ImGuiCond_Always);
        if (scale < 0.999f) {
            ImGui::SetNextWindowPos(ImVec2(wx, wy), ImGuiCond_Always);
        } else {
            ImGui::SetNextWindowPos(ImVec2(wx, wy), ImGuiCond_Once);
        }

        ImGuiWindowFlags wf=
            ImGuiWindowFlags_NoTitleBar  |ImGuiWindowFlags_NoResize    |
            ImGuiWindowFlags_NoScrollbar |ImGuiWindowFlags_NoCollapse  |
            ImGuiWindowFlags_NoMove;

        if (ImGui::Begin("##m",nullptr,wf)) {

            ImVec2      wp =ImGui::GetWindowPos();
            ImDrawList* dl =ImGui::GetWindowDrawList();
            ImGuiIO&    io =ImGui::GetIO();
            ImVec2      mp =io.MousePos;

            // Shadow + BG + Border
            dl->AddShadowRect(
                ImVec2(wp.x+30,wp.y+30),
                ImVec2(wp.x+wsz.x-30,wp.y+wsz.y-30),
                ImColor(g_menu[0],g_menu[1],g_menu[2]),
                60,ImVec2(0.f,0.f),6.f);
            dl->AddRectFilled(
                ImVec2(wp.x+30,wp.y+30),
                ImVec2(wp.x+wsz.x-30,wp.y+wsz.y-30),
                ImColor(0,0,0,255),6.f);
            dl->AddRect(
                ImVec2(wp.x+30,wp.y+30),
                ImVec2(wp.x+wsz.x-30,wp.y+wsz.y-30),
                ImColor(90,90,90,255),
                6.f,ImDrawFlags_RoundCornersAll,2.f);

            // Header line
            dl->AddLine(ImVec2(wp.x+30,wp.y+hh),
                        ImVec2(wp.x+wsz.x-30,wp.y+hh),
                        ImColor(0,0,0,255),1.f);
            dl->AddLine(ImVec2(wp.x+30,wp.y+hh+1),
                        ImVec2(wp.x+wsz.x-30,wp.y+hh+1),
                        ImColor(90,90,90,(int)(50*ma)),1.f);

            // Title — red
            dl->AddText(fontBold,22.f,
                ImVec2(wp.x+50,wp.y+36),
                ImColor(0,   191, 255, 255), "Infinity - X");
            dl->AddText(fontBold,18.f,
                ImVec2(wp.x+50,wp.y+61),
                ImColor(0,   150, 200, 255), "- X");

            // Footer
            dl->AddLine(ImVec2(wp.x+30,wp.y+wsz.y-fh),
                        ImVec2(wp.x+wsz.x-30,wp.y+wsz.y-fh),
                        ImColor(90,90,90,(int)(50*ma)),1.f);
            ImGui::PushFont(fontDesc);
            time_t now2=time(0); tm* ltm=localtime(&now2);
            char tb[32];
            snprintf(tb,sizeof(tb),"%02d:%02d:%02d",ltm->tm_hour,ltm->tm_min,ltm->tm_sec);
            dl->AddText(ImVec2(wp.x+40,wp.y+wsz.y-fh+(fh-fontDesc->FontSize)*0.5f),
                        ImColor(90,90,90,255),"Time ");
            ImVec2 lsz=ImGui::CalcTextSize("Time ");
            dl->AddText(ImVec2(wp.x+40+lsz.x,wp.y+wsz.y-fh+(fh-fontDesc->FontSize)*0.5f),
                        ImColor(g_menu[0],g_menu[1],g_menu[2],ma),tb);
            ImGui::PopFont();

            // Sidebar
            ImVec2 smin(wp.x+30+1,wp.y+hh+2);
            ImVec2 smax(wp.x+30+sw,wp.y+wsz.y-fh);
            dl->AddRectFilled(smin,smax,ImColor(8,8,8,255));
            dl->AddLine(ImVec2(smax.x,smin.y),ImVec2(smax.x,smax.y),
                        ImColor(90,90,90,255),1.f);

            // TAB buttons
            float tsy=smin.y+16;
            float th =50.f;
            float tg =6.f;
            float tp =16.f;

            for (int i=0; i<tc; i++) {
                float ty2=tsy+i*(th+tg);
                float tx2=smin.x+6;
                float tw2=sw-14;
                ImVec2 tmin2(tx2,ty2),tmax2(tx2+tw2,ty2+th);
                bool sel=(tab==i);
                bool hov=ImGui::IsMouseHoveringRect(tmin2,tmax2);

                float sa=anim("t_"+std::to_string(i),sel?1.f:0.f,14.f);
                float ha=anim("th_"+std::to_string(i),hov&&!sel?0.4f:0.f,14.f);

                if (sa>0.01f) {
                    dl->AddRectFilled(tmin2,tmax2,IM_COL32(34,33,34,(int)(255*sa*ma)));
                    dl->AddRect(tmin2,tmax2,IM_COL32(255,255,255,(int)(30*sa*ma)));
                    dl->AddRectFilledMultiColor(
                        tmin2,ImVec2(tmin2.x+3,tmax2.y),
                        ImColor(g_menu[0],g_menu[1],g_menu[2],sa*ma),
                        ImColor(g_menu[0],g_menu[1],g_menu[2],0.f),
                        ImColor(g_menu[0],g_menu[1],g_menu[2],0.f),
                        ImColor(g_menu[0],g_menu[1],g_menu[2],sa*ma));
                } else if (ha>0.01f) {
                    dl->AddRectFilled(tmin2,tmax2,IM_COL32(20,20,20,(int)(255*ha*ma)));
                }

                ImVec4 ttc=anim_col("tt_"+std::to_string(i),
                    sel?ImVec4(g_menu[0],g_menu[1],g_menu[2],1.f)
                       :ImVec4(0.35f,0.35f,0.35f,1.f),14.f);

                if (F48) {
                    ImGui::PushFont(F48);
                    float iy = ty2 + (th - F48->FontSize) * 0.5f;
                    dl->AddText(ImVec2(tx2 + 8.f, iy), col(ttc, ma), tab_icons[i]);
                    ImGui::PopFont();
                }
                ImGui::PushFont(fontMedium);
                float ny = ty2 + (th - fontMedium->FontSize) * 0.5f;
                dl->AddText(ImVec2(tx2 + tp + 20.f, ny), col(ttc, ma), tabs[i]);
                ImGui::PopFont();

                if (hov&&ImGui::IsMouseClicked(0)&&ma>0.5f&&!tsw) {
                    if (!popup()&&tab!=i) {
                        ttab=i; tstab=0; tsw=true;
                        scr_tgt=0.f; scr_cur=0.f;
                        close();
                    }
                }
            }

            // Content layout
            float csx2=wp.x+30+sw+1;
            float cex2=wp.x+wsz.x-30;

            // Subtab only for Visuals (tab==1)
            float hst=(tab==1)?1.f:0.f;
            float sah=sth*hst;

            ImVec2 stmin2(csx2,wp.y+hh+2);
            ImVec2 stmax2(cex2,stmin2.y+sah);

            if (tab==1) {
                dl->AddRectFilled(stmin2,stmax2,ImColor(10,10,10,255));
                dl->AddLine(ImVec2(stmin2.x,stmax2.y),
                            ImVec2(stmax2.x,stmax2.y),
                            ImColor(90,90,90,255),1.f);
                draw_subtabs(dl,stmin2,stmax2,ma);
            }

            float cp  =14.f;
            float sbw2=20.f;
            float sbg2=10.f;

            content_w = cex2 - csx2 - cp*2 - sbg2 - sbw2 - 15.f;

            float cw  =content_w;
            float csy2=stmax2.y+cp;
            float ch  =(wp.y+wsz.y-fh)-csy2-cp;

            ImVec2 cpos(csx2+cp,csy2);
            ImVec2 cmax2(cpos.x+cw,cpos.y+ch);

            if (tab==3) {
                // Settings — direct draw, NO scrollbar
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ca);
                settings_tab(dl, cpos, cw+sbg2+sbw2, ca);
                ImGui::PopStyleVar();
            } else {
                // Other tabs — scrollable with scrollbar (keep as is)
                ImGui::SetCursorScreenPos(cpos);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ca);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,0));
                ImGui::PushStyleColor(ImGuiCol_ChildBg,ImVec4(0,0,0,0));
                ImGui::PushClipRect(cpos,cmax2,true);

                content_alpha=ta;

                ImGui::BeginChild("","##c",ImVec2(cw,ch));

                float sm=ImGui::GetScrollMaxY();

                bool chov=ImGui::IsMouseHoveringRect(cpos,cmax2);
                if (chov&&!popup()) {
                    float wh=io.MouseWheel;
                    if (wh!=0.f) scr_tgt-=wh*60.f;
                }
                scr_tgt=ImClamp(scr_tgt,0.f,ImMax(sm,0.f));
                scr_cur=lrp(scr_cur,scr_tgt,ImClamp(16.f*dt,0.f,1.f));
                if (fabsf(scr_cur-scr_tgt)<0.5f) scr_cur=scr_tgt;
                ImGui::SetScrollY(scr_cur);

                // Content render
                switch (tab) {
                    case 0: aimbot_tab(ma); break;
                    case 1: esp_tab(ma);    break;
                    case 2: config_tab(ma); break;
                }

                sm=ImGui::GetScrollMaxY();
                scr_tgt=ImClamp(scr_tgt,0.f,ImMax(sm,0.f));

                ImGui::EndChild();
                ImGui::PopClipRect();
                ImGui::PopStyleColor(1);
                ImGui::PopStyleVar(2);

                // Old style scrollbar
                float sbx2=cmax2.x+sbg2;
                float sby2=cpos.y;
                float sbh2=ch;

                if (sm>0.f) {
                    dl->AddRectFilledMultiColor(
                        ImVec2(sbx2+2,sby2+2),ImVec2(sbx2+sbw2-2,sby2+sbh2-2),
                        IM_COL32(24,23,24,(int)(255*ma)),
                        IM_COL32(24,23,24,(int)(255*ma)),
                        IM_COL32(18,18,18,(int)(255*ma)),
                        IM_COL32(18,18,18,(int)(255*ma)));
                    dl->AddRect(ImVec2(sbx2,sby2),ImVec2(sbx2+sbw2,sby2+sbh2),
                                IM_COL32(255,255,255,(int)(20*ma)));
                    dl->AddRect(ImVec2(sbx2+1,sby2+1),ImVec2(sbx2+sbw2-1,sby2+sbh2-1),
                                IM_COL32(0,0,0,(int)(255*ma)));

                    float sr=sbh2/(sbh2+sm);
                    float gh=sbh2*sr;
                    gh=ImMax(gh,40.f);
                    float sn=scr_tgt/sm;
                    float gy=sby2+(sbh2-gh)*sn;

                    dl->AddRectFilledMultiColor(
                        ImVec2(sbx2+2,gy+2),ImVec2(sbx2+sbw2-2,gy+gh-2),
                        ImColor(g_menu[0],g_menu[1],g_menu[2],ma),
                        ImColor(g_menu[0],g_menu[1],g_menu[2],ma),
                        ImColor(g_menu[0]*0.78f,g_menu[1]*0.78f,g_menu[2]*0.78f,ma),
                        ImColor(g_menu[0]*0.78f,g_menu[1]*0.78f,g_menu[2]*0.78f,ma));

                    float sb_hitbox_pad=30.f;
                    ImRect sbr2(
                        ImVec2(sbx2-sb_hitbox_pad,sby2),
                        ImVec2(sbx2+sbw2+10.f,sby2+sbh2));
                    bool sbhov=ImGui::IsMouseHoveringRect(sbr2.Min,sbr2.Max);
                    static bool  sbd =false;
                    static float sbds=0.f;

                    if (sbhov&&ImGui::IsMouseClicked(0)&&!popup()) {
                        sbd=true; sbds=mp.y-gy;
                    }
                    if (!io.MouseDown[0]) sbd=false;
                    if (sbd&&sm>0.f) {
                        float ngy=mp.y-sbds;
                        float nn=(ngy-sby2)/(sbh2-gh);
                        nn=ImClamp(nn,0.f,1.f);
                        scr_tgt=nn*sm;
                        scr_cur=scr_tgt;
                    }
                }
            }

            // Drag
            bool inm=(mp.x>=wp.x&&mp.x<=wp.x+wsz.x
                   && mp.y>=wp.y&&mp.y<=wp.y+wsz.y);

            if (inm&&!popup()&&ImGui::IsMouseClicked(0)) {
                bool ont=false;
                for (int i=0; i<tc; i++) {
                    float ty2=tsy+i*(th+tg);
                    float tx2=smin.x+6;
                    float tw2=sw-14;
                    if (mp.x>=tx2&&mp.x<=tx2+tw2
                     && mp.y>=ty2&&mp.y<=ty2+th)
                        {ont=true; break;}
                }
                bool inst=(tab==1
                        && mp.y>=stmin2.y&&mp.y<=stmax2.y
                        && mp.x>=stmin2.x);
                bool inca=(mp.x>=cpos.x&&mp.x<=cmax2.x
                        && mp.y>=cpos.y&&mp.y<=cmax2.y);
                float sbx_d=cmax2.x+sbg2;
                float sby_d=cpos.y;
                float sbh_d=ch;
                bool insb=(mp.x>=sbx_d-30.f&&mp.x<=sbx_d+sbw2+10.f
                        && mp.y>=sby_d&&mp.y<=sby_d+sbh_d);

                if (!ont&&!inca&&!insb&&!inst) {
                    drag=true;
                    doff=ImVec2(mp.x-wp.x,mp.y-wp.y);
                }
            }

            if (drag) {
                if (io.MouseDown[0]) {
                    ImVec2 np(mp.x-doff.x,mp.y-doff.y);
                    np.x=ImClamp(np.x,0.f,g_sw-wsz.x);
                    np.y=ImClamp(np.y,0.f,g_sh-wsz.y);
                    ImGui::SetWindowPos("##m",np);
                } else { drag=false; }
            }

            if (popup()) drag=false;
        }
        ImGui::End();
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(4);
        popups();
    }

} // namespace ui::menu