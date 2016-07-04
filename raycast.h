
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

struct RayObject
{
	RayTriangle *triangles;
	unsigned numTriangles;

	RayColor color;
};

struct RayScene
{
	RayObject *objects;
	unsigned numObjects;

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

