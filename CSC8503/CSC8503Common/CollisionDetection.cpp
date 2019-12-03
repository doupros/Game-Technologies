#include "CollisionDetection.h"
#include "CollisionVolume.h"
#include "AABBVolume.h"
#include "OBBVolume.h"
#include "SphereVolume.h"
#include "../../Common/Vector2.h"
#include "../../Common/Window.h"
#include "../../Common/Maths.h"

#include <list>

#include "../CSC8503Common/Simplex.h"

#include "Debug.h"

using namespace NCL;

bool CollisionDetection::RayPlaneIntersection(const Ray& r, const Plane& p, RayCollision& collisions) {
	return false;
}

bool CollisionDetection::RayIntersection(const Ray& ray, GameObject& object, RayCollision& collision) {
	const Transform& transform = object.GetConstTransform();
	const CollisionVolume* volume = object.GetBoundingVolume();
	if (!volume) {
		return false;
	}
	switch (volume->type) {
	case VolumeType::AABB: return RayAABBIntersection(ray, transform, (const AABBVolume&)*volume, collision);
	case VolumeType::OBB: return RayOBBIntersection(ray, transform, (const OBBVolume&)*volume, collision);
	case VolumeType::Sphere: return RaySphereIntersection(ray, transform, (const SphereVolume&)*volume, collision);
	}
	return false;
}

bool CollisionDetection::RayBoxIntersection(const Ray& ray, const Vector3& boxPos, const Vector3& boxSize, RayCollision& collision) {
	Vector3 boxMin = boxPos - boxSize;
	Vector3 boxMax = boxPos + boxSize;

	Vector3 rayPos = ray.GetPosition();//激光原点
	Vector3 rayDir = ray.GetDirection();//初始化过的 光线法向量

	Vector3 tVals(-1, -1, -1);

	for (int i = 0; i < 3; ++i) { // get best 3 intersections 获取距离激光远点最近的 三个面 
		if (rayDir[i] > 0) {
			tVals[i] = (boxMin[i] - rayPos[i]) / rayDir[i];
		}
		else if (rayDir[i] < 0) {
			tVals[i] = (boxMax[i] - rayPos[i]) / rayDir[i];
		}
	}
	float bestT = tVals.GetMaxElement();//获取最大幅度点

	// the rayDir loop above assumes that the ray is infinite both
	// backwards and forwards , so we can intersect with
	// objects ’behind ’ us!
	if (bestT < 0.0f) {//反向激光
		return false; // no backwards rays !
	}
	Vector3 intersection = rayPos + (rayDir * bestT);

	const float epsilon = 0.0001f; // an amount of leeway in our calcs

	for (int i = 0; i < 3; ++i) {
		if (intersection[i] + epsilon < boxMin[i] ||
			intersection[i] - epsilon > boxMax[i]) {
			return false; // best intersection doesn ’t touch the box !
		}
	}
	collision.collidedAt = intersection;//碰撞点
	collision.rayDistance = bestT;
	return true;
}


bool CollisionDetection::RayAABBIntersection(const Ray& ray, const Transform& worldTransform, const AABBVolume& volume, RayCollision& collision) {
	Vector3 boxPos = worldTransform.GetWorldPosition();
	Vector3 boxSize = volume.GetHalfDimensions();
	return RayBoxIntersection(ray, boxPos, boxSize, collision);
}

bool CollisionDetection::RayOBBIntersection(const Ray& ray, const Transform& worldTransform, const OBBVolume& volume, RayCollision& collision) {
	Quaternion orientation = worldTransform.GetWorldOrientation();
	Vector3 position = worldTransform.GetWorldPosition();

	Matrix3 transform = orientation.ToMatrix3();
	Matrix3 invTransform = orientation.Conjugate().ToMatrix3();

	Vector3 localRayPos = ray.GetPosition() - position;

	Ray tempRay(invTransform * localRayPos, invTransform * ray.GetDirection());

	bool collided = RayBoxIntersection(tempRay, Vector3(), volume.GetHalfDimensions(), collision);

	if (collided) {
		collision.collidedAt = transform * collision.collidedAt + position;
	}
	return collided;
}

