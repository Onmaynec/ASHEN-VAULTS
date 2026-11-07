#pragma once
// Minimal UE-compatible types for standalone testing AV...Navigation.cpp.
// This shim is ONLY used by Tests/Standalone; never include it in a UE target.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
using int32 = std::int32_t;
using uint8 = std::uint8_t;
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
    static int32 Min(int32 A, int32 B) { return std::min(A, B); }
    static int32 Max(int32 A, int32 B) { return std::max(A, B); }
};
