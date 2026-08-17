#pragma once
#include "../other/vector3.h"    // FFM Eric Phillips version
#include "../other/Quaternion.h" // FFM Eric Phillips version
#include "imgui.h"

// ========== D3DMATRIX (For ViewProjection) ==========
struct D3DMatrix {
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
};

// ========== TRANSFORM MATRIX ==========
// NEW (FIXED):
struct Vector4 {
    float x, y, z, w;
};

struct TransformMatrix {
    Vector4 Position;      // ✅ Now defined
    Quaternion Rotation; 
    Vector4 Scale;         // ✅ Now defined
};


// ========== WORLD TO SCREEN ==========
inline Vector3 WorldToScreen(D3DMatrix viewMatrix, Vector3 pos, int screenX, int screenY) {
    Vector3 result = {-1, -1, -1};
    
    float v9 = (pos.x * viewMatrix._11) + (pos.y * viewMatrix._21) + (pos.z * viewMatrix._31) + viewMatrix._41;
    float v10 = (pos.x * viewMatrix._12) + (pos.y * viewMatrix._22) + (pos.z * viewMatrix._32) + viewMatrix._42;
    float v12 = (pos.x * viewMatrix._14) + (pos.y * viewMatrix._24) + (pos.z * viewMatrix._34) + viewMatrix._44;

    if (v12 > 0.001f) {
        float screenCenterX = (float)screenX / 2.0f;
        float screenCenterY = (float)screenY / 2.0f;

        result.x = screenCenterX + (screenCenterX * v9) / v12;
        result.y = screenCenterY - (screenCenterY * v10) / v12;
        result.z = v12;
    }
    return result;
}

// ========== AIMBOT ROTATION ==========
inline Quaternion GetRotationToTheLocation(Vector3 Target, float Height, Vector3 MyEnemy) {
    return Quaternion::LookRotation((Target + Vector3(0, Height, 0)) - MyEnemy, Vector3(0, 1, 0));
}

// ========== FOV CHECK ==========
inline bool isWithinFOV(float screenX, float screenY, float fovRadius, int screenW, int screenH) {
    float centerX = (float)screenW / 2.0f;
    float centerY = (float)screenH / 2.0f;
    float deltaX = screenX - centerX;
    float deltaY = screenY - centerY;
    return sqrt(deltaX*deltaX + deltaY*deltaY) <= fovRadius;
}
