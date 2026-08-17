#include "../includes/tools/Tools.h"      
#include "../includes/obfuscate.h"
#include "../includes/json.h"
#include <curl/curl.h>
#include <sys/time.h>
#include "struct.h"
#include <fstream>
#include <string>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <cstring>
#include <linux/input.h>
#include <cctype>      // for isalnum
#include <algorithm>   // for find

#include "../src/ui/bar.hpp"

// Global license info
LicenseInfo g_license;

using json = nlohmann::json;

const std::string KEY_FILE_PATH = "/storage/emulated/0/Documents/DigiLocker/.nomedia";

// ============ SNIFFER / PACKET CAPTURE APP DETECTION ============
bool isSnifferActive() {
    // List of forbidden package names
    std::vector<std::string> forbiddenPackages = {
        "com.ZENIO",
        "com.network.proxy",
        "com.guoshi.httpcanary",
        "com.guoshi.httpcanary.pro",
        "com.emanuelef.remote_capture",
        "app.greyshirts.sslcapture",
        "com.reqable.android",
        "com.realsignal.packetcapturepro",
        "com.minhui.networkcapture",
        "jp.co.taosoftware.android.packetcapture",
        "com.wirenmicro.sniffer",
        "com.toxic.canary",
        "com.toxic.canary.pro",
        "com.wolf.canary",
        "com.hndev.httpcanary",
        "com.kuro.canary"
    };

    // Android background running process directories ko scan karenge (/proc)
    DIR* dir = opendir("/proc");
    if (dir == nullptr) return false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Sirf wahi folders check karne hain jo numbers (PIDs) hain
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            std::string cmdlinePath = std::string("/proc/") + entry->d_name + "/cmdline";
            std::ifstream cmdFile(cmdlinePath);
            if (cmdFile.is_open()) {
                std::string processName;
                std::getline(cmdFile, processName);
                cmdFile.close();

                // Process name ke sath packages match karenge
                for (const auto& pkg : forbiddenPackages) {
                    if (processName.find(pkg) != std::string::npos) {
                        closedir(dir);
                        return true; // Blacklisted sniffer app active hai
                    }
                }
            }
        }
    }
    closedir(dir);
    return false;
}

// ============ 100% ACCURATE NATIVE SOCKET VPN CHECK ============
bool isVpnActive() {
    bool vpnDetected = false;
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0) {
        struct sockaddr_in loopback;
        loopback.sin_family = AF_INET;
        loopback.sin_addr.s_addr = inet_addr("8.8.8.8");
        loopback.sin_port = htons(53);

        if (connect(sock, (struct sockaddr*)&loopback, sizeof(loopback)) == 0) {
            struct sockaddr_in local_addr;
            socklen_t addr_len = sizeof(local_addr);
            
            if (getsockname(sock, (struct sockaddr*)&local_addr, &addr_len) == 0) {
                FILE* fp = fopen("/proc/net/route", "r");
                if (fp) {
                    char line[256];
                    while (fgets(line, sizeof(line), fp)) {
                        char iface[32];
                        if (sscanf(line, "%31s", iface) == 1) {
                            std::string ifaceStr(iface);
                            if (ifaceStr == "tun0" || ifaceStr == "ppp0" || ifaceStr == "tun1") {
                                vpnDetected = true;
                                break;
                            }
                        }
                    }
                    fclose(fp);
                }
            }
        }
        close(sock);
    }
    
    return vpnDetected;
}

// ============ CURL CALLBACK ============
struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

