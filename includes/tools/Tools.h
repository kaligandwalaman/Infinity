#pragma once

//
// Created by GHr_Ryuuka on 05/12/2021.
//


#include <map>
// #include <jni.h>

// const char* VALID_SIG = OBFUSCATE("204b0a2c193de268c7fadb5819a718e2c3a7f0f0228befa2e45b5f9286c33b92");

namespace Tools {
	void Hook(void *target, void *replace, void **backup);
	
	bool Read(void *addr, void *buffer, size_t length);
	bool Write(void *addr, void *buffer, size_t length);
	bool ReadAddr(void *addr, void *buffer, size_t length);
	bool WriteAddr(void *addr, void *buffer, size_t length);
	bool SetWriteable(void *addr);
	bool PVM_ReadAddr(void *addr, void *buffer, size_t length);
	bool PVM_WriteAddr(void *addr, void *buffer, size_t length);
	bool IsPtrValid(void *addr);
	
	uintptr_t GetBaseAddress(const char *name);
	uintptr_t GetEndAddress(const char *name);
	uintptr_t FindPattern(const char *lib, const char* pattern);
	uintptr_t GetRealOffsets(const char *libraryName, uintptr_t relativeAddr);
	uintptr_t String2Offset(const char *c);
	
	// const char *GetAndroidID(JNIEnv *env, jobject context);
	// const char *GetDeviceModel(JNIEnv *env);
	// const char *GetDeviceBrand(JNIEnv *env);
	// const char *GetDeviceUniqueIdentifier(JNIEnv *env, const char *uuid);
	    // Yeh teen ab JNI nahi lenge
    std::string GetAndroidID();                        // dummy / empty return karega
    std::string GetDeviceModel();                      // native se
    std::string GetDeviceBrand();                      // native se

    // std::string GetDeviceUniqueIdentifier(const std::string& input = "");  // env hata diya
    std::string GetDeviceUniqueIdentifier(const std::string& input);   // ← no default value

    std::string CalcMD5(const std::string& s);        // already string return karta hai
	//std::string CalcMD5(std::string s);
}
