#pragma once
#include "..\CSC8503Common\GameObject.h"
#include "..\CSC8503Common\NavigationGrid.h"
namespace NCL {
	namespace CSC8503 {

		class Enemy :public GameObject
		{
		public:
			Enemy();
			~Enemy();
			//GameObject* enemy;
			Vector3 goosePos;
			Vector3 enemyPos;
			NavigationGrid *grid1;
			float disToGoose;
			float disToDefault;
		};
	}
}