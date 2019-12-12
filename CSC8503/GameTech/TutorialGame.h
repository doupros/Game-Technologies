#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/NavigationGrid.h"
#include "../CSC8503Common/StateMachine.h"
#include "Enemy.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"
//#include "NetworkedGame.h"



namespace NCL {
	namespace CSC8503 {
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/
			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void BridgeConstraintTest();
			void SimpleGJKTest();
			void InitGridMap();

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();
			void LockedCameraMovement();
			void GenerateMap(const std::string& filename);
			void GoosePathFinding();
			void DrawBaseLine();
			void ChangeObjDir(GameObject* object);
			void GrabApple();
			void MoveInWater();
			static void enemyTrack(void* data);
			static void enemyBack(void* data);
			static void enemyStay(void* data);
			void StateMachineTest();
			void UpdateEnemy();
			float totalTime=0;
			void TimeCounting(float dt);
			void ServerWorking();
			void ClientWorking();


			GameObject* AddEnemyToWorld(const Vector3& position);

			static void PathFinding(Vector3& firstPos, Vector3& secondPos, GameObject* a, NavigationGrid* grid);

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f,bool wall = 0);
			GameObject* AddWaterCubeToWorld(const Vector3& position, bool isWater = 1);
			GameObject* AddSpawnToWorld(const Vector3& position);
			//IT'S HAPPENING
			GameObject* AddGooseToWorld(const Vector3& position);
			GameObject* AddParkKeeperToWorld(const Vector3& position);
			GameObject* AddCharacterToWorld(const Vector3& position);
			GameObject* AddAppleToWorld(const Vector3& position);
			vector<GameObject*> apple;
			vector<GameObject*> water;
			vector<StateMachine*> enemyStateMac;
			vector<Enemy*> enemys;

			NavigationGrid* grid = nullptr;
			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;
			Enemy* aEnemy;
			int score=0;
			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLShader*	basicShader = nullptr;

			//Coursework Meshes
			OGLMesh*	gooseMesh	= nullptr;
			OGLMesh*	keeperMesh	= nullptr;
			OGLMesh*	appleMesh	= nullptr;
			OGLMesh*	charA		= nullptr;
			OGLMesh*	charB		= nullptr;

			//Coursework Additional functionality	
			GameObject* spawn;
			GameObject* goose;
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}
		};
	}
}

