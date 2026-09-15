#pragma once
#include <algorithm>

#include "SimpleMath/SimpleMath.h"

struct FQuat;
struct FMatrix;

using FPlane = DirectX::SimpleMath::Plane;
using FRay = DirectX::SimpleMath::Ray;
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
	DirectX::SimpleMath::Vector3 ToSimpleMath() const { return { x, y, z }; }

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
inline const FVector FVector::UnitX{ 1, 0, 0 };
inline const FVector FVector::UnitY{ 0, 1, 0 };
inline const FVector FVector::UnitZ{ 0, 0, 1 };

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
		return x * Rhs.x + y * Rhs.y + z * Rhs.z + w * Rhs.w;
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
};

using FVector3 = FVector;
using FVector2D = FVector2;
using FColor4 = FVector4;

// Euler angles are an editor-facing representation. Runtime transforms keep
// their authoritative rotation in FQuat.
struct FRotator
{
    float x = 0.0f; // pitch
    float y = 0.0f; // yaw
    float z = 0.0f; // roll

    constexpr FRotator() = default;
    constexpr FRotator(float InPitch, float InYaw, float InRoll) : x(InPitch), y(InYaw), z(InRoll) {}
    constexpr FRotator(const FVector& InEuler) : x(InEuler.x), y(InEuler.y), z(InEuler.z) {}

    constexpr operator FVector() const { return { x, y, z }; }
    constexpr bool operator==(const FRotator&) const = default;
    constexpr bool operator==(const FVector& Other) const { return x == Other.x && y == Other.y && z == Other.z; }

    static const FRotator Zero;
};

inline const FRotator FRotator::Zero{};

struct FMatrix
{
    // Value-returning compatibility API. Singular input produces non-finite values.
    // Use TryInverse when the caller needs to handle failure.
    FMatrix Invert() const
    {
        FMatrix result;
        if (!TryInverse(result))
            for (auto& row : result.m)
                for (float& value : row)
                    value = std::numeric_limits<float>::infinity();
        return result;
    }

    static const FMatrix Identity;
    static FMatrix CreateScale(float x, float y, float z)
    {
        return CreateScale(FVector(x, y, z));
    }
    static FMatrix CreateTranslation(float x, float y, float z)
    {
        return CreateTranslation(FVector(x, y, z));
    }
    float m[4][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };

