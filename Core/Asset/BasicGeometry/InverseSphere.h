#pragma once

#include <array>
#include <cmath>
#include <numbers>

namespace BasicGeometry {
	namespace SkyDome {
		inline constexpr uint32 LongitudeSegments = 64;
		inline constexpr uint32 LatitudeSegments = 32;

		inline constexpr float Radius = 1.0f;

		inline constexpr uint32 VertexCount = (LatitudeSegments + 1) * (LongitudeSegments + 1);
		inline constexpr uint32 IndexCount = LatitudeSegments * LongitudeSegments * 6;

		struct FGeometry {
			std::array<FVector3, VertexCount> Positions{};
			std::array<FVector3, VertexCount> Normals{};
			std::array<FVector2D, VertexCount> TexCoords{};
			std::array<uint32, IndexCount> Indices{};
		};

		inline FGeometry GenerateGeometry() {
			FGeometry Result{};

			uint32 Vertex = 0;

			for (uint32 Latitude = 0; Latitude <= LatitudeSegments; ++Latitude) {
				const float V = static_cast<float>(Latitude) / static_cast<float>(LatitudeSegments);
				const float Phi = V * std::numbers::pi_v<float>;

				const float Y = std::cos(Phi);
				const float RingRadius = std::sin(Phi);

				for (uint32 Longitude = 0; Longitude <= LongitudeSegments; ++Longitude) {
					const float U = static_cast<float>(Longitude) / static_cast<float>(LongitudeSegments);
					const float Theta = U * std::numbers::pi_v<float> *2.0f;

					const float X = std::cos(Theta) * RingRadius;
					const float Z = std::sin(Theta) * RingRadius;

					Result.Positions[Vertex] = FVector3{
						X * Radius,
						Y * Radius,
						Z * Radius
					};

					Result.Normals[Vertex] = FVector3{
						-X,
						-Y,
						-Z
					};

					Result.TexCoords[Vertex++] = FVector2D{
						U,
						V
					};
				}
			}

			uint32 Index = 0;

			for (uint32 Latitude = 0; Latitude < LatitudeSegments; ++Latitude) {
				for (uint32 Longitude = 0; Longitude < LongitudeSegments; ++Longitude) {
					const uint32 TopLeft = Latitude * (LongitudeSegments + 1) + Longitude;
					const uint32 TopRight = TopLeft + 1;
					const uint32 BottomLeft = (Latitude + 1) * (LongitudeSegments + 1) + Longitude;
					const uint32 BottomRight = BottomLeft + 1;

					Result.Indices[Index++] = TopLeft;
					Result.Indices[Index++] = BottomRight;
					Result.Indices[Index++] = TopRight;

					Result.Indices[Index++] = TopLeft;
					Result.Indices[Index++] = BottomLeft;
					Result.Indices[Index++] = BottomRight;
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