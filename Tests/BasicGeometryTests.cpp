#include "PCH.h"
#include "doctest.h"

#include "../Core/Asset/BasicGeometry/Capsule.h"
#include "../Core/Asset/BasicGeometry/Corn.h"
#include "../Core/Asset/BasicGeometry/Cube.h"
#include "../Core/Asset/BasicGeometry/Cylinder.h"
#include "../Core/Asset/BasicGeometry/Plane.h"
#include "../Core/Asset/BasicGeometry/Pyramid.h"
#include "../Core/Asset/BasicGeometry/Sphere.h"
#include "../Core/Asset/BasicGeometry/Torus.h"

namespace {
	template<typename TPositions>
	void CheckUnitCubeBounds(const TPositions& Positions) {
		REQUIRE_FALSE(Positions.empty());

		FVector3 Minimum = Positions.front();
		FVector3 Maximum = Positions.front();

		for (const FVector3& Position : Positions) {
			Minimum.x = std::min(Minimum.x, Position.x);
			Minimum.y = std::min(Minimum.y, Position.y);
			Minimum.z = std::min(Minimum.z, Position.z);

			Maximum.x = std::max(Maximum.x, Position.x);
			Maximum.y = std::max(Maximum.y, Position.y);
			Maximum.z = std::max(Maximum.z, Position.z);

			CHECK(Position.x >= doctest::Approx(-0.5f));
			CHECK(Position.x <= doctest::Approx(0.5f));
			CHECK(Position.y >= doctest::Approx(-0.5f));
			CHECK(Position.y <= doctest::Approx(0.5f));
			CHECK(Position.z >= doctest::Approx(-0.5f));
			CHECK(Position.z <= doctest::Approx(0.5f));
		}

		CHECK((Minimum.x + Maximum.x) * 0.5f == doctest::Approx(0.0f).epsilon(0.00001));
		CHECK((Minimum.y + Maximum.y) * 0.5f == doctest::Approx(0.0f).epsilon(0.00001));
		CHECK((Minimum.z + Maximum.z) * 0.5f == doctest::Approx(0.0f).epsilon(0.00001));

		const float MaximumExtent = std::max({
			Maximum.x - Minimum.x,
			Maximum.y - Minimum.y,
			Maximum.z - Minimum.z
		});
		CHECK(MaximumExtent == doctest::Approx(1.0f).epsilon(0.00001));
	}
}

TEST_SUITE("Basic Geometry Tests") {
	TEST_CASE("Every basic geometry is centered and fits a unit cube") {
		SUBCASE("Plane") { CheckUnitCubeBounds(BasicGeometry::Plane::Positions); }
		SUBCASE("Cube") { CheckUnitCubeBounds(BasicGeometry::Cube::Positions); }
		SUBCASE("Sphere") { CheckUnitCubeBounds(BasicGeometry::Sphere::Positions); }
		SUBCASE("Capsule") { CheckUnitCubeBounds(BasicGeometry::Capsule::Positions); }
		SUBCASE("Cone") { CheckUnitCubeBounds(BasicGeometry::Cone::Positions); }
		SUBCASE("Cylinder") { CheckUnitCubeBounds(BasicGeometry::Cylinder::Positions); }
		SUBCASE("Pyramid") { CheckUnitCubeBounds(BasicGeometry::Pyramid::Positions); }
		SUBCASE("Torus") { CheckUnitCubeBounds(BasicGeometry::Torus::Positions); }
	}
}
