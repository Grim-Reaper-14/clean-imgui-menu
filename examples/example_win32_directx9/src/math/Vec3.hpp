#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <type_traits>

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3() noexcept = default;

    constexpr Vec3(float xValue, float yValue, float zValue) noexcept
        : x(xValue), y(yValue), z(zValue)
    {
    }

    [[nodiscard]] float* Data() noexcept
    {
        return &x;
    }

    [[nodiscard]] const float* Data() const noexcept
    {
        return &x;
    }

    [[nodiscard]] float& operator[](std::size_t index) noexcept
    {
        assert(index < 3);
        return Data()[index];
    }

    [[nodiscard]] const float& operator[](std::size_t index) const noexcept
    {
        assert(index < 3);
        return Data()[index];
    }

    [[nodiscard]] constexpr Vec3 operator+(
        const Vec3& other) const noexcept
    {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    [[nodiscard]] constexpr Vec3 operator-(
        const Vec3& other) const noexcept
    {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    [[nodiscard]] constexpr Vec3 operator-() const noexcept
    {
        return Vec3(-x, -y, -z);
    }

    [[nodiscard]] constexpr Vec3 operator*(float scalar) const noexcept
    {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }

    [[nodiscard]] Vec3 operator/(float scalar) const noexcept
    {
        assert(scalar != 0.0f);
        const float inverse = 1.0f / scalar;
        return *this * inverse;
    }

    constexpr Vec3& operator+=(const Vec3& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    constexpr Vec3& operator-=(const Vec3& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    constexpr Vec3& operator*=(float scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    Vec3& operator/=(float scalar) noexcept
    {
        assert(scalar != 0.0f);
        const float inverse = 1.0f / scalar;
        return *this *= inverse;
    }

    [[nodiscard]] constexpr bool operator==(
        const Vec3& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }

    [[nodiscard]] constexpr bool operator!=(
        const Vec3& other) const noexcept
    {
        return !(*this == other);
    }

    [[nodiscard]] constexpr float LengthSquared() const noexcept
    {
        return x * x + y * y + z * z;
    }

    [[nodiscard]] float Length() const noexcept
    {
        return std::sqrt(LengthSquared());
    }

    [[nodiscard]] constexpr float Dot(
        const Vec3& other) const noexcept
    {
        return x * other.x + y * other.y + z * other.z;
    }

    [[nodiscard]] constexpr Vec3 Cross(
        const Vec3& other) const noexcept
    {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x);
    }

    [[nodiscard]] constexpr float DistanceSquaredTo(
        const Vec3& other) const noexcept
    {
        return (*this - other).LengthSquared();
    }

    [[nodiscard]] float DistanceTo(const Vec3& other) const noexcept
    {
        return std::sqrt(DistanceSquaredTo(other));
    }

    bool Normalize(float epsilon = 0.000001f) noexcept
    {
        const float lengthSquared = LengthSquared();
        if (lengthSquared <= epsilon * epsilon)
        {
            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
            return false;
        }

        const float inverseLength = 1.0f / std::sqrt(lengthSquared);
        *this *= inverseLength;
        return true;
    }

    [[nodiscard]] Vec3 Normalized(
        float epsilon = 0.000001f) const noexcept
    {
        Vec3 result = *this;
        result.Normalize(epsilon);
        return result;
    }

    [[nodiscard]] bool IsNearlyZero(
        float epsilon = 0.000001f) const noexcept
    {
        return LengthSquared() <= epsilon * epsilon;
    }

    [[nodiscard]] bool NearlyEquals(
        const Vec3& other,
        float epsilon = 0.000001f) const noexcept
    {
        return std::fabs(x - other.x) <= epsilon &&
               std::fabs(y - other.y) <= epsilon &&
               std::fabs(z - other.z) <= epsilon;
    }

    [[nodiscard]] bool IsFinite() const noexcept
    {
        return std::isfinite(x) &&
               std::isfinite(y) &&
               std::isfinite(z);
    }

    [[nodiscard]] static constexpr Vec3 Zero() noexcept
    {
        return Vec3(0.0f, 0.0f, 0.0f);
    }

    [[nodiscard]] static constexpr Vec3 One() noexcept
    {
        return Vec3(1.0f, 1.0f, 1.0f);
    }

    [[nodiscard]] static constexpr Vec3 Up() noexcept
    {
        return Vec3(0.0f, 0.0f, 1.0f);
    }

    [[nodiscard]] static constexpr Vec3 Lerp(
        const Vec3& start,
        const Vec3& end,
        float amount) noexcept
    {
        return start + (end - start) * amount;
    }
};

[[nodiscard]] constexpr Vec3 operator*(
    float scalar,
    const Vec3& vector) noexcept
{
    return vector * scalar;
}

static_assert(
    std::is_standard_layout<Vec3>::value,
    "Vec3 must remain a standard-layout type.");
static_assert(
    sizeof(Vec3) == sizeof(float) * 3,
    "Vec3 must contain exactly three contiguous floats.");
