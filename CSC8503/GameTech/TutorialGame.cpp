#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include "../CSC8503Common/NavigationGrid.h"
#include <iostream>
#include <fstream>
#include "../../Common/Assets.h"
#include "../CSC8503Common/PositionConstraint.h"
#include "../CSC8503Common/WATERVolume.h"
#include <algorithm>
#include "../../Common/Maths.h"


using namespace NCL;
using namespace CSC8503;
using namespace Maths;


TutorialGame::TutorialGame() {
	world = new GameWorld();
	renderer = new GameTechRenderer(*world);
	physics = new PhysicsSystem(*world);
	grid = new NavigationGrid("MapFile20.txt");
	forceMagnitude = 50.0f;
	useGravity = false;
	inSelectionMode = false;
	aEnemy = new Enemy;
	//aEnemy->grid1 = grid;
	aEnemy->grid1 = new NavigationGrid("MapFile20.txt");
	Debug::SetRenderer(renderer);
	InitialiseAssets();
	ball->appleState = 0;
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes,
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh", &cubeMesh);
	loadFunc("sphere.msh", &sphereMesh);
	loadFunc("goose.msh", &gooseMesh);
	loadFunc("CharacterA.msh", &keeperMesh);
	loadFunc("CharacterM.msh", &charA);
	loadFunc("CharacterF.msh", &charB);
	loadFunc("Apple.msh", &appleMesh);

	basicTex = (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame() {
	delete cubeMesh;
	delete sphereMesh;
	delete gooseMesh;
	delete basicTex;
	delete basicShader;
	delete ball;
	delete grid;
	delete constraint;
	delete selectionObject;
	delete spawn;
	delete goose;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateGame(float dt) {
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}
	if (lockedObject != nullptr) {
		LockedCameraMovement();
	}

	UpdateKeys();

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(10, 40));
		//	physics->SetGravity(Vector3(9, 9, 9));
	}
	else {
		Debug::Print("(G)ravity off", Vector2(10, 40));
	}

	SelectObject();
	MoveSelectedObject();
	GoosePathFinding();
	GrabApple();
	MoveInWater();
	
	UpdateEnemy();

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	Debug::FlushRenderables();
	renderer->Render();
	if (lockedObject)
	{
		TimeCounting(dt);
		if (totalTime >= 180.0)
		{
			lockedObject = nullptr;
			InitCamera();
			Debug::Print("Game END ", Vector2(300, 700));
		}
		if (score >= 27)
		{
			lockedObject = nullptr;
			selectionObject = nullptr;
			InitCamera();
			Debug::Print("YOU WIN!  Time Use:" + std::to_string((int)totalTime), Vector2(300, 700));
		}
	}

	Debug::Print(std::to_string(apple.size()), Vector2(130, 130));
	Debug::Print("Score: " + std::to_string(score), Vector2(1000, 700));
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		selectionObject = nullptr;
		totalTime = 0;
		apple.clear();
		water.clear();
		enemys.clear();
		enemyStateMac.clear();
		score = 0;
		InitCamera();
		InitWorld(); //We can reset the simulation at any time with F1
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::X)) {
		lockedObject = goose;
		selectionObject = goose;
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}

}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view = world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		selectionObject->GetPhysicsObject()->AddForce(-rightAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		selectionObject->GetPhysicsObject()->AddForce(rightAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::W) && selectionObject)
	{
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis * forceMagnitude);
		/*Vector3 tempVec = selectionObject->GetTransform().GetLocalOrientation().ToEuler();
		if ((1 - Vector3::Dot(tempVec, fwdAxis) / tempVec.Length()) > 0) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0,2,0));
		}
		if ((1 - Vector3::Dot(tempVec, fwdAxis) / tempVec.Length()) < 0) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -2, 0));
		}*/
		Vector3 forceDir = selectionObject->GetPhysicsObject()->GetForce();
		forceDir.Normalise();
		float len = forceDir.Length();
		float angle = acos(forceDir.z / len);
		if (forceDir.x < 0)angle = -angle;
		Quaternion currentDir = selectionObject->GetTransform().GetLocalOrientation();
		Quaternion dirShouldBe = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), Maths::RadiansToDegrees(angle));
		Quaternion finalDir = Quaternion::Lerp(currentDir, dirShouldBe, 0.20f);
		selectionObject->GetTransform().SetLocalOrientation(finalDir);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::A)&& selectionObject)
	{
		selectionObject->GetPhysicsObject()->AddForce(-rightAxis * forceMagnitude);
		/*Vector3 tempVec = selectionObject->GetTransform().GetLocalOrientation().ToEuler();
		if ((1 - Vector3::Dot(tempVec, -rightAxis) / tempVec.Length()) > 0) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 2, 0));
		}
		if ((1 - Vector3::Dot(tempVec, -rightAxis) / tempVec.Length()) < 0) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -2, 0));
		}*/
		ChangeObjDir(selectionObject);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::S) && selectionObject)
	{
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis * forceMagnitude);
		ChangeObjDir(selectionObject);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::D) && selectionObject)
	{
		selectionObject->GetPhysicsObject()->AddForce(rightAxis * forceMagnitude);
		ChangeObjDir(selectionObject);
	}

}

