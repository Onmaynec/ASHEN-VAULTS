#pragma once
// Minimal UE-compatible types for standalone testing AV...Navigation.cpp.
// This shim is ONLY used by Tests/Standalone; never include it in a UE target.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
using int32 = std::int32_t;
using uint8 = std::uint8_t;
using uint32 = std::uint32_t;
struct FColor
{
    uint8 R, G, B, A;
    FColor() : R(0), G(0), B(0), A(0) {}
    FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255)
        : R(InR), G(InG), B(InB), A(InA) {}
};
struct FLinearColor
{
    float R, G, B, A;
    FLinearColor(float InR, float InG, float InB, float InA = 1.f)
        : R(InR), G(InG), B(InB), A(InA) {}
};
struct FVector2D
{
    float X = 0.f, Y = 0.f;
    FVector2D() = default;
    FVector2D(float InX, float InY) : X(InX), Y(InY) {}
    FVector2D operator+(const FVector2D& Other) const { return {X + Other.X, Y + Other.Y}; }
    FVector2D operator-(const FVector2D& Other) const { return {X - Other.X, Y - Other.Y}; }
    FVector2D operator*(float Value) const { return {X * Value, Y * Value}; }
    float Size() const { return std::sqrt(X * X + Y * Y); }
    FVector2D GetSafeNormal() const
    {
        const float Length = Size();
        return Length > 0.00001f ? (*this) * (1.f / Length) : FVector2D{};
    }
    static float Distance(const FVector2D& A, const FVector2D& B) { return (A - B).Size(); }
};
struct FIntPoint
{
    int32 X, Y;
    FIntPoint(int32 InX, int32 InY) : X(InX), Y(InY) {}
};
template<class T> class TArray
{
    std::vector<T> Data;
public:
    void Init(const T& Value, int32 Num) { Data.assign(static_cast<size_t>(Num), Value); }
    void Reserve(int32 Num) { Data.reserve(static_cast<size_t>(Num)); }
    void Add(const T& Value) { Data.push_back(Value); }
    int32 Num() const { return static_cast<int32>(Data.size()); }
    bool IsValidIndex(int32 Index) const { return Index >= 0 && Index < Num(); }
    T& operator[](int32 Index) { return Data.at(static_cast<size_t>(Index)); }
    const T& operator[](int32 Index) const { return Data.at(static_cast<size_t>(Index)); }
    auto begin() -> decltype(Data.begin()) { return Data.begin(); }
    auto end() -> decltype(Data.end()) { return Data.end(); }
};
struct FMath
{
    static int32 FloorToInt(float Value) { return static_cast<int32>(std::floor(Value)); }
    static int32 CeilToInt(float Value) { return static_cast<int32>(std::ceil(Value)); }
    template<typename T> static T Abs(T A) { return std::abs(A); }
    template<typename T> static T Clamp(T V, T Lo, T Hi) { return std::min(std::max(V, Lo), Hi); }
    static int32 Min(int32 A, int32 B) { return std::min(A, B); }
    static int32 Max(int32 A, int32 B) { return std::max(A, B); }
};
