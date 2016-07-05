#include "raycast.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

#define _USE_MATH_DEFINES
#include <math.h>

struct Model
{
	RayTriangle *triangles;
	unsigned numTriangles;
};

int parseOBJ(Model *model, FILE *file)
{
	RayVec3 *tempVerts = NULL;
	unsigned tempVertCount = 0, tempVertCap = 0;

	RayTriangle *tempTris = NULL;
	unsigned tempTriCount = 0, tempTriCap = 0;

	char line[1024];
	while (fgets(line, sizeof(line), file))
	{
		{
			RayVec3 vec;
			if (sscanf(line, "v %f %f %f", &vec.x, &vec.y, &vec.z) == 3)
			{
				if (tempVertCount >= tempVertCap)
				{
					tempVertCap = tempVertCap ? tempVertCap * 2 : 16;
					tempVerts = (RayVec3*)realloc(tempVerts, tempVertCap * sizeof(RayVec3));
				}

				tempVerts[tempVertCount++] = vec;
			}
		}

		{
			unsigned a, b, c;
			if (sscanf(line, "f %u %u %u", &a, &b, &c) == 3)
			{
				if (tempTriCount >= tempTriCap)
				{
					tempTriCap = tempTriCap ? tempTriCap * 2 : 16;
					tempTris = (RayTriangle*)realloc(tempTris, tempTriCap * sizeof(RayTriangle));
				}

				RayTriangle *tri = &tempTris[tempTriCount++];
				tri->a = tempVerts[a - 1];
				tri->b = tempVerts[b - 1];
				tri->c = tempVerts[c - 1];
			}
		}
	}

	free(tempVerts);

	model->triangles = tempTris;
	model->numTriangles = tempTriCount;
	return 1;
}

void freeModel(Model *model)
{
	free(model->triangles);
}

float hackT = 0.0f;

int main(int argc, char **argv)
{
	// -- Object import
	Model monkey;
	FILE *monkeyFile = fopen("monkey.obj", "r");
	if (!monkeyFile)
	{
		fprintf(stderr, "Failed to open model\n");
		return 1;
	}

	if (!parseOBJ(&monkey, monkeyFile))
	{
		fprintf(stderr, "Failed to parse OBJ\n");
		return 2;
	}

	if (argc > 2)
		hackT = atof(argv[2]);

	fclose(monkeyFile);

	// -- Scene setup
	RayObject objects[1];
	RayMaterial mat = { 1.0f, 1.0f, 1.0f, 0.8f };

#if 0
	RayTriangle tt = {
		{ -5.0f,  0.0f,  0.0f },
		{  5.0f,  0.0f,  0.0f },
		{  0.0f,  5.0f,  0.0f },
	};
	objects[0].triangles = &tt;
	objects[0].numTriangles = 1;
#else
	objects[0].triangles = monkey.triangles;
	objects[0].numTriangles = monkey.numTriangles;
#endif
	objects[0].material = &mat;

	RayPointLight pointLights[1];
	pointLights[0].pos.x = 3.0f;
	pointLights[0].pos.y = 3.0f;
	pointLights[0].pos.z = 3.0f;
	pointLights[0].attenuation = 10.0f;
	pointLights[0].color.r = 1.0f;
	pointLights[0].color.g = 1.0f;
	pointLights[0].color.b = 1.0f;

	RayScene scene;

	RaySphere spheres[] = {
		{
			{ 0.0f, 3.0f, -3.0f },
			3.0f,
			&mat,
		},
	};
	scene.spheres = spheres;
	scene.numSpheres = sizeof(spheres) / sizeof(*spheres);

	scene.objects = objects;
	scene.numObjects = sizeof(objects) / sizeof(*objects);

	scene.pointLights = pointLights;
	scene.numPointLights = sizeof(pointLights) / sizeof(*pointLights);

	scene.camera.pos.x = 0.0f;
	scene.camera.pos.y = 0.0f;
	scene.camera.pos.z = 5.0f;
	scene.camera.dir.x = 0.0f;
	scene.camera.dir.y = 0.0f;
	scene.camera.dir.z = -1.0f;
	scene.camera.up.x = 0.0f;
	scene.camera.up.y = 1.0f;
	scene.camera.up.z = 0.0f;
	scene.camera.fov = 90.0f * ((float)M_PI / 180.0f);

	// -- Raycast
	RayFramebuffer framebuffer;
	framebuffer.width = 400;
	framebuffer.height = 300;
	framebuffer.pixelData = (unsigned char*)malloc(framebuffer.width * framebuffer.height * 3);

	for (unsigned i = 0; i < framebuffer.width * framebuffer.height * 3; i += 3)
	{
		framebuffer.pixelData[i + 0] = 0x64;
		framebuffer.pixelData[i + 1] = 0x95;
		framebuffer.pixelData[i + 2] = 0xED;
	}

	raycast(&framebuffer, &scene);

	const char *file = "image.png";
	if (argc > 1)
		file = argv[1];

	// -- Image save
	int ret = stbi_write_png(
		file,
		(int)framebuffer.width,
		(int)framebuffer.height,
		3,
		framebuffer.pixelData,
		framebuffer.width * 3);

	if (ret == 0)
	{
		fprintf(stderr, "Failed to write output!\n");
	}

	free(framebuffer.pixelData);
	freeModel(&monkey);

	return 0;
}