void  TutorialGame::LockedCameraMovement() {
	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetWorldPosition();
		Vector3 camPos = objPos + lockedOffset + Vector3(0, 80, 80);

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); /*//nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x-30);
		world->GetMainCamera()->SetYaw(angles.y);
		*/
		//Vector3 objPos = lockedObject->GetTransform().GetWorldPosition();
		objPos.y += 140; objPos.z += 20;
		world->GetMainCamera()->SetPosition(objPos);
		world->GetMainCamera()->SetPitch(-80);
		world->GetMainCamera()->SetYaw(angles.y);

	}
}

void TutorialGame::DebugObjectMovement() {
	//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::W)) {
			selectionObject->GetPhysicsObject()->AddForce(selectionObject->GetTransform().GetLocalOrientation() * Vector3(0, 0, 20));
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::S)) {
			selectionObject->GetPhysicsObject()->AddForce(selectionObject->GetTransform().GetLocalOrientation() * Vector3(0, 0, -20));
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::A)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(-10, 0, 0));
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::D)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(10, 0, 0));
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}
	}
	DrawBaseLine();
}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around.

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(10, 0));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject->isWall ? Debug::Print("is Wall", Vector2(10, 60)) : Debug::Print("is not Wall", Vector2(10, 60));
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
			if (selectionObject) {
				if (lockedObject == selectionObject) {
					lockedObject = nullptr;
				}
				else {
					lockedObject = selectionObject;
				}
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(10, 0));
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	renderer->DrawString("Click Force :" + std::to_string(forceMagnitude),
		Vector2(10, 20)); // Draw debug text at 10 ,20
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return;// we havenet selected anything !
	}
	// Push the selected object !
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		//
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				//selectionObject->GetPhysicsObject() ->AddForce(ray.GetDirection() * forceMagnitude);
				selectionObject->GetPhysicsObject()->
					AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
	/*if(Window::GetKeyboard()->KeyPressed(KeyboardKeys::W)){
		selectionObject->GetPhysicsObject()->AddForce(Vector3(forceMagnitude, 0, 0));
		//->AddForceAtPosition(Vector3(0, 0,-forceMagnitude),Vector3(1,1,1));
		//selectionObject->GetPhysicsObject()->SetLinearVelocity(Vector3(30, 0, 0));
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::S)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(-forceMagnitude, 0, 0));
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::A)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -forceMagnitude));
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::D)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, forceMagnitude));
	}*/
}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.5f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(-90.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();
	//InitGridMap();
	GenerateMap("MapFile20.txt");
	//InitMixedGridWorld(10, 10, 3.5f, 3.5f);
	//AddGooseToWorld(Vector3(30, 2, 0));

	AddGooseToWorld(Vector3(12, 1, 12));
	//AddAppleToWorld(Vector3(35, 2, 0));
	//AddParkKeeperToWorld(Vector3(120, 2, 18));
	//AddCharacterToWorld(Vector3(45, 2, 0));
	AddEnemyToWorld(Vector3(114, 2, 18));
	AddEnemyToWorld(Vector3(114, 2, 60));
	AddEnemyToWorld(Vector3(60, 2, 60));
	AddEnemyToWorld(Vector3(18, 2, 60));
	AddEnemyToWorld(Vector3(30, 2, 114));
	AddEnemyToWorld(Vector3(20, 2, 96));
	ball = AddSphereToWorld(Vector3(114, 5, 108), 2);

	water.push_back(AddWaterCubeToWorld(Vector3(3 * 6, -1.8, 3 * 6)));
	water.push_back(AddWaterCubeToWorld(Vector3(2 * 6, -1.8, 3 * 6)));
	water.push_back(AddWaterCubeToWorld(Vector3(3 * 6, -1.8, 2 * 6)));

	AddSpawnToWorld(Vector3(12,-1.8,12));
	AddFloorToWorld(Vector3(0, -4, 0));

	DrawBaseLine();

	StateMachineTest();

}

