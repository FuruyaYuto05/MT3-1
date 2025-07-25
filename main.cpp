#include <Novice.h>
#include <imgui.h>
#include <math.h>
#include <assert.h>
#include <string.h>

const char kWindowTitle[] = "腕の階層構造";

#define _USE_MATH_DEFINES

// -------- Vector3とMatrix4x4の定義 --------
struct Vector3 {
    float x, y, z;
};

struct Matrix4x4 {
    float m[4][4];
};

// ----------- 行列作成関数 ------------
Matrix4x4 MakeIdentityMatrix() {
    Matrix4x4 result{};
    for (int i = 0; i < 4; i++) result.m[i][i] = 1.0f;
    return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& t) {
    Matrix4x4 result = MakeIdentityMatrix();
    result.m[3][0] = t.x;
    result.m[3][1] = t.y;
    result.m[3][2] = t.z;
    return result;
}

Matrix4x4 MakeScaleMatrix(const Vector3& s) {
    Matrix4x4 result{};
    result.m[0][0] = s.x;
    result.m[1][1] = s.y;
    result.m[2][2] = s.z;
    result.m[3][3] = 1.0f;
    return result;
}

Matrix4x4 MakeRotateXMatrix(float radian) {
    Matrix4x4 result = MakeIdentityMatrix();
    result.m[1][1] = cosf(radian);
    result.m[1][2] = sinf(radian);
    result.m[2][1] = -sinf(radian);
    result.m[2][2] = cosf(radian);
    return result;
}

Matrix4x4 MakeRotateYMatrix(float radian) {
    Matrix4x4 result = MakeIdentityMatrix();
    result.m[0][0] = cosf(radian);
    result.m[0][2] = -sinf(radian);
    result.m[2][0] = sinf(radian);
    result.m[2][2] = cosf(radian);
    return result;
}

Matrix4x4 MakeRotateZMatrix(float radian) {
    Matrix4x4 result = MakeIdentityMatrix();
    result.m[0][0] = cosf(radian);
    result.m[0][1] = sinf(radian);
    result.m[1][0] = -sinf(radian);
    result.m[1][1] = cosf(radian);
    return result;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
    Matrix4x4 result{};
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            for (int k = 0; k < 4; ++k)
                result.m[y][x] += m1.m[y][k] * m2.m[k][x];
    return result;
}

Vector3 TransformPoint(const Vector3& v, const Matrix4x4& m) {
    Vector3 result{};
    result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    return result;
}

// -------- Transform構造体 -----------
struct Transform {
    Vector3 translate;
    Vector3 rotate;
    Vector3 scale;
};

Matrix4x4 MakeTransformMatrix(const Transform& t) {
    Matrix4x4 scale = MakeScaleMatrix(t.scale);
    Matrix4x4 rotateX = MakeRotateXMatrix(t.rotate.x);
    Matrix4x4 rotateY = MakeRotateYMatrix(t.rotate.y);
    Matrix4x4 rotateZ = MakeRotateZMatrix(t.rotate.z);
    Matrix4x4 rotate = Multiply(Multiply(rotateZ, rotateY), rotateX);
    Matrix4x4 translate = MakeTranslateMatrix(t.translate);

    return Multiply(Multiply(scale, rotate), translate);
}

// --------- 球の描画補助関数 ----------
void DrawSphere(const Vector3& pos, float radius, int color) {
    Novice::DrawEllipse(
        static_cast<int>(pos.x * 100 + 640), // 100倍スケーリング
        static_cast<int>(-pos.y * 100 + 360),
        static_cast<int>(radius * 100),
        static_cast<int>(radius * 100),
        0.0f,
        color,
        kFillModeSolid
    );
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, 1280, 720);

    char keys[256] = {};
    char preKeys[256] = {};

    // 初期Transform
    Transform shoulder = { {0.2f, 1.0f, 0.0f}, {0.0f, 0.0f, -6.8f}, {1.0f, 1.0f, 1.0f} };
    Transform elbow = { {0.4f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.4f}, {1.0f, 1.0f, 1.0f} };
    Transform hand = { {0.3f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},  {1.0f, 1.0f, 1.0f} };

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        // ImGui UI
        ImGui::Begin("Arm Controller");
        ImGui::Text("Shoulder");
        ImGui::DragFloat3("Shoulder Pos", &shoulder.translate.x, 0.01f);
        ImGui::DragFloat3("Shoulder Rot", &shoulder.rotate.x, 0.01f);
        ImGui::Text("Elbow");
        ImGui::DragFloat3("Elbow Pos", &elbow.translate.x, 0.01f);
        ImGui::DragFloat3("Elbow Rot", &elbow.rotate.x, 0.01f);
        ImGui::Text("Hand");
        ImGui::DragFloat3("Hand Pos", &hand.translate.x, 0.01f);
        ImGui::DragFloat3("Hand Rot", &hand.rotate.x, 0.01f);
        ImGui::End();

        // ワールド行列
        Matrix4x4 shoulderMat = MakeTransformMatrix(shoulder);
        Matrix4x4 elbowMat = Multiply(MakeTransformMatrix(elbow), shoulderMat);
        Matrix4x4 handMat = Multiply(MakeTransformMatrix(hand), elbowMat);

        // 各関節の世界座標
        Vector3 shoulderWorld = TransformPoint({ 0, 0, 0 }, shoulderMat);
        Vector3 elbowWorld = TransformPoint({ 0, 0, 0 }, elbowMat);
        Vector3 handWorld = TransformPoint({ 0, 0, 0 }, handMat);

        // 球の描画
        DrawSphere(shoulderWorld, 0.05f, 0xFF0000FF); // 赤
        DrawSphere(elbowWorld, 0.05f, 0x00FF00FF); // 緑
        DrawSphere(handWorld, 0.05f, 0x0000FFFF); // 青

        // 線の描画
        Novice::DrawLine(
            static_cast<int>(shoulderWorld.x * 100 + 640),
            static_cast<int>(-shoulderWorld.y * 100 + 360),
            static_cast<int>(elbowWorld.x * 100 + 640),
            static_cast<int>(-elbowWorld.y * 100 + 360),
            0xFFFFFFFF);
        Novice::DrawLine(
            static_cast<int>(elbowWorld.x * 100 + 640),
            static_cast<int>(-elbowWorld.y * 100 + 360),
            static_cast<int>(handWorld.x * 100 + 640),
            static_cast<int>(-handWorld.y * 100 + 360),
            0xFFFFFFFF);

        Novice::EndFrame();
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) break;
    }

    Novice::Finalize();
    return 0;
}