bool CollisionDetection::RaySphereIntersection(const Ray& ray, const Transform& worldTransform, const SphereVolume& volume, RayCollision& collision) {
	Vector3 spherePos = worldTransform.GetWorldPosition();
	float sphereRadius = volume.GetRadius();

	// Get the direction between the ray origin and the sphere origin
	Vector3 dir = (spherePos - ray.GetPosition());// 激光原点到圆心的向量
	// Then project the sphere ’s origin onto our ray direction vector
	float sphereProj = Vector3::Dot(dir, ray.GetDirection());//点乘 圆心在激光上的投影 长度
	if (sphereProj < 0.0) {
		return false;
	}
	// Get closest point on ray line to sphere
	Vector3 point = ray.GetPosition() + (ray.GetDirection() * sphereProj);//圆心到激光的最近 点坐标

	float sphereDist = (point - spherePos).Length();//圆心到激光的最近距离

	if (sphereDist > sphereRadius) {//如果最近距离大于半径 返回 未碰撞
		return false;
	}
	float sNorm = sphereDist / sphereRadius; //最近距离/圆半径???
	sNorm = cos(DegreesToRadians(sNorm * 90.0f));//cos(数据转角度(??x 90°));

	collision.rayDistance = sphereProj - (sphereRadius * sNorm);
	collision.collidedAt = ray.GetPosition() + (ray.GetDirection() * collision.rayDistance);//碰撞点
	return true;
}

