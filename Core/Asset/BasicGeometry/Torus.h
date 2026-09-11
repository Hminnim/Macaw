#pragma once

#include <array>
#include <cmath>
#include <numbers>

namespace BasicGeometry {
	namespace Torus {
		inline constexpr uint32 MajorSegments = 32;
		inline constexpr uint32 MinorSegments = 16;

		inline constexpr float MajorRadius = 5.0f / 13.0f;
		inline constexpr float MinorRadius = 1.5f / 13.0f;

		inline constexpr uint32 VertexCount = (MajorSegments + 1) * (MinorSegments + 1);
		inline constexpr uint32 IndexCount = MajorSegments * MinorSegments * 6;

		struct FGeometry {
			std::array<FVector3, VertexCount> Positions{};
			std::array<FVector3, VertexCount> Normals{};
			std::array<FVector2D, VertexCount> TexCoords{};
			std::array<uint32, IndexCount> Indices{};
		};

		inline FGeometry GenerateGeometry(float InMajorRadius = MajorRadius, float InMinorRadius = MinorRadius) {
			FGeometry Result{};

			uint32 Vertex = 0;

			for (uint32 Major = 0; Major <= MajorSegments; ++Major) {
				const float U = static_cast<float>(Major) / static_cast<float>(MajorSegments);
				const float Theta = U * std::numbers::pi_v<float> *2.0f;
				const float CosTheta = std::cos(Theta);
				const float SinTheta = std::sin(Theta);

				for (uint32 Minor = 0; Minor <= MinorSegments; ++Minor) {
					const float V = static_cast<float>(Minor) / static_cast<float>(MinorSegments);
					const float Phi = V * std::numbers::pi_v<float> *2.0f;
					const float CosPhi = std::cos(Phi);
					const float SinPhi = std::sin(Phi);

					const float RingRadius = InMajorRadius + InMinorRadius * CosPhi;

					Result.Positions[Vertex] = FVector3{ RingRadius * CosTheta, InMinorRadius * SinPhi, RingRadius * SinTheta };
					Result.Normals[Vertex] = FVector3{ CosPhi * CosTheta, SinPhi, CosPhi * SinTheta };
					Result.TexCoords[Vertex++] = FVector2D{ U, V };
				}
			}

			uint32 Index = 0;

			for (uint32 Major = 0; Major < MajorSegments; ++Major) {
				for (uint32 Minor = 0; Minor < MinorSegments; ++Minor) {
					const uint32 Current = Major * (MinorSegments + 1) + Minor;
					const uint32 MinorNext = Current + 1;
					const uint32 MajorNext = (Major + 1) * (MinorSegments + 1) + Minor;
					const uint32 MajorMinorNext = MajorNext + 1;

					Result.Indices[Index++] = Current;
					Result.Indices[Index++] = MinorNext;
					Result.Indices[Index++] = MajorMinorNext;

					Result.Indices[Index++] = Current;
					Result.Indices[Index++] = MajorMinorNext;
					Result.Indices[Index++] = MajorNext;
				}
			}

			return Result;
		}

		inline const FGeometry Geometry = GenerateGeometry();

		inline const auto& Positions = Geometry.Positions;
		inline const auto& Normals = Geometry.Normals;
		inline const auto& TexCoords = Geometry.TexCoords;
		inline const auto& Indices = Geometry.Indices;
	}
}