    FMatrix operator*(const FMatrix& rhs) const
    {
        FMatrix result;

        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                result.m[row][col] = 0.0f;

                for (int k = 0; k < 4; ++k)
                {
                    result.m[row][col] +=
                        m[row][k] * rhs.m[k][col];
                }
            }
        }

        return result;
    }

    static FMatrix CreateScale(const FVector& scale)
    {
        FMatrix result;

        result.m[0][0] = scale.x;
        result.m[1][1] = scale.y;
        result.m[2][2] = scale.z;

        return result;
    }

    static FMatrix CreateTranslation(const FVector& position)
    {
        FMatrix result;

        result.m[3][0] = position.x;
        result.m[3][1] = position.y;
        result.m[3][2] = position.z;

        return result;
    }

    static FMatrix CreateRotationX(float radians)
    {
        FMatrix result;
        const float c = std::cos(radians);
        const float s = std::sin(radians);

        result.m[1][1] = c;
        result.m[1][2] = s;
        result.m[2][1] = -s;
        result.m[2][2] = c;

        return result;
    }

    static FMatrix CreateRotationY(float radians)
    {
        FMatrix result;
        const float c = std::cos(radians);
        const float s = std::sin(radians);

        result.m[0][0] = c;
        result.m[0][2] = -s;
        result.m[2][0] = s;
        result.m[2][2] = c;

        return result;
    }

    static FMatrix CreateRotationZ(float radians)
    {
        FMatrix result;
        const float c = std::cos(radians);
        const float s = std::sin(radians);

        result.m[0][0] = c;
        result.m[0][1] = s;
        result.m[1][0] = -s;
        result.m[1][1] = c;

        return result;
    }

    static FMatrix CreateFromYawPitchRoll(
        float yaw, float pitch, float roll)
    {
        return CreateRotationY(roll)
            * CreateRotationX(pitch)
            * CreateRotationZ(yaw);
    }


    FVector TransformPosition(const FVector& position) const
    {
        return FVector(
            position.x * m[0][0] +
            position.y * m[1][0] +
            position.z * m[2][0] + m[3][0],

            position.x * m[0][1] +
            position.y * m[1][1] +
            position.z * m[2][1] + m[3][1],

            position.x * m[0][2] +
            position.y * m[1][2] +
            position.z * m[2][2] + m[3][2]
        );
    }

    FVector TransformDirection(const FVector& direction) const
    {
        return FVector(
            direction.x * m[0][0] +
            direction.y * m[1][0] +
            direction.z * m[2][0],

            direction.x * m[0][1] +
            direction.y * m[1][1] +
            direction.z * m[2][1],

            direction.x * m[0][2] +
            direction.y * m[1][2] +
            direction.z * m[2][2]
        );
    }

    FVector Translation() const
    {
        return FVector(m[3][0], m[3][1], m[3][2]);
    }

    void Translation(const FVector& position)
    {
        m[3][0] = position.x;
        m[3][1] = position.y;
        m[3][2] = position.z;
    }

    FVector Right() const
    {
        return FVector(m[0][0], m[0][1], m[0][2]);
    }

    FVector Up() const
    {
        return FVector(m[1][0], m[1][1], m[1][2]);
    }

    FVector Forward() const
    {
        return FVector(m[2][0], m[2][1], m[2][2]);
    }

    static FMatrix CreatePerspectiveFieldOfView(
        float fovY,
        float aspectRatio,
        float nearZ,
        float farZ)
    {

        const float yScale = 1.0f / std::tan(fovY * 0.5f);
        const float xScale = yScale / aspectRatio;
        const float depthScale = farZ / (farZ - nearZ);

        FMatrix result;

        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                result.m[row][col] = 0.0f;
            }
        }

        result.m[0][0] = xScale;
        result.m[1][1] = yScale;
        result.m[2][2] = depthScale;
        result.m[2][3] = 1.0f;
        result.m[3][2] = -nearZ * depthScale;

        return result;
    }

    bool TransformCoord(
        const FVector& position,
        FVector& outPosition) const
    {
        const float x =
            position.x * m[0][0] +
            position.y * m[1][0] +
            position.z * m[2][0] + m[3][0];

        const float y =
            position.x * m[0][1] +
            position.y * m[1][1] +
            position.z * m[2][1] + m[3][1];

        const float z =
            position.x * m[0][2] +
            position.y * m[1][2] +
            position.z * m[2][2] + m[3][2];

        const float w =
            position.x * m[0][3] +
            position.y * m[1][3] +
            position.z * m[2][3] + m[3][3];

        if (std::abs(w) <= 1e-6f)
        {
            return false;
        }

        outPosition = FVector(x / w, y / w, z / w);
        return true;
    }

    bool TryInverse(FMatrix& outInverse) const
    {
        double augmented[4][8] = {};

        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                augmented[row][col] = m[row][col];
            }

            augmented[row][row + 4] = 1.0;
        }

        for (int col = 0; col < 4; ++col)
        {
            int pivotRow = col;

            for (int row = col + 1; row < 4; ++row)
            {
                if (std::abs(augmented[row][col]) >
                    std::abs(augmented[pivotRow][col]))
                {
                    pivotRow = row;
                }
            }

            if (std::abs(augmented[pivotRow][col]) <= 1e-12)
            {
                return false;
            }

            if (pivotRow != col)
            {
                for (int k = 0; k < 8; ++k)
                {
                    const double temp = augmented[col][k];
                    augmented[col][k] = augmented[pivotRow][k];
                    augmented[pivotRow][k] = temp;
                }
            }

            const double pivot = augmented[col][col];

            for (int k = 0; k < 8; ++k)
            {
                augmented[col][k] /= pivot;
            }

            for (int row = 0; row < 4; ++row)
            {
                if (row == col)
                {
                    continue;
                }

                const double factor = augmented[row][col];

                for (int k = 0; k < 8; ++k)
                {
                    augmented[row][k] -=
                        factor * augmented[col][k];
                }
            }
        }

        FMatrix result;

        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                result.m[row][col] =
                    static_cast<float>(augmented[row][col + 4]);
            }
        }

        outInverse = result;
        return true;
    }

    static FMatrix CreateOrthographic(
        float width,
        float height,
        float nearZ,
        float farZ)
    {
        FMatrix result;

        result.m[0][0] = 2.0f / width;
        result.m[1][1] = 2.0f / height;
        result.m[2][2] = 1.0f / (farZ - nearZ);
        result.m[3][2] = -nearZ / (farZ - nearZ);

        return result;
    }

    static FMatrix CreateFromQuaternion(const FQuat& rotation);

    DirectX::SimpleMath::Matrix ToSimpleMath() const
    {
        return { m[0][0], m[0][1], m[0][2], m[0][3],
                m[1][0], m[1][1], m[1][2], m[1][3],
                m[2][0], m[2][1], m[2][2], m[2][3],
                m[3][0], m[3][1], m[3][2], m[3][3] };
    }

    FQuat ToQuaternion() const;

    bool Decompose(
        FVector& outScale,
        FQuat& outRotation,
        FVector& outTranslation) const;
};