bool CollisionDetection::ObjectIntersection(GameObject* a, GameObject* b, CollisionInfo& collisionInfo) {
	//获取碰撞箱
	const CollisionVolume* volA = a->GetBoundingVolume();
	const CollisionVolume* volB = b->GetBoundingVolume();
	if (!volA || !volB) {
		return false;
	}
	collisionInfo.a = a;
	collisionInfo.b = b;
	//获得两个object的transform
	const Transform& transformA = a->GetConstTransform();
	const Transform& transformB = b->GetConstTransform();

	//比较两个object的类型
	VolumeType pairType = (VolumeType)((int)volA->type | (int)volB->type);

	if (pairType == VolumeType::AABB) {
		return AABBIntersection((AABBVolume&)*volA, transformA,
			(AABBVolume&)*volB, transformB, collisionInfo);
	}

	if (pairType == VolumeType::Sphere) {
		return SphereIntersection((SphereVolume&)*volA, transformA,
			(SphereVolume&)*volB, transformB, collisionInfo);
	}

	if (volA -> type == VolumeType::AABB &&volB -> type == VolumeType::Sphere) {
		return AABBSphereIntersection((AABBVolume&)*volA, transformA,
			(SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA -> type == VolumeType::Sphere &&volB -> type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBSphereIntersection((AABBVolume&)*volB, transformB,
			(SphereVolume&)*volA, transformA, collisionInfo);
	}

	return false;

}

bool CollisionDetection::AABBTest(const Vector3& posA, const Vector3& posB, const Vector3& halfSizeA, const Vector3& halfSizeB) {
	Vector3 delta = posB - posA;//获取盒心向量
	Vector3 totalSize = halfSizeA + halfSizeB;//获取两盒half长宽高和

	if (abs(delta.x) < totalSize.x &&
		abs(delta.y) < totalSize.y &&
		abs(delta.z) < totalSize.z) {
		return true;
	}
	return false;
}

//AABB/AABB Collisions
bool CollisionDetection::AABBIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	Vector3 boxAPos = worldTransformA.GetWorldPosition();
	Vector3 boxBPos = worldTransformB.GetWorldPosition();

	Vector3 boxASize = volumeA.GetHalfDimensions();
	Vector3 boxBSize = volumeB.GetHalfDimensions();

	bool overlap = AABBTest(boxAPos, boxBPos, boxASize, boxBSize);
	if (overlap) {
		static const Vector3 faces[6] =
		{
		Vector3(-1 , 0 , 0) , Vector3(1 , 0 , 0) ,
		Vector3(0 , -1 , 0) , Vector3(0 , 1 , 0) ,
		Vector3(0 , 0 , -1) , Vector3(0 , 0 , 1) ,
		};

		Vector3 maxA = boxAPos + boxASize;
		Vector3 minA = boxAPos - boxASize;

		Vector3 maxB = boxBPos + boxBSize;
		Vector3 minB = boxBPos - boxBSize;

		float distances[6] =
		{
		(maxB.x - minA.x) ,// distance of box ’b’ to ’left ’ of ’a ’.
		(maxA.x - minB.x) ,// distance of box ’b’ to ’right ’ of ’a ’.
		(maxB.y - minA.y) ,// distance of box ’b’ to ’bottom ’ of ’a ’.
		(maxA.y - minB.y) ,// distance of box ’b’ to ’top ’ of ’a ’.
		(maxB.z - minA.z) ,// distance of box ’b’ to ’far ’ of ’a ’.
		(maxA.z - minB.z) // distance of box ’b’ to ’near ’ of ’a ’.
		};
		float penetration = FLT_MAX;
		Vector3 bestAxis;

		for (int i = 0; i < 6; i++)
		{
			if (distances[i] < penetration) {
				penetration = distances[i];
				bestAxis = faces[i];
			}
		}
		collisionInfo.AddContactPoint(Vector3(), Vector3(),
			bestAxis, penetration);
		return true;
	}
	return false;
}

//Sphere / Sphere Collision
bool CollisionDetection::SphereIntersection(const SphereVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	//获取半径和
	float radii = volumeA.GetRadius() + volumeB.GetRadius();
	//获取两球圆心向量B→A
	Vector3 delta = worldTransformB.GetWorldPosition() - worldTransformA.GetWorldPosition();
	float deltaLength = delta.Length();//向量的模
	if (deltaLength < radii) {
		float penetration = (radii - deltaLength);//穿模量
		Vector3 normal = delta.Normalised();//圆心向量
		Vector3 localA = normal * volumeA.GetRadius();//A圆心长度的0向量
		Vector3 localB = -normal * volumeB.GetRadius();//B圆心长度的反向0向量
		collisionInfo.AddContactPoint(localA, localB, normal, penetration);
		return true;// we ’re colliding !
	}
	return false;
}

//AABB - Sphere Collision
bool CollisionDetection::AABBSphereIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	//获取AABB的half长宽高
	Vector3 boxSize = volumeA.GetHalfDimensions();
	//模心向量
	Vector3 delta = worldTransformB.GetWorldPosition() - worldTransformA.GetWorldPosition();
	//获取aabb六面里距离球最近的点
	Vector3 closestPointOnBox = Maths::Clamp(delta, -boxSize, boxSize);
	//从球到最近点的向量
	Vector3 localPoint = delta - closestPointOnBox;
	//近点向量模
	float distance = localPoint.Length();

	if (distance < volumeB.GetRadius()) {// yes , we ’re colliding !
		Vector3 collisionNormal = localPoint.Normalised();//碰撞方向0向量
		float penetration = (volumeB.GetRadius() - distance);//穿模量
		Vector3 localA = Vector3(closestPointOnBox);//AABB方向
		Vector3 localB = -collisionNormal * volumeB.GetRadius();//球的碰撞方向
		collisionInfo.AddContactPoint(localA, localB,collisionNormal, penetration);
		return true;
	}
	return false;
}

bool CollisionDetection::OBBIntersection(
	const OBBVolume& volumeA, const Transform& worldTransformA,
	const OBBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	return false;
}

//It's helper functions for generating rays from here on out:

Matrix4 GenerateInverseView(const Camera& c) {
	float pitch = c.GetPitch();
	float yaw = c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(-yaw, Vector3(0, -1, 0)) *
		Matrix4::Rotation(-pitch, Vector3(-1, 0, 0));

	return iview;
}

Matrix4 GenerateInverseProjection(float aspect, float nearPlane, float farPlane, float fov) {
	float negDepth = nearPlane - farPlane;

	float invNegDepth = negDepth / (2 * (farPlane * nearPlane));

	Matrix4 m;

	float h = 1.0f / tan(fov * PI_OVER_360);

	m.array[0] = aspect / h;
	m.array[5] = tan(fov * PI_OVER_360);
	m.array[10] = 0.0f;

	m.array[11] = invNegDepth;//// +PI_OVER_360;
	m.array[14] = -1.0f;
	m.array[15] = (0.5f / nearPlane) + (0.5f / farPlane);

	//Matrix4 temp = projection.Inverse();
	//return temp;
	return m;
}