// ============ ONLINE VERIFY FUNCTION ============
bool verifyKeyOnline(const std::string& user_key, std::string& errorMsg) {
    // Security check: VPN aur Sniffer dono blocked hain ya nahi
    if (isVpnActive() || isSnifferActive()) {
        errorMsg = "Security Violation: VPN or Sniffer Tools Detected!";
        printf("\033[1;31m[✕] Connection Blocked: Disable VPN or Packet Capture Tools!\033[0m\n");
        return false;
    }

    std::string full_hwid = Tools::GetDeviceUniqueIdentifier("");
    std::string UUID = Tools::CalcMD5(full_hwid);

    struct MemoryStruct chunk{};
    chunk.memory = (char *)malloc(1);
    chunk.size = 0;

    CURL *curl = curl_easy_init();
    bool bValid = false;

    if (curl) {
        std::string url = (const char*)OBFUSCATE("https://eagle.arisugpt.in/connect");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, OBFUSCATE("Content-Type: application/x-www-form-urlencoded"));
        headers = curl_slist_append(headers, OBFUSCATE("X-API-Key: X7B4N2P8Q9W3Z6M5"));
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        char postData[4096];
        sprintf(postData, OBFUSCATE("game=PUBG&user_key=%s&serial=%s"), 
                user_key.c_str(), UUID.c_str());

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK) {            
            try {
                json result = json::parse(chunk.memory);

                if (result[std::string(OBFUSCATE("status"))] == true) {
                    auto& data = result[std::string(OBFUSCATE("data"))];

                    g_license.username = data.value("username", "");
                    g_license.seller   = data.value("seller",   "");
                    g_license.created  = data.value("createdAt", 0LL) / 1000;
                    g_license.expiry   = data.value("expiry",    0LL) / 1000;
                    g_license.valid    = true;

                    std::string token = data[std::string(OBFUSCATE("token"))].get<std::string>();
                    time_t rng = data[std::string(OBFUSCATE("rng"))].get<time_t>();

                    if (rng + 30 > time(0)) {
                        std::string auth = OBFUSCATE("PUBG");
                        auth += std::string(OBFUSCATE("-"));
                        auth += user_key;
                        auth += std::string(OBFUSCATE("-"));
                        auth += UUID;
                        auth += std::string(OBFUSCATE("-"));
                        auth += std::string(OBFUSCATE("Vm8Lk7Uj2JmsjCPVPVjrLa7zgfx3uz9E"));

                        std::string localAuth = Tools::CalcMD5(auth);

                        bValid = (token == localAuth);
                        if (!bValid) {
                            errorMsg = "Token Validation Discrepancy";
                            printf("\033[1;31m[✕] Token Mismatch!\033[0m\n");
                        }
                    } else {
                        errorMsg = "Response Timeout Window Expired";
                        printf("\033[1;33m[⚠] Response Expired!\033[0m\n");
                    }
                } else {
                    errorMsg = result[std::string(OBFUSCATE("reason"))].get<std::string>();
                    printf("\033[1;31m[✕] Server Rejected: \033[1;37m%s\033[0m\n", errorMsg.c_str());
                }
            } catch (const std::exception& e) {
                errorMsg = std::string("JSON Error: ") + e.what();
                printf("\033[1;31m[✕] JSON Parse Error!\033[0m\n");
            }
        } else {
            errorMsg = std::string("Connection Error: ") + curl_easy_strerror(res);
            printf("\033[1;31m[✕] Connection Failed!\033[0m\n");
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        errorMsg = "CURL Engine Initialization Failure";
    }

    free(chunk.memory);
    return bValid;
}

// ============ checkOrSaveKey SYSTEM ============
bool checkOrSaveKey() {
    // Startup check for VPN and Sniffers
    if (isVpnActive() || isSnifferActive()) {
        printf("\033[1;31m[✕] Security Violation: Close VPN/Capture tools before launching.\033[0m\n");
        sleep(3);
        exit(0); 
    }

    std::ifstream inFile(KEY_FILE_PATH);
    if (inFile.good()) {
        std::string savedKey;
        std::getline(inFile, savedKey);
        inFile.close();

        while (!savedKey.empty() && std::isspace(savedKey.back()))
            savedKey.pop_back();

        std::string errorMsg;
        if (verifyKeyOnline(savedKey, errorMsg)) {
            printf("\033[1;32m[✓] Auto-login successful! Key authenticated online.\033[0m\n\n");
            sleep(1);
            return true;
        } else {
            printf("\033[1;31m[✕] Auto-verification failed: %s\033[0m\n", errorMsg.c_str());
            printf("\033[1;33m[⚠] Purging stored credential file...\033[0m\n");
            
            std::ofstream outFile(KEY_FILE_PATH, std::ios::trunc);
            outFile.close();
            sleep(2);
        }
    }

    while (true) {
        printf("\033[2J\033[H");
        printf("\033[1;38;2;162;144;225m╔════════════════════════════════════════╗\n");
        printf("║          Infinity - X Login System                 ║\n");
        printf("╚════════════════════════════════════════╝\033[0m\n\n");
        
        if (isVpnActive() || isSnifferActive()) {
            printf("\033[1;31m[✕] Security Error: Turn off VPN/Sniffer apps to continue.\033[0m\n");
            sleep(2);
            continue;
        }

        printf("\033[1;36mEnter Key: \033[0m");

        std::string inputKey;
        std::getline(std::cin, inputKey);

        while (!inputKey.empty() && std::isspace(inputKey.back()))
            inputKey.pop_back();
        while (!inputKey.empty() && std::isspace(inputKey.front()))
            inputKey.erase(inputKey.begin());

        if (inputKey.empty()) continue;

        std::string errorMsg;
        if (verifyKeyOnline(inputKey, errorMsg)) {
            printf("\033[1;32m[✓] Access Granted! Persisting credential registry...\033[0m\n");

            std::ofstream outFile(KEY_FILE_PATH);
            if (outFile.is_open()) {
                outFile << inputKey << std::endl;
                outFile.close();
                printf("\033[1;32m[✓] Credentials cached natively.\033[0m\n");
            } else {
                printf("\033[1;33m[⚠] Could not write cache registry (storage permissions?)\033[0m\n");
            }

            sleep(1);
            return true;
        }

        printf("\033[1;31m[✕] Auth failed: %s\033[0m\n", errorMsg.c_str());
        printf("\033[1;33mPress Enter to repeat authentication...\033[0m\n");
        std::cin.get();
    }
}


