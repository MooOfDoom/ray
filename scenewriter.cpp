/*
 * Ray
 *
 * Usage: ray <scene.scn> <render.tga>
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define EPSILON 0.00001f

#include "types.h"
#include "util.h"
#include "memory.h"
#include "string.h"
#include "vector.h"
#include "random.h"
#include "scene.h"
#include "tga.h"

#include <stdarg.h>

function void
PrintToBuffer(buffer* Buffer, const char* FormatStr, ...)
{
	va_list VAList;
	va_start(VAList, FormatStr);
	Buffer->Count += vsprintf((char*)Buffer->Data + Buffer->Count, FormatStr, VAList);
	va_end(VAList);
}

function v3
RandomUnilateralV3(random_sequence* RNG)
{
	v3 Result = {RandomUnilateral(RNG), RandomUnilateral(RNG), RandomUnilateral(RNG)};
	return Result;
}

function v3
RandomBilateralV3(random_sequence* RNG)
{
	v3 Result = {RandomBilateral(RNG), RandomBilateral(RNG), RandomBilateral(RNG)};
	return Result;
}

function v3
RandomV3AvoidingPoint(random_sequence* RNG, v3 Point, f32 MinDist, f32 MaxCoord)
{
	v3 Result = MaxCoord*RandomBilateralV3(RNG);
	f32 MinDistSq = MinDist*MinDist;
	while (LengthSq(Result - Point) < MinDistSq)
	{
		Result = MaxCoord*RandomBilateralV3(RNG);
	}
	return Result;
}

function b32
WriteScene(const char* FileName, s32 NumObjects, f32 SceneSize, s64 Seed, memory_arena* Arena)
{
	b32 Success = true;
	FILE* DestFile = fopen(FileName, "wb");
	
	if (DestFile)
	{
		temporary_memory Temp = BeginTemporaryMemory(Arena);
		s64 OldAlignment = Arena->Alignment;
		SetAlignment(Arena, 1);
		buffer TotalBuffer = {};
		TotalBuffer.Count = Arena->Capacity - Arena->Allocated;
		TotalBuffer.Data = PushArray(Arena, TotalBuffer.Count, u8);
		
		buffer SceneBuffer = {0, TotalBuffer.Data};
		
		random_sequence RNG = SeedRandom(Seed);
		
		v3 CameraOrigin = {0, -0.75f*SceneSize, 0};
		v3 CameraLookAt = {0, 0, 0};
		f32 MaxSphereRadius = 5.0f;
		f32 MinDistFromCamera = 1.5f;
		if (CameraOrigin.Y + MinDistFromCamera + MaxSphereRadius > SceneSize)
		{
			MaxSphereRadius = SceneSize - CameraOrigin.Y - MinDistFromCamera;
		}
		
		const char* Textures[] =
		{
			"data/checkerboard.tga",
			"data/bricks.tga",
		};
		
		s32 NumTextures = ArrayCount(Textures);
		PrintToBuffer(&SceneBuffer,
			"Textures\n"
			"{\n");
		for (s32 Index = 0; Index < NumTextures; ++Index)
		{
			PrintToBuffer(&SceneBuffer,
				"\t%d = \"%s\",\n",
				Index + 1, Textures[Index]);
		}
		PrintToBuffer(&SceneBuffer, "}\n\n");
		
		PrintToBuffer(&SceneBuffer,
			"Camera (Origin = (%.2f, %.2f, %.2f), DistToSurface = 1, SurfaceWidth = 1, SurfaceHeight = 1)\n"
			"{\n"
			"\tLookAt = (%.2f, %.2f, %.2f),\n"
			"\tSkyColor = (1, 1, 1),\n"
			"}\n\n",
			CameraOrigin.X, CameraOrigin.Y, CameraOrigin.Z,
			CameraLookAt.X, CameraLookAt.Y, CameraLookAt.Z);
		
		for (s32 Index = 0; Index < NumObjects; ++Index)
		{
			object_type ObjectType;
			f32 RandType = RandomUnilateral(&RNG);
			if (RandType < 1.0f/3.0f)
			{
				ObjectType = Obj_Sphere;
			}
			else if (RandType < 2.0f/3.0f)
			{
				ObjectType = Obj_Triangle;
			}
			else
			{
				ObjectType = Obj_Parallelogram;
			}
			
			switch (ObjectType)
			{
				case Obj_Sphere:
				{
					f32 Radius = 0.2f + (MaxSphereRadius - 0.2f)*RandomUnilateral(&RNG);
					v3 Center = RandomV3AvoidingPoint(&RNG, CameraOrigin, MinDistFromCamera + Radius, SceneSize);
					color Color = RandomUnilateralV3(&RNG);
					f32 Glossy = RandomUnilateral(&RNG);
					PrintToBuffer(&SceneBuffer,
						"Sphere (Center = (%.2f, %.2f, %.2f), Radius = %.2f)\n"
						"{\n"
						"\tColor = (%.2f, %.2f, %.2f),\n"
						"\tGlossy = %.2f,\n"
						"}\n\n",
						Center.X, Center.Y, Center.Z, Radius,
						Color.R, Color.G, Color.B, Glossy);
				} break;
				
				case Obj_Triangle:
				{
					f32 Radius = 0.2f + (MaxSphereRadius - 0.2f)*RandomUnilateral(&RNG);
					v3 Center = RandomV3AvoidingPoint(&RNG, CameraOrigin, MinDistFromCamera + Radius, SceneSize);
					v3 V0 = Center + Radius*RandomBilateralV3(&RNG);
					v3 V1 = Center + Radius*RandomBilateralV3(&RNG);
					v3 V2 = Center + Radius*RandomBilateralV3(&RNG);
					f32 Glossy = RandomUnilateral(&RNG);
					f32 RandTexture = RandomUnilateral(&RNG);
					if (RandTexture < 0.5f)
					{
						v3 Color = RandomUnilateralV3(&RNG);
						PrintToBuffer(&SceneBuffer,
							"Triangle (Vertices = ((%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f)))\n"
							"{\n"
							"\tColor = (%.2f, %.2f, %.2f),\n"
							"\tGlossy = %.2f,\n"
							"}\n\n",
							V0.X, V0.Y, V0.Z, V1.X, V1.Y, V1.Z, V2.X, V2.Y, V2.Z,
							Color.R, Color.G, Color.B, Glossy);
					}
					else
					{
						u32 Texture = (NextRandom(&RNG) % NumTextures) + 1;
						PrintToBuffer(&SceneBuffer,
							"Triangle (Vertices = ((%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f)))\n"
							"{\n"
							"\tTexture = %d,\n"
							"\tUVMap = ((0, 0), (1, 0), (0, 1)),\n"
							"\tGlossy = %.2f,\n"
							"}\n\n",
							V0.X, V0.Y, V0.Z, V1.X, V1.Y, V1.Z, V2.X, V2.Y, V2.Z,
							Texture, 0.5f*Glossy);
					}
				} break;
				
				case Obj_Parallelogram:
				{
					f32 Radius = 0.2f + (MaxSphereRadius - 0.2f)*RandomUnilateral(&RNG);
					v3 Origin = RandomV3AvoidingPoint(&RNG, CameraOrigin, MinDistFromCamera + Radius, SceneSize);
					v3 XAxis = 0.5f*Radius*RandomBilateralV3(&RNG);
					v3 YAxis = 0.5f*Radius*RandomBilateralV3(&RNG);
					f32 Glossy = RandomUnilateral(&RNG);
					f32 RandTexture = RandomUnilateral(&RNG);
					if (RandTexture < 0.5f)
					{
						v3 Color = RandomUnilateralV3(&RNG);
						PrintToBuffer(&SceneBuffer,
							"Parallelogram (Origin = (%.2f, %.2f, %.2f), Axes = ((%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f)))\n"
							"{\n"
							"\tColor = (%.2f, %.2f, %.2f),\n"
							"\tGlossy = %.2f,\n"
							"}\n\n",
							Origin.X, Origin.Y, Origin.Z, XAxis.X, XAxis.Y, XAxis.Z, YAxis.X, YAxis.Y, YAxis.Z, 
							Color.R, Color.G, Color.B, Glossy);
					}
					else
					{
						u32 Texture = (NextRandom(&RNG) % NumTextures) + 1;
						PrintToBuffer(&SceneBuffer,
							"Parallelogram (Origin = (%.2f, %.2f, %.2f), Axes = ((%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f)))\n"
							"{\n"
							"\tTexture = %d,\n"
							"\tUVMap = ((0, 0), (1, 0), (0, 1)),\n"
							"\tGlossy = %.2f,\n"
							"}\n\n",
							Origin.X, Origin.Y, Origin.Z, XAxis.X, XAxis.Y, XAxis.Z, YAxis.X, YAxis.Y, YAxis.Z, 
							Texture, 0.5f*Glossy);
					}
				} break;
				
				default:
				{
					// Does not occur
				} break;
			}
		}
		
		Success = (fwrite(SceneBuffer.Data, SceneBuffer.Count, 1, DestFile) == 1);
			
		EndTemporaryMemory(Temp);
		SetAlignment(Arena, OldAlignment);
		fclose(DestFile);
	}
	
	return Success;
}

int
main(int ArgCount, char** Args)
{
	b32 Success = true;
	
	if (ArgCount == 4)
	{
		memory_arena Arena = MakeArena(1024*1024*1024, 16);
		Success = WriteScene(Args[1], strtol(Args[2], 0, 10), strtof(Args[3], 0), 4815162342ull, &Arena);
	}
	else if (ArgCount == 5)
	{
		s64 Seed = strtol(Args[4], 0, 10);
		if (Seed)
		{
			memory_arena Arena = MakeArena(1024*1024*1024, 16);
			Success = WriteScene(Args[1], strtol(Args[2], 0, 10), strtof(Args[3], 0), Seed, &Arena);
		}
		else
		{
			Success = false;
			fprintf(stderr, "Invalid argument given as random seed: %s\n", Args[4]);
		}
	}
	else
	{
		Success = false;
		fprintf(stderr, "Usage: %s <destfile.scn> <num objects> <scene size> [<rng seed>]\n", Args[0]);
	}
	
	return !Success;
}
