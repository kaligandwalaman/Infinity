#include <termios.h>
#include <fstream>     // file read/write ke liye
#include <string>          // ← std::string ke liye
#include <thread>          // ← std::thread ke liye
#include <fcntl.h>         // ← open(), O_RDONLY ke liye
#include "Android_draw/draw.h"
#include "ui/theme/theme.hpp"
#include "ui/menu.hpp"
#include "ui/bar.hpp"
#include "other/memory.hpp"
#include "game/game.hpp"
#include "func/visuals.hpp"
#include <linux/input.h>
#include <cstdio>
#include <unistd.h>
#include <signal.h>  // Process check karne ke liye
#include "login/struct.h"     // ← yeh add karo

// FFM Globals
#define PERM_EXEC "r-xp"
#define targetaLibName "libil2cpp.so"
#define targetaLibName2 "libunity.so"


int game_pid = -1;
uint64_t lib_base = 0;
uint64_t lib_base2 = 0;
char packageName[] = "com.dts.freefiremax";
int abs_ScreenX = 0;
int abs_ScreenY = 0;

static bool main_thread_flag = true;

static void print_status(const char* status) {
    printf("\033[2J\033[H\033[1;38;2;162;144;225m[FFMax]\033[0m \033[1;37m%s\033[0m\n", status);
}

bool isGameRunning() {
    if (game_pid <= 0) return false;
    
    // Check if process still exists
    if (kill(game_pid, 0) != 0) {
        // Process died
        game_pid = -1;
        lib_base = 0;
        lib_base2 = 0;
        return false;
    }
    return (lib_base != 0);
}


int main() {
    // 🔥 SABSE PEHLE LOGIN (MT Manager terminal mein dikhega)
    if (!checkOrSaveKey()) {
        return -1;
    }

    // Normal game init (pura code same)
        // Stand-off style: Exit nahi karna agar game nahi mili toh
    game_pid = getProcessID(packageName);
    if (game_pid > 0) {
        lib_base = get_module_base(targetaLibName, PERM_EXEC);
        lib_base2 = get_module_base(targetaLibName2, PERM_EXEC);
    }
    
    // Agar abhi nahi mili toh bhi chalega, loop mein retry karega

    screen_config();
    
    int max_size = (displayInfo.height > displayInfo.width ? displayInfo.height : displayInfo.width);
    int min_size = (displayInfo.height < displayInfo.width ? displayInfo.height : displayInfo.width);

    g_sw = static_cast<float>(max_size);
    g_sh = static_cast<float>(min_size);
    abs_ScreenX = max_size;
    abs_ScreenY = min_size;
    native_window_screen_x = max_size;
    native_window_screen_y = max_size;

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) {
        return -1;
    }

    touch::init(max_size, min_size, (uint8_t)displayInfo.orientation);

    // 🔥 Volume listener thread start (background mein)
    std::thread vol_thread(volume_listener);
    vol_thread.detach();

    print_status("Game Not Running");  // Ya phir "Game Not Running" likh do

    // Baaki pura loop same (while(main_thread_flag) wala)
    static float alpha = 0.f;
    static bool prev = false;
    static int frame = 0;

    while (main_thread_flag) {
    drawBegin();
    
    // 🔥 YEH SABSE IMPORTANT HAI - Har frame pe check karo
    // Agar game nahi chal rahi toh dhundho
    if (game_pid <= 0 || !isGameRunning()) {
        game_pid = getProcessID(packageName);
        if (game_pid > 0) {
            lib_base = get_module_base(targetaLibName, PERM_EXEC);
            lib_base2 = get_module_base(targetaLibName2, PERM_EXEC);
        }
    }
    
    // Ab check karo
    bool run = (game_pid > 0 && lib_base != 0 && kill(game_pid, 0) == 0);
    
    // Status update
    if (run && !prev) {
    // 🔥 YEH ADD KARO - Game structures initialize karo
    if (game::init()) {
        print_status("game detected");
        prev = true;
    } else {
        // Game process hai lekin structures load nahi hue
        // Retry next frame
        lib_base = 0;  // Force re-load
    }
    } else if (!run && prev) {
        print_status("game closed");
        game::reset();  // 🔥 Reset game cache
        prev = false;
        // 🔥 YEH LINE ADD KARO: Game close hote hi cheat process ko band karne ke liye
        main_thread_flag = false; 
    }
    
#if defined(__x86_64__)
        bool is_landscape = (displayInfo.orientation == 0 || displayInfo.orientation == 2);
#else
        bool is_landscape = (displayInfo.orientation == 1 || displayInfo.orientation == 3);
#endif

        if (is_landscape) {
            ImGuiIO& io = ImGui::GetIO();
            float dt = io.DeltaTime;
            if (dt <= 0.f || dt > 0.1f) dt = 0.016f;

            float target = run ? 1.f : 0.f;
            float spd = run ? 4.f : 6.f;

            if (alpha < target) {
                alpha += dt * spd;
                if (alpha > target) alpha = target;
            } else if (alpha > target) {
                alpha -= dt * spd;
                if (alpha < target) alpha = target;
            }

            ui::bar::set_game_alpha(alpha);

            if (alpha > 0.001f) ui::menu::render();
            if (run && lib_base != 0) visuals::draw();
        }

        bool vis = ui::bar::g_open;
        drawEnd();
        usleep(vis ? 1500 : 4000);
        
        if (frame % 300 == 0) {
        }
    }

    printf("[SHUTDOWN] Exiting...\n");
    shutdown();
    return 0;
}