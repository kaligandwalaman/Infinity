//
// Created by GHr_Ryuuka on 05/12/2021.
//
#include <sys/system_properties.h>   // __system_property_get ke liye
#include <cstring>                   // strlen, strcpy etc.
#include <android/log.h>
#include <libgen.h>
#include <fcntl.h>
#include <inttypes.h>
#include <jni.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <sys/mman.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include "../includes/obfuscate.h"
#include "Tools.h"

#define INRANGE(x, a, b)        (x >= a && x <= b)
#define getBits(x)              (INRANGE(x,'0','9') ? (x - '0') : ((x&(~0x20)) - 'A' + 0xa))
#define getByte(x)              (getBits(x[0]) << 4 | getBits(x[1]))

#if defined(__arm__)
#define process_vm_readv_syscall 376
#define process_vm_writev_syscall 377
#elif defined(__aarch64__)
#define process_vm_readv_syscall 270
#define process_vm_writev_syscall 271
#elif defined(__i386__)
#define process_vm_readv_syscall 347
#define process_vm_writev_syscall 348
#else
#define process_vm_readv_syscall 310
#define process_vm_writev_syscall 311
#endif

ssize_t process_v(pid_t __pid, const struct iovec *__local_iov, unsigned long __local_iov_count, const struct iovec *__remote_iov, unsigned long __remote_iov_count, unsigned long __flags, bool iswrite) {
    return syscall((iswrite ? process_vm_writev_syscall : process_vm_readv_syscall), __pid, __local_iov, __local_iov_count, __remote_iov, __remote_iov_count, __flags);
}

bool Tools::Read(void *addr, void *buffer, size_t length) {
	return memcpy(buffer, addr, length) != 0;
}

bool Tools::Write(void *addr, void *buffer, size_t length) {
	return memcpy(addr, buffer, length) != 0;
}

bool Tools::ReadAddr(void *addr, void *buffer, size_t length) {
	unsigned long page_size = sysconf(_SC_PAGESIZE);
	unsigned long size = page_size * sizeof(uintptr_t);
	return mprotect((void *) ((uintptr_t) addr - ((uintptr_t) addr % page_size) - page_size), (size_t) size, PROT_EXEC | PROT_READ | PROT_WRITE) == 0 && memcpy(buffer, addr, length) != 0;
}

bool Tools::WriteAddr(void *addr, void *buffer, size_t length) {
	unsigned long page_size = sysconf(_SC_PAGESIZE);
    unsigned long size = page_size * sizeof(uintptr_t);
    return mprotect((void *) ((uintptr_t) addr - ((uintptr_t) addr % page_size) - page_size), (size_t) size, PROT_EXEC | PROT_READ | PROT_WRITE) == 0 && memcpy(addr, buffer, length) != 0;
}

bool Tools::SetWriteable(void *addr) {
    long page_size = sysconf(_SC_PAGESIZE);
    long protect_size = page_size * sizeof(uintptr_t);
    void *p = (void *) ((uintptr_t) addr - ((uintptr_t) addr % page_size) - page_size);
    return mprotect(p, (size_t) protect_size, PROT_EXEC | PROT_READ | PROT_WRITE) == 0;
}

uintptr_t Tools::GetBaseAddress(const char *name) {
    uintptr_t base = 0;
    char line[512];

    FILE *f = fopen(OBFUSCATE("/proc/self/maps"), OBFUSCATE("r"));

    if (!f) {
        return 0;
    }
    while (fgets(line, sizeof line, f)) {
        uintptr_t tmpBase;
        char tmpName[256];
        if (sscanf(line, "%" PRIXPTR "-%*" PRIXPTR "%*s %*s %*s %*s %s", &tmpBase, tmpName) > 0) {
            if (!strcmp(basename(tmpName), name)) {
                base = tmpBase;
                break;
            }
        }
    }
    fclose(f);
    return base;
}

uintptr_t Tools::GetEndAddress(const char *name) {
    uintptr_t end = 0;
    char line[512];

    FILE *f = fopen(OBFUSCATE("/proc/self/maps"), OBFUSCATE("r"));

    if (!f) {
        return 0;
    }

    bool found = false;
    while (fgets(line, sizeof line, f)) {
        uintptr_t tmpEnd;
        char tmpName[256];
        if (sscanf(line, "%*" PRIXPTR "-%" PRIXPTR "%*s %*s %*s %*s %s", &tmpEnd, tmpName) > 0) {
            if (!strcmp(basename(tmpName), name)) {
                if (!found) {
                    found = true;
                }
            } else {
                if (found) {
                    end = tmpEnd;
                    break;
                }
            }
        }
    }

    fclose(f);
    return end;
}

uintptr_t Tools::FindPattern(const char *lib, const char *pattern) {
    auto start = GetBaseAddress(lib);
    if (!start)
        return 0;
    auto end = GetEndAddress(lib);
    if (!end)
        return 0;
    auto curPat = reinterpret_cast<const unsigned char *>(pattern);
    uintptr_t firstMatch = 0;
    for (uintptr_t pCur = start; pCur < end; ++pCur) {
        if (*(uint8_t *) curPat == (uint8_t) '\?' || *(uint8_t *) pCur == getByte(curPat)) {
            if (!firstMatch) {
                firstMatch = pCur;
            }
            curPat += (*(uint16_t *) curPat == (uint16_t) '\?\?' || *(uint8_t *) curPat != (uint8_t) '\?') ? 2 : 1;
            if (!*curPat) {
                return firstMatch;
            }
            curPat++;
            if (!*curPat) {
                return firstMatch;
            }
        } else if (firstMatch) {
            pCur = firstMatch;
            curPat = reinterpret_cast<const unsigned char *>(pattern);
            firstMatch = 0;
        }
    }
    return 0;
}

