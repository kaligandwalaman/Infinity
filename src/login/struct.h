// login/struct.h
#pragma once

#include <string>
#include <ctime>

// Function declarations
bool checkOrSaveKey();

void volume_listener();

struct LicenseInfo {
    std::string username = "";
    std::string seller   = "";
    time_t      created  = 0;     // seconds since epoch
    time_t      expiry   = 0;     // seconds since epoch
    bool        valid    = false;
};

// Global instance (sab jagah access kar sakte hain)
extern LicenseInfo g_license;
// Agar future mein aur functions add karne hain to yahan declare kar dena