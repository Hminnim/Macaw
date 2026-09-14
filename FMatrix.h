/*#pragma once

#include <cmath>
#include <limits>
#include "FVector.h"
#include "FQuat.h"
//using FQuat = DirectX::SimpleMath::Quaternion;

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

        return CreateRotationZ(roll)
            * CreateRotationX(pitch)
            * CreateRotationY(yaw);
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

    static FMatrix CreateFromQuaternion(const FQuat& rotation)
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

    DirectX::SimpleMath::Matrix ToSimpleMath() const
    {
        return {m[0][0], m[0][1], m[0][2], m[0][3],
                m[1][0], m[1][1], m[1][2], m[1][3],
                m[2][0], m[2][1], m[2][2], m[2][3],
                m[3][0], m[3][1], m[3][2], m[3][3]};
    }

    FQuat ToQuaternion() const
    {
        return FQuat::CreateFromRotationMatrix(*this);
    }

    bool Decompose(
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
};

inline const FMatrix FMatrix::Identity{};

inline FVector FVector::Transform(const FVector& position, const FMatrix& matrix)
{
    FVector result;
    if (!matrix.TransformCoord(position, result))
    {
        const float invalid = std::numeric_limits<float>::quiet_NaN();
        return {invalid, invalid, invalid};
    }
    return result;
}

inline FVector FVector::TransformNormal(const FVector& direction, const FMatrix& matrix)
{
    return matrix.TransformDirection(direction);
}*/
