#include "Enemy.h"
#include "..\CSC8503Common\AABBVolume.h"
namespace NCL {
	namespace CSC8503 {
		Enemy::Enemy() {
			float meshSize = 4.0f;
			// = new GameObject("enemy");
			grid1 = new NavigationGrid("MapFile20.txt");
			AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * meshSize);
			this->SetBoundingVolume((CollisionVolume*)volume);
			this->GetTransform().SetWorldScale(Vector3(meshSize, meshSize, meshSize));
		}
	}
}