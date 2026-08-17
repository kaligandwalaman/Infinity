#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include <cstdio>

// ========== GLOBALS (from FFM main.cpp) ==========
extern int game_pid;
extern uint64_t lib_base;
extern uint64_t lib_base2;

// ========== STRUCTURES ==========
struct ElfRegionInfo {
    uint64_t start;
    uint64_t end;
    char perms[8];
    bool is_elf_header;
};

// ========== HELPER FUNCTIONS ==========

inline int getProcessID(const char *packageName) {
    int id = -1;
    DIR *dir;
    FILE *fp;
    char filename[64];
    char cmdline[64];
    struct dirent *entry;

    dir = opendir("/proc");
    if (!dir) return -1;

    while ((entry = readdir(dir)) != NULL) {
        id = atoi(entry->d_name);
        if (id != 0) {
            sprintf(filename, "/proc/%d/cmdline", id);
            fp = fopen(filename, "r");
            if (fp) {
                fgets(cmdline, sizeof(cmdline), fp);
                fclose(fp);
                if (strcmp(packageName, cmdline) == 0) {
                    closedir(dir);
                    return id;
                }
            }
        }
    }
    closedir(dir);
    return -1;
}

inline bool is_elf_header(uint64_t addr, int pid) {
    if (addr == 0) return false;
    
    char mem_file[64];
    snprintf(mem_file, sizeof(mem_file), "/proc/%d/mem", pid);
    int fd = open(mem_file, O_RDONLY);
    if (fd < 0) {
       // printf("[-] Failed to open /proc/%d/mem\n", pid);
        return false;
    }

    // Check if address is readable first
    char test_byte;
    ssize_t test_read = pread64(fd, &test_byte, 1, addr);
    if (test_read != 1) {
        close(fd);
        return false;
    }

    char magic[4];
    ssize_t read_size = pread64(fd, magic, sizeof(magic), addr);
    close(fd);

    if (read_size != sizeof(magic)) {
       // printf("[-] Could not read 4 bytes at 0x%lx\n", addr);
        return false;
    }

    bool is_elf = (magic[0] == 0x7F && magic[1] == 'E' && 
                   magic[2] == 'L' && magic[3] == 'F');
    
    if (is_elf) {
       // printf("[ELF] Valid ELF header at 0x%lx\n", addr);
    }
    
    return is_elf;
}


// inline bool is_elf_header(uint64_t addr, int pid) {
    // char mem_file[64];
    // snprintf(mem_file, sizeof(mem_file), "/proc/%d/mem", pid);
    // int fd = open(mem_file, O_RDONLY);
    // if (fd < 0) return false;

    // char magic[4];
    // ssize_t read_size = pread64(fd, magic, sizeof(magic), addr);
    // close(fd);

    // if (read_size != sizeof(magic)) return false;
    // return magic[0] == 0x7F && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F';
// }

inline void get_elf_regions(const char* module_name, int pid, std::vector<ElfRegionInfo>& regions) {
    char path[64], line[1024];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *maps = fopen(path, "rt");
    if (!maps) return;

    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, module_name)) {
            uint64_t start = 0, end = 0;
            char perms[8] = {0};
            sscanf(line, "%lx-%lx %4s", &start, &end, perms);

            ElfRegionInfo r = {start, end, "", false};
            strncpy(r.perms, perms, sizeof(perms)-1);
            r.perms[sizeof(perms)-1] = 0;
            r.is_elf_header = is_elf_header(start, pid);
            regions.push_back(r);
        }
    }
    fclose(maps);
}