Vector3 CollisionDetection::Unproject(const Vector3& screenPos, const Camera& cam) {
	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	float aspect = screenSize.x / screenSize.y;
	float fov = cam.GetFieldOfVision();
	float nearPlane = cam.GetNearPlane();
	float farPlane = cam.GetFarPlane();

	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(cam) * GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

	Matrix4 test1 = GenerateInverseView(cam);
	Matrix4 test2 = cam.BuildViewMatrix().Inverse();

	Matrix4 proj = cam.BuildProjectionMatrix(aspect);
	Matrix4 test4 = cam.BuildProjectionMatrix(aspect).Inverse();
	Matrix4 test3 = GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(screenPos.x / (float)screenSize.x) * 2.0f - 1.0f,
		(screenPos.y / (float)screenSize.y) * 2.0f - 1.0f,
		(screenPos.z),
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

Ray CollisionDetection::BuildRayFromMouse(const Camera& cam) {
	Vector2 screenMouse = Window::GetMouse()->GetAbsolutePosition();
	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	//We remove the y axis mouse position from height as OpenGL is 'upside down',
	//and thinks the bottom left is the origin, instead of the top left!
	Vector3 nearPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		-0.99999f
	);

	//We also don't use exactly 1.0 (the normalised 'end' of the far plane) as this
	//causes the unproject function to go a bit weird. 
	Vector3 farPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		0.99999f
	);

	Vector3 a = Unproject(nearPos, cam);
	Vector3 b = Unproject(farPos, cam);
	Vector3 c = b - a;

	c.Normalise();

	//std::cout << "Ray Direction:" << c << std::endl;

	return Ray(cam.GetPosition(), c);
}

//http://bookofhook.com/mousepick.pdf
Matrix4 CollisionDetection::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	Matrix4 m;

	float t = tan(fov * PI_OVER_360);

	float neg_depth = nearPlane - farPlane;

	const float h = 1.0f / t;

	float c = (farPlane + nearPlane) / neg_depth;
	float e = -1.0f;
	float d = 2.0f * (nearPlane * farPlane) / neg_depth;

	m.array[0] = aspect / h;
	m.array[5] = tan(fov * PI_OVER_360);

	m.array[10] = 0.0f;
	m.array[11] = 1.0f / d;

	m.array[14] = 1.0f / e;

	m.array[15] = -c / (d * e);

	return m;
}

/*
And here's how we generate an inverse view matrix. It's pretty much
an exact inversion of the BuildViewMatrix function of the Camera class!
*/
Matrix4 CollisionDetection::GenerateInverseView(const Camera& c) {
	float pitch = c.GetPitch();
	float yaw = c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(yaw, Vector3(0, 1, 0)) *
		Matrix4::Rotation(pitch, Vector3(1, 0, 0));

	return iview;
}


/*
If you've read through the Deferred Rendering tutorial you should have a pretty
good idea what this function does. It takes a 2D position, such as the mouse
position, and 'unprojects' it, to generate a 3D world space position for it.

Just as we turn a world space position into a clip space position by multiplying
it by the model, view, and projection matrices, we can turn a clip space
position back to a 3D position by multiply it by the INVERSE of the
view projection matrix (the model matrix has already been assumed to have
'transformed' the 2D point). As has been mentioned a few times, inverting a
matrix is not a nice operation, either to understand or code. But! We can cheat
the inversion process again, just like we do when we create a view matrix using
the camera.

So, to form the inverted matrix, we need the aspect and fov used to create the
projection matrix of our scene, and the camera used to form the view matrix.

*/
Vector3	CollisionDetection::UnprojectScreenPosition(Vector3 position, float aspect, float fov, const Camera& c) {
	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(c) * GenerateInverseProjection(aspect, fov, c.GetNearPlane(), c.GetFarPlane());

	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(position.x / (float)screenSize.x) * 2.0f - 1.0f,
		(position.y / (float)screenSize.y) * 2.0f - 1.0f,
		(position.z) - 1.0f,
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}