//From here on it's functions to add in objects to the world!

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();
	floor->isWall = true;
	Vector3 floorSize = Vector3(70, 2, 70);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(floorSize);
	//floor->GetTransform().SetWorldPosition(position);
	Vector3 tempPos = Vector3(position.x + floorSize.x, position.y, position.z + floorSize.z);
	floor->GetTransform().SetWorldPosition(tempPos);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->GetRenderObject()->SetColour(Vector4(0, 0, 0, 1));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple'
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject("sphere");

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);
	sphere->GetTransform().SetWorldScale(sphereSize);
	sphere->GetTransform().SetWorldPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));
	sphere->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass, bool wall) {
	GameObject* cube = new GameObject("cube");
	cube->isWall = wall;

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->GetRenderObject()->SetColour(Vector4(1, 0.9, 0.8, 1));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddWaterCubeToWorld(const Vector3& position,bool isWater) {
	GameObject* water = new GameObject("water");
	water->isWall = true;
	water->isWater = true;
	WATERVolume* volume = new WATERVolume(Vector3(3, 0.01, 3));
	water->SetBoundingVolume((CollisionVolume*)volume);
	water->GetTransform().SetWorldPosition(position);
	water->GetTransform().SetWorldScale(Vector3(3, 0.1, 3));
	water->SetRenderObject(new RenderObject(&water->GetTransform(), cubeMesh, basicTex, basicShader));
	water->GetRenderObject()->SetColour(Vector4(0, 0, 0.8, 1));
	water->SetPhysicsObject(new PhysicsObject(&water->GetTransform(), water->GetBoundingVolume()));
	water->GetPhysicsObject()->SetInverseMass(0);
	water->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(water);
	return water;
}

GameObject* TutorialGame::AddSpawnToWorld(const Vector3& position) {
	spawn = new GameObject("spawn");
	spawn->isWall = true;
	spawn->isWater = false;
	spawn->isSpown = true;
	AABBVolume* volume = new AABBVolume(Vector3(3, 1.5, 3));
	spawn->SetBoundingVolume((CollisionVolume*)volume);
	spawn->GetTransform().SetWorldPosition(position);
	spawn->GetTransform().SetWorldScale(Vector3(3, 0.1, 3));
	spawn->SetRenderObject(new RenderObject(&spawn->GetTransform(), cubeMesh, basicTex, basicShader));
	spawn->GetRenderObject()->SetColour(Vector4(0.8, 0, 0, 1));
	spawn->SetPhysicsObject(new PhysicsObject(&spawn->GetTransform(), spawn->GetBoundingVolume()));
	spawn->GetPhysicsObject()->SetInverseMass(0);
	spawn->GetPhysicsObject()->InitCubeInertia();
	world->AddGameObject(spawn);
	return spawn;
}

GameObject* TutorialGame::AddGooseToWorld(const Vector3& position)
{
	float size = 1.2f;
	float inverseMass = 1.0f;

	goose = new GameObject("goose");

	SphereVolume* volume = new SphereVolume(size);
	goose->SetBoundingVolume((CollisionVolume*)volume);

	goose->GetTransform().SetWorldScale(Vector3(size, size, size));
	goose->GetTransform().SetWorldPosition(position);

	goose->SetRenderObject(new RenderObject(&goose->GetTransform(), gooseMesh, nullptr, basicShader));
	goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));

	goose->GetPhysicsObject()->SetInverseMass(inverseMass);
	goose->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(goose);

	return goose;
}