inline uint64_t get_module_base(const char* module_name, const char* perms) {
    uint64_t returned = 0;
    char path[64], line[1024];
    snprintf(path, sizeof(path), "/proc/%d/maps", game_pid);
    FILE* maps = fopen(path, "rt");
    if (!maps) return 0;

    // Step 1: Collect ALL matching regions with proper parsing
    struct RegionInfo {
        uint64_t start;
        uint64_t end;
        uint64_t size;
        char perms[8];
        char pathname[256];
        bool is_elf;
    };
    std::vector<RegionInfo> all_regions;

    while (fgets(line, sizeof(line), maps)) {
        // Parse: start-end perms offset dev inode pathname
        uint64_t start = 0, end = 0, offset = 0;
        char perms_str[8] = {0};
        char dev[16] = {0};
        char pathname_str[256] = {0};
        int inode = 0;
        
        // Format: 7a16a34000-7a1d9b1000 r-xp 00000000 00:00 0 /data/app/.../libil2cpp.so
        int parsed = sscanf(line, "%lx-%lx %4s %lx %s %d %255[^\n]",
                           &start, &end, perms_str, &offset, dev, &inode, pathname_str);
        
        // Check if this line contains our module
        if (strstr(line, module_name)) {
            RegionInfo r;
            r.start = start;
            r.end = end;
            r.size = end - start;
            strncpy(r.perms, perms_str, 7);
            r.perms[7] = '\0';
            strncpy(r.pathname, pathname_str, 255);
            r.pathname[255] = '\0';
            r.is_elf = is_elf_header(start, game_pid);
            
            // printf("[MAPS] 0x%lx-0x%lx %s | offset=0x%lx | ELF=%d | %s\n",
                   // start, end, r.perms, offset, r.is_elf, 
                   // pathname_str[0] ? pathname_str : "[anon]");
            
            all_regions.push_back(r);
        }
    }
    fclose(maps);

    // Step 2: Find the REAL base - ELF header with r-xp following it
    // Real libil2cpp.so structure: [r--p ELF header] -> [r-xp code] -> [r--p data]
    
    for (size_t i = 0; i < all_regions.size(); i++) {
        // Look for r-xp region that follows an ELF header region
        if (strcmp(all_regions[i].perms, "r-xp") == 0 && all_regions[i].size > 0x100000) {
            // Check if previous region was ELF header
            if (i > 0 && all_regions[i-1].is_elf && 
                strcmp(all_regions[i-1].perms, "r--p") == 0) {
                // The ELF header region is the REAL base
                returned = all_regions[i-1].start;
                // printf("[+] FOUND REAL BASE (ELF+Code pair): 0x%lx\n", returned);
                return returned;
            }
        }
    }

    // Fallback 1: Any region with ELF header
    for (const auto& r : all_regions) {
        if (r.is_elf && strcmp(r.perms, "r--p") == 0) {
            returned = r.start;
            // printf("[!] Fallback to ELF header: 0x%lx\n", returned);
            return returned;
        }
    }

    // Fallback 2: Largest r-xp region
    uint64_t largest_size = 0;
    for (const auto& r : all_regions) {
        if (strcmp(r.perms, "r-xp") == 0 && r.size > largest_size) {
            largest_size = r.size;
            returned = r.start;
        }
    }
    if (returned) {
        // printf("[!] Fallback to largest r-xp: 0x%lx\n", returned);
    }

    return returned;
}

// inline uint64_t get_module_base(const char* module_name, const char* perms) {
    // uint64_t returned = 0;
    // char path[64], line[1024];
    // snprintf(path, sizeof(path), "/proc/%d/maps", game_pid);
    // FILE* maps = fopen(path, "rt");
    // if (!maps) return 0;

    // while (fgets(line, sizeof(line), maps)) {
        // if (strstr(line, module_name) && strstr(line, perms)) {
            // uint64_t start = 0, end = 0;
            // sscanf(line, "%lx-%lx", &start, &end);
            // if ((end - start) < 0x5100000) {
                // returned = start;
                // break;
            // }
        // }
    // }
    // fclose(maps);
    // return returned;
// }

// ========== MEMORY READ/WRITE ==========

inline bool write_bytes_to_memory(int pid, uint64_t address, const uint8_t* bytes, size_t len) {
    if (pid <= 0 || !bytes || len == 0) return false;

    char mem_file[64];
    snprintf(mem_file, sizeof(mem_file), "/proc/%d/mem", pid);
    int fd = open(mem_file, O_RDWR);
    if (fd < 0) return false;

    ssize_t written = pwrite64(fd, bytes, len, address);
    close(fd);

    return (written == (ssize_t)len);
}

inline void read_game_bytes(uint64_t address, int pid, uint8_t* buf, size_t len) {
    char mem_file[64];
    snprintf(mem_file, sizeof(mem_file), "/proc/%d/mem", pid);
    int fd = open(mem_file, O_RDONLY);
    if (fd < 0) {
        memset(buf, 0, len);
        return;
    }
    pread64(fd, buf, len, address);
    close(fd);
}