uintptr_t Tools::GetRealOffsets(const char *libraryName, uintptr_t relativeAddr) {
	uintptr_t libBase = Tools::GetBaseAddress(libraryName);
	if (libBase == 0)
		return 0;
	return (reinterpret_cast<uintptr_t>(libBase + relativeAddr));
}

uintptr_t Tools::String2Offset(const char *c) {
    int base = 16;
	static_assert(sizeof(uintptr_t) == sizeof(unsigned long) || sizeof(uintptr_t) == sizeof(unsigned long long), "Please add string to handle conversion for this architecture.");
	
    if (sizeof(uintptr_t) == sizeof(unsigned long)) {
        return strtoul(c, nullptr, base);
    }
    return strtoull(c, nullptr, base);
}

// const char *Tools::GetAndroidID(JNIEnv *env, jobject context) {
    // jclass contextClass = env->FindClass(OBFUSCATE("android/content/Context"));
    // jmethodID getContentResolverMethod = env->GetMethodID(contextClass, OBFUSCATE("getContentResolver"), OBFUSCATE("()Landroid/content/ContentResolver;"));
    // jclass settingSecureClass = env->FindClass(OBFUSCATE("android/provider/Settings$Secure"));
    // jmethodID getStringMethod = env->GetStaticMethodID(settingSecureClass, OBFUSCATE("getString"), OBFUSCATE("(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;"));

    // auto obj = env->CallObjectMethod(context, getContentResolverMethod);
    // auto str = (jstring) env->CallStaticObjectMethod(settingSecureClass, getStringMethod, obj, env->NewStringUTF(OBFUSCATE("android_id")));
    // return env->GetStringUTFChars(str, 0);
// }

// const char *Tools::GetDeviceModel(JNIEnv *env) {
	// jclass buildClass = env->FindClass(OBFUSCATE("android/os/Build"));
	// jfieldID modelId = env->GetStaticFieldID(buildClass, OBFUSCATE("MODEL"), OBFUSCATE("Ljava/lang/String;"));
	
	// auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
	// return env->GetStringUTFChars(str, 0);
// }

// const char *Tools::GetDeviceBrand(JNIEnv *env) {
	// jclass buildClass = env->FindClass(OBFUSCATE("android/os/Build"));
	// jfieldID modelId = env->GetStaticFieldID(buildClass, OBFUSCATE("BRAND"), OBFUSCATE("Ljava/lang/String;"));
	
	// auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
	// return env->GetStringUTFChars(str, 0);
// }

// const char *Tools::GetDeviceUniqueIdentifier(JNIEnv *env, const char *uuid) {
    // jclass uuidClass = env->FindClass(OBFUSCATE("java/util/UUID"));

    // auto len = strlen(uuid);

    // jbyteArray myJByteArray = env->NewByteArray(len);
    // env->SetByteArrayRegion(myJByteArray, 0, len, (jbyte *) uuid);

    // jmethodID nameUUIDFromBytesMethod = env->GetStaticMethodID(uuidClass, OBFUSCATE("nameUUIDFromBytes"), OBFUSCATE("([B)Ljava/util/UUID;"));
    // jmethodID toStringMethod = env->GetMethodID(uuidClass, OBFUSCATE("toString"), OBFUSCATE("()Ljava/lang/String;"));

    // auto obj = env->CallStaticObjectMethod(uuidClass, nameUUIDFromBytesMethod, myJByteArray);
    // auto str = (jstring) env->CallObjectMethod(obj, toStringMethod);
    // return env->GetStringUTFChars(str, 0);
// }

// Dummy AndroidID (ab use nahi karenge, lekin declaration match kare)
std::string Tools::GetAndroidID() {
    return "";   // ya "unknown" bhi daal sakte ho
}

// Native model
std::string Tools::GetDeviceModel() {
    char value[PROP_VALUE_MAX + 1] = {0};
    if (__system_property_get("ro.product.model", value) > 0) {
        return std::string(value);
    }
    return "unknown_model";
}

// Native brand
std::string Tools::GetDeviceBrand() {
    char value[PROP_VALUE_MAX + 1] = {0};
    if (__system_property_get("ro.product.brand", value) > 0) {
        return std::string(value);
    }
    return "unknown_brand";
}

// Sabse important: New unique identifier (no JNI)
std::string Tools::GetDeviceUniqueIdentifier(const std::string& input) {
    std::string hwid = input.empty() ? "" : input;  // agar empty aaya toh blank shuru karo

    char value[PROP_VALUE_MAX + 1] = {0};

    if (__system_property_get("ro.serialno", value) > 0) {
        hwid += std::string("|serial=") + value;
    }

    memset(value, 0, sizeof(value));
    if (__system_property_get("ro.build.fingerprint", value) > 0) {
        hwid += std::string("|fingerprint=") + value;
    }

    hwid += "|model=" + Tools::GetDeviceModel();
    hwid += "|brand=" + Tools::GetDeviceBrand();

    return hwid;
}

// Tools.cpp (line \~264 ke aas-paas)
std::string Tools::CalcMD5(const std::string& s) {   // ← yahan const & add kar do
    std::string result;

    unsigned char hash[MD5_DIGEST_LENGTH];
    char tmp[4];

    MD5_CTX md5;
    MD5_Init(&md5);
    MD5_Update(&md5, s.c_str(), s.length());         // .c_str() safe hai const &
    MD5_Final(hash, &md5);

    for (unsigned char i : hash) {
        sprintf(tmp, "%02x", i);
        result += tmp;
    }
    return result;
}


// =============================