GameObject* TutorialGame::AddParkKeeperToWorld(const Vector3& position)
{
	float meshSize = 4.0f;
	float inverseMass = 0.5f;

	GameObject* keeper = new GameObject("keeper");

	AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * meshSize);
	keeper->SetBoundingVolume((CollisionVolume*)volume);

	keeper->GetTransform().SetWorldScale(Vector3(meshSize, meshSize, meshSize));
	keeper->GetTransform().SetWorldPosition(position);

	keeper->SetRenderObject(new RenderObject(&keeper->GetTransform(), keeperMesh, nullptr, basicShader));
	keeper->SetPhysicsObject(new PhysicsObject(&keeper->GetTransform(), keeper->GetBoundingVolume()));

	keeper->GetPhysicsObject()->SetInverseMass(inverseMass);
	keeper->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(keeper);

	return keeper;
}

GameObject* TutorialGame::AddCharacterToWorld(const Vector3& position) {
	float meshSize = 4.0f;
	float inverseMass = 0.5f;

	auto pos = keeperMesh->GetPositionData();

	Vector3 minVal = pos[0];
	Vector3 maxVal = pos[0];

	for (auto& i : pos) {
		maxVal.y = max(maxVal.y, i.y);
		minVal.y = min(minVal.y, i.y);
	}

	GameObject* character = new GameObject();

	float r = rand() / (float)RAND_MAX;


	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform().SetWorldScale(Vector3(meshSize, meshSize, meshSize));
	character->GetTransform().SetWorldPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), r > 0.5f ? charA : charB, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddAppleToWorld(const Vector3& position) {
	GameObject* apple = new GameObject("apple");

	SphereVolume* volume = new SphereVolume(0.7f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform().SetWorldScale(Vector3(4, 4, 4));
	apple->GetTransform().SetWorldPosition(position);
	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), appleMesh, nullptr, basicShader));
	apple->GetRenderObject()->SetColour(Vector4(0, 1, 1, 1));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();
	apple->defultPos = position;
	world->AddGameObject(apple);

	return apple;
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	//AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
	//AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitGridMap() {
	AddCubeToWorld(Vector3(10, -1, 10), Vector3(1, 1, 10), 0.0f);
	AddSphereToWorld(Vector3(15, 0, 15), 1, 3.5f);
	int xSize = 20; int zSize = 20; int tall = 2;
	/*for (int x = 0; x < 10; x++)
	{
		for (int z = 0; z < 10; z++)
		{
			if ((x==0||x==9)||(z==0||z==9))
			{
				AddCubeToWorld(Vector3(x * 2+80, -1, z * 2+80), Vector3(1, 1, 1), 0);
			}
		}
	}*/

	//AddCubeToWorld(Vector3(100, 0, 20), Vector3(xSize, tall, zSize), 0);

}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols + 1; ++x) {
		for (int z = 1; z < numRows + 1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
	//AddFloorToWorld(Vector3(0, -3, 0));
}

void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(8, 8, 8);

	float	invCubeMass = 5;
	int		numLinks = 25;
	float	maxDistance = 30;
	float	cubeDistance = 20;

	Vector3 startPos = Vector3(500, 1000, 500);

	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);

	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0);

	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}

	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

void TutorialGame::SimpleGJKTest() {
	Vector3 dimensions = Vector3(5, 5, 5);
	Vector3 floorDimensions = Vector3(100, 2, 100);

	GameObject* fallingCube = AddCubeToWorld(Vector3(0, 20, 0), dimensions, 10.0f);
	GameObject* newFloor = AddCubeToWorld(Vector3(0, 0, 0), floorDimensions, 0.0f);

	delete fallingCube->GetBoundingVolume();
	delete newFloor->GetBoundingVolume();

	fallingCube->SetBoundingVolume((CollisionVolume*)new OBBVolume(dimensions));
	newFloor->SetBoundingVolume((CollisionVolume*)new OBBVolume(floorDimensions));

}

void TutorialGame::GenerateMap(const std::string& filename) {
	int nodeSize = 0;
	int gridWidth = 0;
	int gridHeight = 0;
	//GridNode* allNodes = nullptr;
	std::ifstream infile(Assets::DATADIR + filename);
	//if (infile.is_open());
	infile >> nodeSize;
	infile >> gridWidth;
	infile >> gridHeight;
	//allNodes = new GridNode[gridWidth * gridHeight];
	int count = 0;
	for (int z = 0; z < gridHeight; z++) {
		for (int x = 0; x < gridWidth; x++) {
			char type;
			infile >> type;
			if (type == 120) {
				AddCubeToWorld(Vector3(x * nodeSize + nodeSize, 1, z * nodeSize + nodeSize), Vector3(3, 3, 3), 0, true);
			}
			else if (type == 46 && count < 25) {
				if (rand() % 10 == 8) {
					apple.push_back(AddAppleToWorld(Vector3(x * nodeSize + nodeSize, 1, z * nodeSize + nodeSize)));
					count++;
				}
			}
		}
	}
}