inline int read_game_int(uint64_t address, int pid) {
    int value = 0;
    char mem_file[64];
    snprintf(mem_file, sizeof(mem_file), "/proc/%d/mem", pid);
    int fd = open(mem_file, O_RDONLY);
    if (fd < 0) return 0;
    pread64(fd, &value, sizeof(int), address);
    close(fd);
    return value;
}

inline float read_game_float(uint64_t address, int pid) {
    float value = 0.0f;
    char mem_file[64];
    snprintf(mem_file, sizeof(mem_file), "/proc/%d/mem", pid);
    int fd = open(mem_file, O_RDONLY);
    if (fd < 0) return 0.0f;
    pread64(fd, &value, sizeof(float), address);
    close(fd);
    return value;
}

inline uintptr_t read_game_ptr(uint64_t address, int pid) {
    uintptr_t ptr = 0;
    char mem_file[64];
    snprintf(mem_file, sizeof(mem_file), "/proc/%d/mem", pid);
    int fd = open(mem_file, O_RDONLY);
    if (fd < 0) return 0;
    pread64(fd, &ptr, sizeof(uintptr_t), address);
    close(fd);
    return ptr;
}

inline uint64_t read_game_qword(uint64_t address, int pid) {
    uint64_t value = 0;
    char mem_file[64];
    snprintf(mem_file, sizeof(mem_file), "/proc/%d/mem", pid);
    int fd = open(mem_file, O_RDONLY);
    if (fd < 0) return 0;
    pread64(fd, &value, sizeof(uint64_t), address);
    close(fd);
    return value;
}

// ========== TEMPLATE READ/WRITE ==========

template<typename T>
inline T Read(uint64_t address) {
    T buffer{};
    if (game_pid <= 0 || address == 0) {
        return buffer;
    }
    
    char mem_file[64];
    snprintf(mem_file, sizeof(mem_file), "/proc/%d/mem", game_pid);
    int fd = open(mem_file, O_RDONLY);
    if (fd < 0) {
        return buffer;
    }
    
    pread64(fd, &buffer, sizeof(T), address);
    close(fd);
    return buffer;
}

template<typename T>
inline bool Write(uint64_t address, T value) {
    if (game_pid <= 0 || address == 0) {
        return false;
    }
    return write_bytes_to_memory(game_pid, address, (uint8_t*)&value, sizeof(T));
}

/*
// ============================================
// STEP 1-2 AUTO DETECTION (Read ke baad rakho)
// ============================================

inline bool is_real_elf_quick(uint64_t base) {
    if (base == 0) return false;
    
    auto game_facade = Read<uintptr_t>(base + 0xA984D48);
    
    // Fake: 0x2001bfe5 (32-bit range mein)
    // Real: 0x78fb5f5e80 (64-bit heap range mein)
    if (game_facade < 0x5000000000 || game_facade > 0xFFFFFFFFFF) {
        return false;
    }
    
    auto static_facade = Read<uintptr_t>(game_facade + 0xB8);
    
    return (static_facade != 0);
}

// ============================================
// REPLACE OLD get_module_base WITH THIS
// ============================================

inline uint64_t get_module_base(const char* module_name, const char* perms) {
    (void)perms; // Ignore perms parameter
    
    uint64_t returned = 0;
    char path[64], line[1024];
    snprintf(path, sizeof(path), "/proc/%d/maps", game_pid);
    FILE* maps = fopen(path, "rt");
    if (!maps) return 0;

    while (fgets(line, sizeof(line), maps)) {
        // Sirf libil2cpp.so + r--p (Cd ELF header)
        if (!strstr(line, module_name)) continue;
        if (!strstr(line, "r--p")) continue;

        uint64_t start = 0, end = 0;
        sscanf(line, "%lx-%lx", &start, &end);
        
        if (!is_elf_header(start, game_pid)) continue;

        // 🔥 Sirf Step 1-2 check, full chain nahi!
        if (is_real_elf_quick(start)) {
            returned = start;
            break; // Mil gaya!
        }        
    }
    
    fclose(maps);
    return returned;
}
*/
// ========== PATTERN SCANNING ==========

inline bool pattern_compare(const uint8_t* data, const uint8_t* pattern, const uint8_t* mask, size_t length) {
    if (!data || !pattern || !mask || length == 0) return false;
    for (size_t i = 0; i < length; i++) {
        if (mask[i] == 0x00) continue;
        if (data[i] != pattern[i]) return false;
    }
    return true;
}

