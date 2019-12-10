#pragma once
#include "CollisionVolume.h"
#include "../../Common/Vector3.h"
using namespace NCL::Maths;
namespace NCL {
	class WATERVolume :CollisionVolume
	{
	public:
		WATERVolume(const Vector3& halfDims) {
			type = VolumeType::AABB;
			halfSizes = halfDims;
		}
		~WATERVolume() {

		}

		Vector3 GetHalfDimensions() const {
			return halfSizes;
		}
	protected:
		Vector3 halfSizes;

	};
}