struct FQuat {
    float x;
    float y;
    float z;
    float w;

    FQuat() : x(0), y(0), z(0), w(1) {}
    FQuat(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    FQuat(const FVector& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
    FQuat(const FVector4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
    explicit FQuat(const DirectX::XMFLOAT4& value) : x(value.x), y(value.y), z(value.z), w(value.w) {}

    DirectX::SimpleMath::Quaternion ToSimpleMath() const { return { x, y, z, w }; }

    FQuat operator*(const FQuat& Other) const
    {
        return FQuat(
            w * Other.x + x * Other.w + y * Other.z - z * Other.y, // X
            w * Other.y - x * Other.z + y * Other.w + z * Other.x, // Y
            w * Other.z + x * Other.y - y * Other.x + z * Other.w, // Z
            w * Other.w - x * Other.x - y * Other.y - z * Other.z  // W
        );
    }

    static FQuat FromRotator(const FRotator& Rotation) {
        const FQuat Roll = CreateFromAxisAngle(FVector::UnitY, Rotation.z);
        const FQuat Pitch = CreateFromAxisAngle(FVector::UnitX, Rotation.x);
        const FQuat Yaw = CreateFromAxisAngle(FVector::UnitZ, Rotation.y);
        return Concatenate(Concatenate(Roll, Pitch), Yaw);
    }

    FRotator ToRotator() const {
        const FMatrix RotationMatrix = FMatrix::CreateFromQuaternion(*this);
        const float Pitch = std::asin(std::clamp(RotationMatrix.m[1][2], -1.0f, 1.0f));
        const float CosPitch = std::cos(Pitch);

        if (std::abs(CosPitch) > 1e-6f) {
            return {
                Pitch,
                std::atan2(-RotationMatrix.m[1][0], RotationMatrix.m[1][1]),
                std::atan2(-RotationMatrix.m[0][2], RotationMatrix.m[2][2])
            };
        }

        return { Pitch, std::atan2(RotationMatrix.m[0][1], RotationMatrix.m[0][0]), 0.0f };
    }

    void Normalize() {
        float d = sqrt(x * x + y * y + z * z + w * w);
        if (d <= 1e-8f) {
            *this = FQuat{};
            return;
        }
        x /= d;
        y /= d;
        z /= d;
        w /= d;
    };

    FQuat Inverse() const {
        DirectX::SimpleMath::Quaternion Result;
        ToSimpleMath().Inverse(Result);
        return FQuat(Result);
    }

    static FQuat Concatenate(const FQuat& First, const FQuat& Second) {
        return First * Second;
    }

    const static FQuat CreateFromRotationMatrix(const FMatrix& M) {
        FQuat result;
        float r22 = M.m[2][2];
        if (r22 <= 0.f)  // x^2 + y^2 >= z^2 + w^2
        {
            float dif10 = M.m[1][1] - M.m[0][0];
            float omr22 = 1.f - r22;
            if (dif10 <= 0.f)  // x^2 >= y^2
            {
                float fourXSqr = omr22 - dif10;
                float inv4x = 0.5f / sqrtf(fourXSqr);
                result.x = fourXSqr * inv4x;
                result.y = (M.m[0][1] + M.m[1][0]) * inv4x;
                result.z = (M.m[0][2] + M.m[2][0]) * inv4x;
                result.w = (M.m[1][2] - M.m[2][1]) * inv4x;
            }
            else  // y^2 >= x^2
            {
                float fourYSqr = omr22 + dif10;
                float inv4y = 0.5f / sqrtf(fourYSqr);
                result.x = (M.m[0][1] + M.m[1][0]) * inv4y;
                result.y = fourYSqr * inv4y;
                result.z = (M.m[1][2] + M.m[2][1]) * inv4y;
                result.w = (M.m[2][0] - M.m[0][2]) * inv4y;
            }
        }
        else  // z^2 + w^2 >= x^2 + y^2
        {
            float sum10 = M.m[1][1] + M.m[0][0];
            float opr22 = 1.f + r22;
            if (sum10 <= 0.f)  // z^2 >= w^2
            {
                float fourZSqr = opr22 - sum10;
                float inv4z = 0.5f / sqrtf(fourZSqr);
                result.x = (M.m[0][2] + M.m[2][0]) * inv4z;
                result.y = (M.m[1][2] + M.m[2][1]) * inv4z;
                result.z = fourZSqr * inv4z;
                result.w = (M.m[0][1] - M.m[1][0]) * inv4z;
            }
            else  // w^2 >= z^2
            {
                float fourWSqr = opr22 + sum10;
                float inv4w = 0.5f / sqrtf(fourWSqr);
                result.x = (M.m[1][2] - M.m[2][1]) * inv4w;
                result.y = (M.m[2][0] - M.m[0][2]) * inv4w;
                result.z = (M.m[0][1] - M.m[1][0]) * inv4w;
                result.w = fourWSqr * inv4w;
            }
        }
        return result;
    }

    const FVector ToEuler() const {
        const float xx = x * x;
        const float yy = y * y;
        const float zz = z * z;

        const float m31 = 2.f * x * z + 2.f * y * w;
        const float m32 = 2.f * y * z - 2.f * x * w;
        const float m33 = 1.f - 2.f * xx - 2.f * yy;

        const float cy = sqrtf(m33 * m33 + m31 * m31);
        const float cx = atan2f(-m32, cy);
        if (cy > 16.f * FLT_EPSILON)
        {
            const float m12 = 2.f * x * y + 2.f * z * w;
            const float m22 = 1.f - 2.f * xx - 2.f * zz;

            return FVector(cx, atan2f(m31, m33), atan2f(m12, m22));
        }
        else
        {
            const float m11 = 1.f - 2.f * yy - 2.f * zz;
            const float m21 = 2.f * x * y - 2.f * z * w;

            return FVector(cx, 0.f, atan2f(-m21, m11));
        }
    }

    const static FQuat CreateFromAxisAngle(const FVector axis, const float angle) {

        FVector result = axis;
        result.Normalize();

        float halfAngle = angle * 0.5f;

        float s = sin(halfAngle);
        float c = cos(halfAngle);

        return FQuat(result.x * s, result.y * s, result.z * s, c);
    }
};

inline const FMatrix FMatrix::Identity{};

inline FVector FVector::Transform(const FVector& position, const FMatrix& matrix)
{
    FVector result;
    if (!matrix.TransformCoord(position, result))
    {
        const float invalid = std::numeric_limits<float>::quiet_NaN();
        return { invalid, invalid, invalid };
    }
    return result;
}

inline FVector FVector::TransformNormal(const FVector& direction, const FMatrix& matrix)
{
    return matrix.TransformDirection(direction);
}

inline FQuat FMatrix::ToQuaternion() const
{
    return FQuat::CreateFromRotationMatrix(*this);
}

inline bool FMatrix::Decompose(
    FVector& outScale,
    FQuat& outRotation,
    FVector& outTranslation) const
{
    constexpr float epsilon = 1e-6f;
    constexpr float orthogonalTolerance = 1e-4f;


    if (std::abs(m[0][3]) > epsilon ||
        std::abs(m[1][3]) > epsilon ||
        std::abs(m[2][3]) > epsilon ||
        std::abs(m[3][3] - 1.0f) > epsilon)
    {
        return false;
    }

    FVector axisX(m[0][0], m[0][1], m[0][2]);
    FVector axisY(m[1][0], m[1][1], m[1][2]);
    FVector axisZ(m[2][0], m[2][1], m[2][2]);

    FVector scale(
        axisX.Length(),
        axisY.Length(),
        axisZ.Length()
    );

    if (scale.x <= epsilon ||
        scale.y <= epsilon ||
        scale.z <= epsilon)
    {
        return false;
    }

    axisX = axisX / scale.x;
    axisY = axisY / scale.y;
    axisZ = axisZ / scale.z;


    if (std::abs(axisX.Dot(axisY)) > orthogonalTolerance ||
        std::abs(axisX.Dot(axisZ)) > orthogonalTolerance ||
        std::abs(axisY.Dot(axisZ)) > orthogonalTolerance)
    {
        return false;
    }


    const float determinant = axisX.Dot(axisY.Cross(axisZ));

    if (determinant < 0.0f)
    {
        scale.x = -scale.x;
        axisX = axisX * -1.0f;
    }

    FMatrix rotationMatrix;

    rotationMatrix.m[0][0] = axisX.x;
    rotationMatrix.m[0][1] = axisX.y;
    rotationMatrix.m[0][2] = axisX.z;

    rotationMatrix.m[1][0] = axisY.x;
    rotationMatrix.m[1][1] = axisY.y;
    rotationMatrix.m[1][2] = axisY.z;

    rotationMatrix.m[2][0] = axisZ.x;
    rotationMatrix.m[2][1] = axisZ.y;
    rotationMatrix.m[2][2] = axisZ.z;

    outScale = scale;
    outRotation = rotationMatrix.ToQuaternion();
    outTranslation = Translation();

    return true;
}

inline FMatrix FMatrix::CreateFromQuaternion(const FQuat& rotation)
{
    FQuat q = rotation;
    q.Normalize();

    const float xx = q.x * q.x;
    const float yy = q.y * q.y;
    const float zz = q.z * q.z;

    const float xy = q.x * q.y;
    const float xz = q.x * q.z;
    const float yz = q.y * q.z;

    const float xw = q.x * q.w;
    const float yw = q.y * q.w;
    const float zw = q.z * q.w;

    FMatrix result;

    result.m[0][0] = 1.0f - 2.0f * (yy + zz);
    result.m[0][1] = 2.0f * (xy + zw);
    result.m[0][2] = 2.0f * (xz - yw);

    result.m[1][0] = 2.0f * (xy - zw);
    result.m[1][1] = 1.0f - 2.0f * (xx + zz);
    result.m[1][2] = 2.0f * (yz + xw);

    result.m[2][0] = 2.0f * (xz + yw);
    result.m[2][1] = 2.0f * (yz - xw);
    result.m[2][2] = 1.0f - 2.0f * (xx + yy);

    return result;
}
