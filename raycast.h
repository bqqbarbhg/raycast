#ifndef INCLUDED_RAYCAST_H
#define INCLUDED_RAYCAST_H

struct RayVec3
{
	float x, y, z;
};

struct RayColor
{
	float r, g, b;
};

struct RayTriangle
{
	RayVec3 a, b, c;
};

struct RayPointLight
{
	RayVec3 pos;
	float attenuation;
	RayColor color;
};

struct RayCamera
{
	RayVec3 pos;
	RayVec3 dir;
	RayVec3 up;
	float fov;
};

struct RayMaterial
{
	RayColor color;
	float reflectivity;
};

struct RayObject
{
	RayTriangle *triangles;
	unsigned numTriangles;

	RayMaterial *material;
};

struct RaySphere
{
	RayVec3 origin;
	float radius;

	RayMaterial *material;
};

struct RayScene
{
	RayObject *objects;
	unsigned numObjects;

	RaySphere *spheres;
	unsigned numSpheres;

	RayPointLight *pointLights;
	unsigned numPointLights;

	RayCamera camera;
};

struct RayFramebuffer
{
	unsigned char *pixelData;
	unsigned width, height;
};

void raycast(RayFramebuffer *framebuffer, const RayScene *scene);

#endif
