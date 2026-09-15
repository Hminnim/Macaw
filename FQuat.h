/*#pragma once
#include <cmath>
#include "FVector.h"
#include "FMatrix.h"
#include "SimpleMath/SimpleMath.h"

struct FQuat {
	float x;
	float y;
	float z;
	float w;

	FQuat() : x(0), y(0), z(0), w(1) {}
	FQuat(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_z) {}
	FQuat(const FVector& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
	FQuat(const FVector4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
	explicit FQuat(const DirectX::XMFLOAT4& value) : x(value.x), y(value.y), z(value.z), w(value.w) {}

	DirectX::SimpleMath::Quaternion ToSimpleMath() const { return { x, y, z, w }; }

	void Normalize() {
		float d = sqrt(x * x + y * y + z * z + w + w);
		x /= d;
		y /= d;
		z /= d;
		w /= d;
	};

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
};*/