// ============ EXTERNAL / NDK HOOK HANDLERS ============
// External C-linkage function for ImGui / Native Window backend call
extern "C" {
    void toggle_menu_extern() {
        ui::bar::toggle();
    }
}

// Helper for ImGui_ImplAndroid_HandleInputEvent in imgui_impl_android.cpp
// Usage inside imgui_impl_android.cpp:
// if (handle_android_input_event(event_key_code, event_action)) return 1;
bool handle_android_input_event(int32_t key_code, int32_t action) {
    // 24 = AKEYCODE_VOLUME_UP, 0 = AKEY_EVENT_ACTION_DOWN
    if (key_code == 24 && action == 0) {
        toggle_menu_extern();
        return true;
    }
    return false;
}

// ============ ADVANCED DYNAMIC MULTI-THREADED VOLUME LISTENER ============

namespace volume_hook {

    // Thread handler for a single hooked input event node
    static void input_device_thread(int fd, std::string device_path) {
        struct input_event events[16];
        
        while (fd >= 0) {
            ssize_t bytes_read = read(fd, events, sizeof(events));
            if (bytes_read < (ssize_t)sizeof(struct input_event)) {
                if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    usleep(10000); // 10ms sleep if non-blocking wait
                    continue;
                }
                break; // Error or device closed
            }

            size_t event_count = bytes_read / sizeof(struct input_event);
            for (size_t i = 0; i < event_count; i++) {
                // EV_KEY + KEY_VOLUMEUP + Press (Value 1)
                if (events[i].type == EV_KEY && events[i].code == KEY_VOLUMEUP && events[i].value == 1) {
                    ui::bar::toggle();
                }
            }
        }

        if (fd >= 0) {
            close(fd);
        }
    }

} // namespace volume_hook

void volume_listener() {
    const char *dir_path = "/dev/input/";
    DIR *dir = opendir(dir_path);
    
    if (!dir) {
        printf("\033[1;31m[✘] Unable to open /dev/input directory (Requires Root / Permission Denied).\033[0m\n");
        return;
    }

    std::vector<int> active_fds;
    struct dirent *entry;

    // Dynamic scanning of ALL event nodes in /dev/input/
    while ((entry = readdir(dir)) != nullptr) {
        if (strstr(entry->d_name, "event")) {
            char full_path[256];
            snprintf(full_path, sizeof(full_path), "%s%s", dir_path, entry->d_name);

            int fd = open(full_path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            // Step 1: Verify device supports EV_KEY events
            unsigned char evbit[EV_MAX / 8 + 1] = {0};
            if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0 || !(evbit[EV_KEY / 8] & (1 << (EV_KEY % 8)))) {
                close(fd);
                continue;
            }

            // Step 2: Verify device supports KEY_VOLUMEUP
            unsigned char keybit[KEY_MAX / 8 + 1] = {0};
            if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) >= 0) {
                if (keybit[KEY_VOLUMEUP / 8] & (1 << (KEY_VOLUMEUP % 8))) {
                    active_fds.push_back(fd);
                    // Spawn dedicated thread for every matching Volume+ input node
                    std::thread(volume_hook::input_device_thread, fd, std::string(full_path)).detach();
                } else {
                    close(fd);
                }
            } else {
                close(fd);
            }
        }
    }
    closedir(dir);

    if (active_fds.empty()) {
        printf("\033[1;33m[⚠] Native event listener failed to hook input layers (Requires Root or No Volume Node Found).\033[0m\n");
        return;
    }

    printf("\033[1;32m[✓] Dynamic Multi-Threaded Volume+ Hotkey Listener Initialized (%zu nodes hooked)\033[0m\n", active_fds.size());
}