void TutorialGame::GoosePathFinding() {
	vector<Vector3> mapNodes;
	if (selectionObject)
	{
		NavigationPath outPath;
		Vector3 startPos = selectionObject->GetTransform().GetWorldPosition();
		Vector3 endPos = goose->GetTransform().GetWorldPosition();//(120, 6, 120);

		bool found = grid->FindPath(startPos, endPos, outPath);
		Vector3 pos;
		while (outPath.PopWaypoint(pos)) {
			mapNodes.push_back(pos);
		}
		for (int i = 1; i < mapNodes.size(); ++i) {
			Vector3 a = mapNodes[i - 1];
			//	a.x += startPos.y;
			Vector3 b = mapNodes[i];

			//Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
			Debug::DrawLine(Vector3(a.x + 6, a.y, a.z + 6), Vector3(b.x + 6, b.y, b.z + 6), Vector4(0, 1, 0, 1));
		}
	}
}

void TutorialGame::DrawBaseLine() {
	//std::vector<GameObject*>::const_iterator first;
	//std::vector<GameObject*>::const_iterator last;
	//world->GetObjectIterators(first, last);
	//for (std::vector<GameObject*>::const_iterator i = first; i != last; ++i) {
	if (selectionObject) {
		Vector3 tempVec = selectionObject->GetTransform().GetWorldPosition();
		Vector3 tempAng = selectionObject->GetTransform().GetLocalOrientation() * Vector3(0, 0, 1);
		tempAng.Normalise();
		Debug::DrawLine(Vector3(0, 0, 0), tempAng * 30, Vector4(0, 0, 1, 1));
		Debug::DrawLine(tempVec, tempVec + tempAng * 30, Vector4(0, 0, 1, 1));
	}
	//}
}

void TutorialGame::ChangeObjDir(GameObject* object) {
	Vector3 forceDir = object->GetPhysicsObject()->GetForce();
	forceDir.Normalise();
	float len = forceDir.Length();
	if (len != 0) {
		float angle = acos(forceDir.z / len);
		if (forceDir.x < 0)angle = -angle;
		Quaternion currentDir = object->GetTransform().GetLocalOrientation();
		Quaternion dirShouldBe = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), Maths::RadiansToDegrees(angle));
		Quaternion finalDir = Quaternion::Lerp(currentDir, dirShouldBe, 0.20f);
		object->GetTransform().SetLocalOrientation(finalDir);
	}
}

void TutorialGame::GrabApple() {
	std::vector < GameObject* >::const_iterator first = apple.begin();
	std::vector < GameObject* >::const_iterator last= apple.end();
	vector<GameObject> tempVector;
	//world->GetObjectIterators(first, last);

	CollisionDetection::CollisionInfo info;

	for (auto i = first; i != last; ++i) {
		bool gooseAndspawn = CollisionDetection::ObjectIntersection(spawn, goose, info);
		if ((*i)->appleState==2&& gooseAndspawn){
			std::cout << "Collision between " << (spawn)->GetName() << " and " << (goose)->GetName() << std::endl;
			(*i)->appleState = 3;//collected
			Vector3 tempVec3 = spawn->GetTransform().GetWorldPosition();
			(*i)->GetTransform().SetWorldPosition(Vector3(tempVec3.x, tempVec3.y + 20, tempVec3.z));
			score++;
		}
		if ((*i)->appleState==2&&!gooseAndspawn){
			Vector3 tempVec3 = goose->GetTransform().GetWorldPosition();
			(*i)->GetTransform().SetWorldPosition(Vector3(tempVec3.x, tempVec3.y + 10, tempVec3.z));
		}
		if (CollisionDetection::ObjectIntersection(*i,goose,info)&& (*i)->appleState ==0){
			std::cout << "Collision between " << (*i)->GetName() << " and " << (goose)->GetName() << std::endl;
			(*i)->appleState = 2;//collecting
		}
		if (CollisionDetection::ObjectIntersection(ball, goose, info)&&ball->appleState==0)
		{
			ball->appleState = 2;
			constraint = new PositionConstraint(goose, ball, 2.0f);
			world->AddConstraint(constraint);
		}
		if (ball->appleState == 2 && gooseAndspawn)
		{
			world->RemoveConstraint(constraint);
			ball->GetTransform().SetWorldPosition(Vector3(0, 0, 0));
			ball->appleState = 3;
			score += 2;
		}

	}	
	/*	if ((*i)->GetPhysicsObject() == nullptr) {
			continue;(*i).
		}
		for (auto j = i + 1; j != last; ++j) {
			if ((*j)->GetPhysicsObject() == nullptr) {
				continue;
			}
			if (((*i)->GetName() == "goose" && (*j)->GetName() == "apple") ||
				((*i)->GetName() == "apple" && (*j)->GetName() == "goose")) {
				Vector3 tempVec = (*i)->GetTransform().GetWorldOrientation().ToEuler();
				tempVec.y += 2;
				(*j)->GetTransform().SetWorldPosition(tempVec);
			}*/
	
	for (auto j = enemys.begin(); j != enemys.end(); j++)//touch AI back all apple
	{
		for (auto i = first; i != last; i++)
		{
			if (CollisionDetection::ObjectIntersection((*j), goose, info) && (*i)->appleState == 2)
			{
				std::cout << "Collision between " << (*j)->GetName() << " and " << (goose)->GetName() << std::endl;
				(*i)->appleState = 0;
				(*i)->GetTransform().SetWorldPosition((*i)->defultPos);
			}
		}
	}


}