inline uint64_t scan_pattern_in_region(int pid, uint64_t start_addr, uint64_t end_addr,
    const uint8_t* pattern, const uint8_t* mask, size_t pattern_len) {
    
    if (pid <= 0 || start_addr >= end_addr || !pattern || !mask || pattern_len == 0) return 0;

    char mem_file[64];
    snprintf(mem_file, sizeof(mem_file), "/proc/%d/mem", pid);
    int fd = open(mem_file, O_RDONLY);
    if (fd < 0) return 0;

    const size_t CHUNK_SIZE = 0x10000;
    uint8_t* buffer = new uint8_t[CHUNK_SIZE + pattern_len];
    if (!buffer) { close(fd); return 0; }

    uint64_t current_addr = start_addr;
    uint64_t found_addr = 0;

    while (current_addr < end_addr && found_addr == 0) {
        size_t read_size = std::min((size_t)(end_addr - current_addr), CHUNK_SIZE);
        if (read_size == 0) break;

        ssize_t bytes_read = pread64(fd, buffer, read_size, current_addr);
        if (bytes_read <= 0 || (size_t)bytes_read < pattern_len) {
            current_addr += CHUNK_SIZE;
            continue;
        }

        size_t scan_limit = (size_t)bytes_read - pattern_len;
        for (size_t i = 0; i <= scan_limit; i++) {
            if (pattern_compare(buffer + i, pattern, mask, pattern_len)) {
                found_addr = current_addr + i;
                break;
            }
        }
        
        if ((size_t)bytes_read > pattern_len) {
            current_addr += bytes_read - pattern_len + 1;
        } else {
            current_addr += bytes_read;
        }
    }

    delete[] buffer;
    close(fd);
    return found_addr;
}

inline std::vector<uint64_t> scan_pattern_in_module(const char* module_name, int pid,
    const uint8_t* pattern, const uint8_t* mask, size_t pattern_len, int max_results) {
    
    std::vector<uint64_t> results;
    if (!module_name || pid <= 0 || !pattern || !mask || pattern_len == 0) return results;

    std::vector<ElfRegionInfo> regions;
    get_elf_regions(module_name, pid, regions);
    if (regions.empty()) return results;

    for (const auto& region : regions) {
        if (strcmp(region.perms, "r-xp") == 0) {
            uint64_t addr = scan_pattern_in_region(pid, region.start, region.end, pattern, mask, pattern_len);
            if (addr != 0) {
                results.push_back(addr);
                if (max_results > 0 && results.size() >= (size_t)max_results) break;
            }
        }
    }
    return results;
}

// ========== HEX UTILITIES ==========

inline std::vector<uint8_t> hex_string_to_bytes(const std::string& hex_str) {
    std::vector<uint8_t> bytes;
    if (hex_str.empty()) return bytes;

    std::stringstream ss(hex_str);
    std::string token;

    while (ss >> token) {
        if (token == "??" || token == "?") {
            bytes.push_back(0x00);
            continue;
        }
        if (token.length() == 2) {
            bool is_hex = true;
            for (char c : token) {
                if (!isxdigit(c)) { is_hex = false; break; }
            }
            if (is_hex) {
                bytes.push_back((uint8_t)strtol(token.c_str(), nullptr, 16));
            }
        }
    }
    return bytes;
}

inline std::vector<uint8_t> hex_string_to_mask(const std::string& hex_str) {
    std::vector<uint8_t> mask;
    if (hex_str.empty()) return mask;

    std::stringstream ss(hex_str);
    std::string token;

    while (ss >> token) {
        if (token == "??" || token == "?") {
            mask.push_back(0x00);
        } else if (token.length() == 2) {
            bool is_hex = true;
            for (char c : token) {
                if (!isxdigit(c)) { is_hex = false; break; }
            }
            if (is_hex) mask.push_back(0xFF);
        }
    }
    return mask;
}

// ========== UI HELPERS ==========

inline void show_regions_in_imgui(std::vector<ElfRegionInfo>& regions, uint64_t* plibBase);
inline bool PatchOffsetWithHex(uint64_t libbase, uint64_t offset, const char* replace_hex);
inline std::string ReadHexFromOffset(uint64_t offset, int byte_count);
