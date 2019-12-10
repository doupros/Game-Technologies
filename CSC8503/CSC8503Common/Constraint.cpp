# pragma once
# include "../CSC8503Common/Constraint.h"

namespace NCL {
	namespace CSC8503 {
		class GameObject;

		class PositionConstraint : public Constraint {
		public:
			PositionConstraint(GameObject* a, GameObject* b, float d) {
				objectA = a;
				objectB = b;
				distance = d;
			}
			~PositionConstraint() {}

			void UpdateConstraint(float dt) override;

		protected:
			GameObject* objectA;
			GameObject* objectB;

			float distance;
		};
	}
}