void TutorialGame::MoveInWater() {
	std::vector < GameObject* >::const_iterator first = water.begin();
	std::vector < GameObject* >::const_iterator last = water.end();
	for (auto i = first; i != last; ++i){
		CollisionDetection::CollisionInfo info;
		bool gooseAndWater = CollisionDetection::ObjectIntersection((*i), goose, info);
		if (gooseAndWater){
			goose->GetPhysicsObject()->ClearForces();
			std::cout << "Collision between " << (*i)->GetName() << " and " << (goose)->GetName() << std::endl;
		}
	}
}

void TutorialGame::PathFinding(Vector3& firstPos,Vector3 &secondPos,GameObject *aObj,NavigationGrid* grid) {
	vector<Vector3> mapNodes;
	if (aObj&&grid){
		NavigationPath outPath;
		//Vector3 startPos = selectionObject->GetTransform().GetWorldPosition();
		//Vector3 endPos = goose->GetTransform().GetWorldPosition();//(120, 6, 120);
		bool found = grid->FindPath(firstPos, secondPos, outPath);
		Vector3 pos;
		while (outPath.PopWaypoint(pos)) {
			mapNodes.push_back(pos);
		}
		for (int i = 1; i < mapNodes.size(); ++i) {
			Vector3 a = mapNodes[i - 1];
			//	a.x += startPos.y;
			Vector3 b = mapNodes[i];

			//Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
			Debug::DrawLine(Vector3(a.x + 6, a.y, a.z + 6), Vector3(b.x + 6, b.y, b.z + 6), Vector4(1, 0, 0, 1));
			aObj->GetPhysicsObject()->AddForce((Vector3(b.x + 6, b.y, b.z + 6) - Vector3(a.x + 6, a.y, a.z + 6)).Normalised()*10);
			Vector3 forceDir = aObj->GetPhysicsObject()->GetForce();
			forceDir.Normalise();
			float len = forceDir.Length();
			if (len != 0) {
				float angle = acos(forceDir.z / len);
				if (forceDir.x < 0)angle = -angle;
				Quaternion currentDir = aObj->GetTransform().GetLocalOrientation();
				Quaternion dirShouldBe = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), Maths::RadiansToDegrees(angle));
				Quaternion finalDir = Quaternion::Lerp(currentDir, dirShouldBe, 0.20f);
				aObj->GetTransform().SetLocalOrientation(finalDir);
			}
		}
	}
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3 &position) {
	float inverseMass = 0.5f;
	aEnemy = new Enemy;

	aEnemy->GetTransform().SetWorldPosition(position);
	aEnemy->enemyPos = position;
	aEnemy->SetRenderObject
	(new RenderObject(&aEnemy->GetTransform(), keeperMesh, nullptr, basicShader));

	aEnemy->SetPhysicsObject
	(new PhysicsObject(&aEnemy->GetTransform(), aEnemy->GetBoundingVolume()));

	aEnemy->GetPhysicsObject()->SetInverseMass(inverseMass);
	aEnemy->GetPhysicsObject()->InitCubeInertia();
	world->AddGameObject(aEnemy);
	enemys.push_back(aEnemy);
	return aEnemy;

}

