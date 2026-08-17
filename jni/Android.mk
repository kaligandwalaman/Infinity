# Android.mk
LOCAL_PATH := $(call my-dir)

# =====================================================
# PREBUILT: libcurl 
# =====================================================
include $(CLEAR_VARS)
LOCAL_MODULE := curl_static

# libcurl.a ko jni/ folder mein rakho (sabse easy)
LOCAL_SRC_FILES := libcurl.a

include $(PREBUILT_STATIC_LIBRARY)

# =====================================================
# EXECUTABLE: infinity.sh
# =====================================================
include $(CLEAR_VARS)
LOCAL_MODULE := infinity.sh

LOCAL_SRC_FILES := \
    ../src/main.cpp \
    ../src/ui/menu.cpp \
    ../src/ui/bar.cpp \
    ../src/ui/widgets/widgets.cpp \
    ../src/func/visuals.cpp \
    ../src/login/login.cpp \
    ../src/protect/oxorany.cpp \
    ../includes/tools/Tools.cpp \
    ../includes/draw/Android_draw/draw.cpp \
    ../includes/draw/Android_touch/Touch.cpp \
    ../includes/draw/ImGui/imgui.cpp \
    ../includes/draw/ImGui/imgui_draw.cpp \
    ../includes/draw/ImGui/imgui_tables.cpp \
    ../includes/draw/ImGui/imgui_widgets.cpp \
    ../includes/draw/ImGui/backends/imgui_impl_android.cpp \
    ../includes/draw/ImGui/backends/imgui_impl_opengl3.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../includes \
    $(LOCAL_PATH)/../includes/tools \
    $(LOCAL_PATH)/../includes/curl \
    $(LOCAL_PATH)/../includes/openssl \
    $(LOCAL_PATH)/../includes/fonts \
    $(LOCAL_PATH)/../includes/internal \
    $(LOCAL_PATH)/../includes/internal/ImGui \
    $(LOCAL_PATH)/../includes/internal/ImGui/backends \
    $(LOCAL_PATH)/../includes/draw/ImGui \
    $(LOCAL_PATH)/../includes/draw/ImGui/backends \
    $(LOCAL_PATH)/../src \
    $(LOCAL_PATH)/../src/ui

LOCAL_STATIC_LIBRARIES := curl_static

LOCAL_LDLIBS := \
    -llog \
    -landroid \
    -lEGL \
    -lGLESv3 \
    -lz \
    -ldl

LOCAL_CPPFLAGS := \
    -std=c++17 \
    -fexceptions \
    -frtti \
    -fvisibility=hidden \
    -fvisibility-inlines-hidden \
    -Oz \
    -ffunction-sections \
    -fdata-sections \
    -fomit-frame-pointer \
    -Wno-error=format-security

LOCAL_LDFLAGS := \
    -Wl,--gc-sections \
    -Wl,--strip-all \
    -Wl,--build-id=none \
    -Wl,--no-undefined \
    -pie \
    -Wl,-z,relro \
    -Wl,-z,now \
    -Wl,-z,noexecstack

include $(BUILD_EXECUTABLE)
