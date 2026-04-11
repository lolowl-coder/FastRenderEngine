#pragma once

#include <glm/glm.hpp>

namespace fre
{
	template<class T>
	struct BoundingBox
	{
		BoundingBox()
			: mMin(std::numeric_limits<float>::max())
			, mMax(std::numeric_limits<float>::min())
		{
		}

		BoundingBox(const T& mn, const T& mx)
			: mMin(mn)
			, mMax(mx)
		{
		}

		T getCenter() const
		{
			return (mMax + mMin) * 0.5f;
		}

		T getSize() const
		{
			return mMax - mMin;
		}

		bool operator == (const BoundingBox<T>& other) const
		{
			return isEqual(mMin);
		}

		T mMin;
		T mMax;
	};

	using BoundingBox3D = BoundingBox<glm::vec3>;
	using BoundingBox2D = BoundingBox<glm::vec2>;
}