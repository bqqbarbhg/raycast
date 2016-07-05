#include "raycast.h"
#include <math.h>

#define EPSILON 0.00001f

#define RAY_ASSERT(x) do { if (!(x)) __debugbreak(); } while (0)

struct Vec2
{
	float x, y;
};

struct Vec3
{
	float x, y, z;
};

Vec3 vec3(float x, float y, float z)
{
	Vec3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

Vec3 vecFromRay3(const RayVec3& rv)
{
	return vec3(rv.x, rv.y, rv.z);
}

Vec3 operator+(const Vec3& a, const Vec3& b)
{
	return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3 operator-(const Vec3& a, const Vec3& b)
{
	return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3 operator*(const Vec3& a, float b)
{
	return vec3(a.x * b, a.y * b, a.z * b);
}

Vec3 operator*(float a, const Vec3& b)
{
	return vec3(a * b.x, a * b.y, a * b.z);
}

Vec3 operator*(const Vec3& a, const Vec3& b)
{
	return vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

float dot(const Vec3& a, const Vec3& b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

float lengthSquared(const Vec3& a)
{
	return a.x*a.x + a.y*a.y + a.z*a.z;
}

float length(const Vec3& a)
{
	return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
}

Vec3 normalize(const Vec3& a)
{
	return a * (1.0f / length(a));
}

Vec3 cross(const Vec3& a, const Vec3& b)
{
	return vec3(
		+ (a.y*b.z - a.z*b.y),
		- (a.x*b.z - a.z*b.x),
		+ (a.x*b.y - a.y*b.x));
}

Vec3 lerp(const Vec3& a, const Vec3& b, float t)
{
	return (1.0f - t) * a + t * b;
}

bool rayToPlane(
		float *outT,
		const Vec3& rayPos,
		const Vec3& rayDir,
		const Vec3& planeNormal,
		const float planeD)
{
	float on = dot(rayPos, planeNormal);
	float rn = dot(rayDir, planeNormal);

	if (fabs(rn) < EPSILON)
		return false;

	*outT = (planeD - on) / rn;
	return true;
}

bool rayToTri(
		float *outT,
		Vec2 *outUV,
		const Vec3& rayPos,
		const Vec3& rayDir,
		const Vec3& a,
		const Vec3& b,
		const Vec3& c)
{
	Vec3 e0 = b - a;
	Vec3 e1 = c - a;
	Vec3 normal = cross(e0, e1);
	float d = dot(normal, a);
	float t;

	if (!rayToPlane(&t, rayPos, rayDir, normal, d))
		return false;

	if (t < 0.0f)
		return false;

	Vec3 pos = rayPos + rayDir * t;
	Vec3 p = pos - a;

	float e00 = dot(e0, e0);
	float e01 = dot(e0, e1);
	float e11 = dot(e1, e1);

	float p0 = dot(p, e0);
	float p1 = dot(p, e1);

	float det = e00*e11 - e01*e01;

	float u = (p0*e11 - p1*e01) / det;
	float v = (p1*e00 - p0*e01) / det;

	if (u < -EPSILON || v < -EPSILON || u + v > 1.0f + EPSILON)
		return false;

	*outT = t;
	outUV->x = u;
	outUV->y = v;

	return true;
}

bool rayToSphere(
	float *outNear,
	float *outFar,
	const Vec3& rayPos,
	const Vec3& rayDir,
	const Vec3& origin,
	float radius)
{
	Vec3 diff = rayPos - origin;

	RAY_ASSERT(fabs(length(rayDir) - 1.0f) < EPSILON);

	float ud = dot(diff, rayDir);
	float uu = dot(diff, diff);

	float discriminant = ud*ud - uu + radius*radius;
	if (discriminant < 0.0f)
		return false;

	float disc = sqrtf(discriminant);

	*outNear = -ud - disc;
	*outFar = -ud + disc;
	return true;
}

extern float hackT;

Vec3 castRay(const RayScene *scene, const Vec3& rayPos, const Vec3& rayDir, int nBounce)
{
	float nearestDist = 10000000.0f;
	Vec3 nearestNormal;
	RayMaterial *nearestMaterial = 0;

	for (unsigned oi = 0; oi < scene->numObjects; oi++)
	{
		RayObject *obj = &scene->objects[oi];
		for (unsigned ti = 0; ti < obj->numTriangles; ti++)
		{
			float t;
			Vec2 uv;

			bool hit = rayToTri(
				&t, &uv,
				rayPos,
				rayDir,
				vecFromRay3(obj->triangles[ti].a),
				vecFromRay3(obj->triangles[ti].b),
				vecFromRay3(obj->triangles[ti].c));

			if (hit && t < nearestDist)
			{
				nearestDist = t;

				RayTriangle *tri = &obj->triangles[ti];
				Vec3 a = vecFromRay3(tri->a);
				Vec3 b = vecFromRay3(tri->b);
				Vec3 c = vecFromRay3(tri->c);
				Vec3 e0 = b - a;
				Vec3 e1 = c - a;
				nearestNormal = normalize(cross(e0, e1));

				nearestMaterial = obj->material;
			}
		}
	}

	for (unsigned si = 0; si < scene->numSpheres; si++)
	{
		RaySphere *sphere = &scene->spheres[si];
		float tNear, tFar;

		bool hit = rayToSphere(
			&tNear,
			&tFar,
			rayPos,
			rayDir,
			vecFromRay3(sphere->origin),
			sphere->radius);

		if (hit && tNear >= 0 && tNear < nearestDist)
		{
			nearestDist = tNear;

			Vec3 pos = rayPos + rayDir * tNear;
			nearestNormal = normalize(pos - vecFromRay3(sphere->origin));

			nearestMaterial = sphere->material;
		}
	}

	// HACK floor
	{
		float t;
		bool hit = rayToPlane(
			&t,
			rayPos,
			rayDir,
			vec3(0.0f, 1.0f, 0.0f),
			-5.0f);

		static RayMaterial mats[] = {
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0xff / 255.0f, 0x78 / 255.0f, 0x99 / 255.0f, 0.0f },
		};

		if (hit && t >= 0.0f && t < nearestDist)
		{
			nearestDist = t;
			nearestNormal = vec3(0.0f, 1.0f, 0.0f);
			Vec3 p = (rayPos + rayDir * t) * 0.5f;
			int cell = ((int)floorf(p.x) ^ (int)floorf(p.z + hackT)) & 1;
			nearestMaterial = &mats[cell];
		}
	}

	if (nearestMaterial != 0)
	{
		float light = dot(nearestNormal, vec3(0.0f, 1.0f, 0.0f)) * 0.5f + 0.5f;
		Vec3 col = vec3(light, light, light) * vec3(nearestMaterial->color.r, nearestMaterial->color.g, nearestMaterial->color.b);

		if (nearestMaterial->reflectivity > EPSILON && nBounce < 3)
		{
			Vec3 pos = rayPos + rayDir * nearestDist;
			float dt = dot(rayDir, nearestNormal);
			Vec3 reflect = normalize(rayDir - 2.0f * dt * nearestNormal);
			Vec3 bounce = castRay(scene, pos + reflect * 0.0001f, reflect, nBounce + 1);
			col = lerp(col, bounce, nearestMaterial->reflectivity);
		}

		return col;
	}
	else
	{
		return vec3(0x00 / 255.0f, 0x80 / 255.0f, 0x80 / 255.0f);
	}
}

void raycast(RayFramebuffer *framebuffer, const RayScene *scene)
{
	float aspectRatio = (float)framebuffer->width / (float)framebuffer->height;
	float planeDist = 1.0f;
	float planeHalfWidth = planeDist * sinf(scene->camera.fov / 2.0f);
	float planeHalfHeight = planeHalfWidth / aspectRatio;

	Vec3 dir = vecFromRay3(scene->camera.dir);
	Vec3 up = vecFromRay3(scene->camera.up);
	Vec3 right = cross(up, dir);

	Vec3 topLeft = planeDist * dir - planeHalfWidth * right + planeHalfHeight * up;
	Vec3 stepX = right * (planeHalfWidth * 2.0f / (float)framebuffer->width);
	Vec3 stepY = up * (-planeHalfHeight * 2.0f / (float)framebuffer->height);

	for (unsigned y = 0; y < framebuffer->height; y++)
	{
		for (unsigned x = 0; x < framebuffer->width; x++)
		{
			Vec3 rayPos = vecFromRay3(scene->camera.pos);
			Vec3 rayDir = normalize(topLeft + stepX * (float)x + stepY * (float)y);

			Vec3 col = castRay(scene, rayPos, rayDir, 0);

			unsigned pi = (x + y * framebuffer->width) * 3;
			framebuffer->pixelData[pi + 0] = (unsigned char)(col.x * 255.0f);
			framebuffer->pixelData[pi + 1] = (unsigned char)(col.y * 255.0f);
			framebuffer->pixelData[pi + 2] = (unsigned char)(col.z * 255.0f);
		}
	}
}

