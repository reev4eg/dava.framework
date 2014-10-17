#include "stdafx.h"
#include "DAVAEngine.h"

#include "IdeUnitTestsSupport.h"

namespace
{
	DAVA::float32 SquareDist(const DAVA::Matrix4& m1, const DAVA::Matrix4& m2)
	{
		DAVA::float32 result = 0.0f;
		for (DAVA::uint32 i = 0; i < 4; ++i)
			for (DAVA::uint32 j = 0; j < 4; ++j)
				result += DAVA::Abs(m1(i, j) - m2(i, j)) * DAVA::Abs(m1(i, j) - m2(i, j));

		return result;
	}

	DAVA::float32 TestMatrixDecomposition(const DAVA::Matrix4& mat)
	{
		DAVA::Vector3 position, scale;
		DAVA::Quaternion rotation;
		mat.Decomposition(position, scale, rotation);

		DAVA::Matrix4 reconstructedMatrix = rotation.GetMatrix() * DAVA::Matrix4::MakeScale(scale);
		reconstructedMatrix.SetTranslationVector(position);

		return SquareDist(reconstructedMatrix, mat);
	}
}


IDE_TEST_CASE_START(MathTestCase, "[math]")
{
	IDE_REQUIRE(TestMatrixDecomposition(DAVA::Matrix4::MakeTranslation(DAVA::Vector3(10.0f, 0.0f, 0.0f))) < 0.0001f);
	IDE_REQUIRE(TestMatrixDecomposition(DAVA::Matrix4::MakeRotation(DAVA::Vector3(1.0f, 0.0f, 0.0f), DAVA::PI_05)) < 0.0001f);
	IDE_REQUIRE(TestMatrixDecomposition(DAVA::Matrix4::MakeScale(DAVA::Vector3(3.0f, 3.0f, 3.0f))) < 0.0001f);

	DAVA::Vector3 axis(0.0f, 1.0f, 1.0f);
	axis.Normalize();
	IDE_REQUIRE(
		TestMatrixDecomposition(
		DAVA::Matrix4::MakeTranslation(DAVA::Vector3(10.0f, 0.0f, 0.0f)) *
		DAVA::Matrix4::MakeRotation(axis, DAVA::PI_05 * 0.25f) *
		DAVA::Matrix4::MakeScale(DAVA::Vector3(3.0f, 3.0f, 3.0f))) < 0.0001f);

	IDE_SECTION("test section")
	{
		int i = 0;
		i++;
		IDE_REQUIRE(1 == i);
	}
}
IDE_TEST_CASE_END
