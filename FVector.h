/*#pragma once

#include <cmath>
#include "SimpleMath/SimpleMath.h"

struct FVector2
{
	float x = 0.0f;
	float y = 0.0f;

	constexpr FVector2() = default;
	constexpr FVector2(float InX, float InY) : x(InX), y(InY) {}

	float LengthSquared() const
	{
		return x * x + y * y;
	}

	float Length() const
	{
		return sqrt(LengthSquared());
	}

	void Normalize()
	{
		float VectorLength = Length();
		if (VectorLength > 0.0f)
		{
			x /= VectorLength;
			y /= VectorLength;
		}
	}

	FVector2 operator+(const FVector2& Other) const
	{
		return FVector2(x + Other.x, y + Other.y);
	}

	FVector2 operator-(const FVector2& Other) const
	{
		return FVector2(x - Other.x, y - Other.y);
	}

	FVector2 operator*(float Scalar) const
	{
		return FVector2(x * Scalar, y * Scalar);
	}

	FVector2& operator/=(float Scalar)
	{
		x /= Scalar;
		y /= Scalar;
		return *this;
	}
};

struct FVector
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	constexpr FVector() = default;

	constexpr FVector(float InX, float InY, float InZ) : x(InX), y(InY), z(InZ) {}

    explicit FVector(const DirectX::XMFLOAT3& value) : x(value.x), y(value.y), z(value.z) {}
    DirectX::SimpleMath::Vector3 ToSimpleMath() const { return {x, y, z}; }

    static FVector Transform(const FVector& position, const FMatrix& matrix);
    static FVector TransformNormal(const FVector& direction, const FMatrix& matrix);
	static const FVector Zero, UnitX, UnitY, UnitZ;
	bool operator==(const FVector&) const = default;
	FVector operator-() const { return FVector(-x, -y, -z); }
	static FVector Min(const FVector& a, const FVector& b)
	{
		return { a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z };
	}
	static FVector Max(const FVector& a, const FVector& b)
	{
		return { a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z };
	}

	FVector operator+(const FVector& rhs) const
	{
		return FVector(x + rhs.x, y + rhs.y, z + rhs.z);
	}

	FVector operator-(const FVector& rhs) const
	{
		return FVector(x - rhs.x, y - rhs.y, z - rhs.z);
	}

	FVector operator*(float scalar) const
	{
		return FVector(x * scalar, y * scalar, z * scalar);
	}

	FVector operator/(float scalar) const
	{
		return FVector(x / scalar, y / scalar, z / scalar);
	}

	float Dot(const FVector& rhs) const {
		return x * rhs.x + y * rhs.y + z * rhs.z;
	}

	FVector Cross(const FVector& rhs) const {
		return FVector(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
	}

	float LengthSquared() const
	{
		return Dot(*this);
	}

	float Length() const
	{
		return std::sqrt(LengthSquared());
	}

	void Normalize()
	{
		const float length = Length();

		if (length <= 1e-6f)
		{
			x = y = z = 0.0f;
			return;
		}

		x /= length;
		y /= length;
		z /= length;
	}
};

inline const FVector FVector::Zero{};
inline const FVector FVector::UnitX{1, 0, 0};
inline const FVector FVector::UnitY{0, 1, 0};
inline const FVector FVector::UnitZ{0, 0, 1};

struct FVector4
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float w = 0.0f;

	constexpr FVector4() = default;
	constexpr FVector4(float InX, float InY, float InZ, float InW) : x(InX), y(InY), z(InZ), w(InW) {}
	constexpr FVector4(FVector InXYZ, float InW) : x(InXYZ.x), y(InXYZ.y), z(InXYZ.z), w(InW) {}

	float Dot(const FVector4& Rhs) const
	{
		return x * Rhs.x + y * Rhs.y + z * Rhs.z+ w * Rhs.w;
	}

	float LengthSquared() const
	{
		return Dot(*this);
	}
	float Length() const
	{
		return std::sqrt(LengthSquared());
	}
	//float Length3Squared() const;
	//float Length3() const;

	FVector4 operator+(const FVector4& Rhs) const
	{
		return FVector4(x + Rhs.x, y + Rhs.y, z + Rhs.z, w + Rhs.w);
	}

	FVector4 operator-(const FVector4& Rhs) const
	{
		return FVector4(x - Rhs.x, y - Rhs.y, z - Rhs.z, w - Rhs.w);
	}

	FVector4 operator*(float Scalar) const
	{
		return FVector4(x * Scalar, y * Scalar, z * Scalar, w * Scalar);
	}

	FVector4& operator+=(const FVector4& Rhs)
	{
		x += Rhs.x;
		y += Rhs.y;
		z += Rhs.z;
		w += Rhs.w;
		return *this;
	}

	FVector4& operator-=(const FVector4& Rhs)
	{
		x -= Rhs.x;
		y -= Rhs.y;
		z -= Rhs.z;
		w -= Rhs.w;
		return *this;
	}

	FVector4& operator*=(float Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		z *= Scalar;
		w *= Scalar;
		return *this;
	}

	FVector4& operator/=(float Scalar)
	{
		x /= Scalar;
		y /= Scalar;
		z /= Scalar;
		w /= Scalar;
		return *this;
	}
};*/