void TutorialGame::enemyTrack(void* data) {
	Enemy* temp = (Enemy*)data;
	temp->disToGoose = (temp->goosePos - temp->GetTransform().GetLocalPosition()).Length();
	temp->disToDefault = (temp->enemyPos - temp->GetTransform().GetLocalPosition()).Length();
	PathFinding(temp->GetTransform().GetWorldPosition(), temp->goosePos, temp,temp->grid1);
	//std::cout << "Track" << temp->disToGoose <<"/t"<< temp->disToDefault <<std::endl;
}

void TutorialGame::enemyBack(void* data) {
	Enemy* temp = (Enemy*)data;
	temp->disToGoose = (temp->goosePos - temp->GetTransform().GetWorldPosition()).Length();
	temp->disToDefault = (temp->enemyPos - temp->GetTransform().GetWorldPosition()).Length();
	PathFinding( temp->GetTransform().GetWorldPosition(), temp->enemyPos, temp, temp->grid1);

	//std::cout << "Back" << std::endl;
}

void TutorialGame::enemyStay(void* data) {
	Enemy* temp = (Enemy*)data;
	temp->disToGoose = (temp->goosePos - temp->GetTransform().GetWorldPosition()).Length();
	temp->disToDefault = (temp->enemyPos - temp->GetTransform().GetWorldPosition()).Length();
	//std::cout << "stay" << std::endl;
}

void TutorialGame::StateMachineTest() {
	for (auto i = enemys.begin(); i != enemys.end(); i++)
	{
		
		StateMachine* newMachine = new StateMachine();

		StateFunc TrackEnemy = &TutorialGame::enemyTrack;
		StateFunc BackEnemy = &TutorialGame::enemyBack;
		StateFunc StayEnemy = &TutorialGame::enemyStay;

		GenericState* Track = new GenericState(TrackEnemy, (void*)((*i)));
		GenericState* Back = new GenericState(BackEnemy, (void*)((*i)));
		GenericState* Stay = new GenericState(StayEnemy, (void*)((*i)));

		newMachine->AddState(Track);
		newMachine->AddState(Back);
		newMachine->AddState(Stay);

		GenericTransition<float&, float>* transitionA =
			new GenericTransition<float&, float>(
				GenericTransition<float&, float>::GreaterThanTransition, (*i)->disToDefault, 25.0f, Track, Back);

		GenericTransition<float&, float>* transitionB =
			new GenericTransition<float&, float>(
				GenericTransition<float&, float>::LessThanTransition, (*i)->disToDefault, 5.0f, Back, Stay);

		GenericTransition<float&, float>* transitionC =
			new GenericTransition<float&, float>(
				GenericTransition<float&, float>::LessThanTransition, (*i)->disToGoose, 30.0f, Stay, Track);

		GenericTransition<float&, float>* transitionD =
			new GenericTransition<float&, float>(
				GenericTransition<float&, float>::GreaterThanTransition, (*i)->disToGoose, 50.0f, Track, Back);

		newMachine->AddTransition(transitionA);
		newMachine->AddTransition(transitionB);
		newMachine->AddTransition(transitionC);
		newMachine->AddTransition(transitionD);
		enemyStateMac.emplace_back(newMachine);
	}
}

void TutorialGame::UpdateEnemy(){
	std::vector<StateMachine*>::iterator first = enemyStateMac.begin();
	
	for (auto i = first; i != enemyStateMac.end(); i++)
	{
		(*i)->Update();
	}
	Vector3 goosePos = goose->GetTransform().GetWorldPosition();
	for (auto i = enemys.begin(); i != enemys.end(); i++)
	{
		(*i)->goosePos = goosePos;
	}
}

void TutorialGame::TimeCounting(float dt) {
	totalTime += dt;
	Debug::Print("totalTime: " + std::to_string((int)totalTime), Vector2(540, 700));
}

void TutorialGame::ServerWorking() {

}

void TutorialGame::ClientWorking() {

}

