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

#include <omp.h>
#include <chrono>

#include "parser.cpp"

typedef struct ray_trace_stats
{
	s64 RaysCast;
	s64 ObjectsChecked;
	s64 SamplesComputed;
	s64 Padding[5];
} ray_trace_stats;

function ray_hit
RayIntersectScene(v3 RayOrigin, v3 RayDir, scene* Scene)
{
	ray_hit RayHit = {};
	for (s32 Index = 0; Index < Scene->ObjectCount; ++Index)
	{
		object* Object = Scene->Objects + Index;
		switch (Object->Type)
		{
			case Obj_Plane:
			{
				f32 RayDDotNormal = Dot(RayDir, Object->Plane.Normal);
				if (Abs(RayDDotNormal) > EPSILON)
				{
					f32 Hit = (Object->Plane.Displacement - Dot(RayOrigin, Object->Plane.Normal)) / RayDDotNormal;
					if (Hit > EPSILON && (RayHit.Dist == 0 || Hit < RayHit.Dist))
					{
						// Record a hit
						RayHit.Dist = Hit;
						RayHit.Object = Object;
						RayHit.Normal = (RayDDotNormal < 0 ? Object->Plane.Normal : -Object->Plane.Normal);
					}
				}
			} break;
			
			case Obj_Sphere:
			{
				v3 FromCenter = RayOrigin - Object->Sphere.Center;
				f32 RayDDotFromCenter = Dot(RayDir, FromCenter);
				f32 Discriminant = RayDDotFromCenter*RayDDotFromCenter - LengthSq(FromCenter) + Object->Sphere.Radius*Object->Sphere.Radius;
				if (Discriminant > EPSILON)
				{
					f32 RootDisc = sqrtf(Discriminant);
					// b32 Inside = false;
					f32 Hit = -RayDDotFromCenter - RootDisc;
					if (Hit <= 0)
					{
						Hit = -RayDDotFromCenter + RootDisc;
						// Inside = true;
					}
					if (Hit > EPSILON && (RayHit.Dist == 0 || Hit < RayHit.Dist))
					{
						// Record a hit
						v3 RelHitPoint = RayOrigin + RayDir*Hit - Object->Sphere.Center;
						v3 Normal = NormOrZero(RelHitPoint);
						RayHit.Dist = Hit;
						RayHit.Object = Object;
						RayHit.Normal = Normal; //(Inside ? -Normal : Normal);
					}
				}
			} break;
			case Obj_Triangle:
			{
				v3 AB = Object->Triangle.Vertex[1] - Object->Triangle.Vertex[0];
				v3 AC = Object->Triangle.Vertex[2] - Object->Triangle.Vertex[0];
				v3 Normal = NormOrZero(Cross(AB, AC));
				if (Normal != (v3){0})
				{
					f32 RayDDotNormal = Dot(RayDir, Normal);
					if (Abs(RayDDotNormal) > EPSILON)
					{
						f32 Hit = Dot(Object->Triangle.Vertex[0] - RayOrigin, Normal) / RayDDotNormal;
						if (Hit > EPSILON && (RayHit.Dist == 0 || Hit < RayHit.Dist))
						{
							v3 HitP = RayOrigin + RayDir*Hit;
							v3 AP = HitP - Object->Triangle.Vertex[0];
							f32 ABDotAC = Dot(AB, AC);
							v3 ABPerp = AC - AB*(ABDotAC/LengthSq(AB));
							f32 V = Dot(AP, ABPerp)/LengthSq(ABPerp);
							if (V > 0)
							{
								v3 ACPerp = AB - AC*(ABDotAC/LengthSq(AC));
								f32 U = Dot(AP, ACPerp)/LengthSq(ACPerp);
								if (U > 0 && U + V < 1.0f)
								{
									// Record a hit
									RayHit.Dist = Hit;
									RayHit.Object = Object;
									RayHit.Normal = (RayDDotNormal < 0 ? Normal : -Normal);
									RayHit.UV = (uv){U, V};
								}
							}
						}
					}
				}
			} break;
			
			case Obj_Parallelogram:
			{
				v3 Normal = NormOrZero(Cross(Object->Parallelogram.XAxis, Object->Parallelogram.YAxis));
				if (Normal != (v3){0})
				{
					f32 RayDDotNormal = Dot(RayDir, Normal);
					if (Abs(RayDDotNormal) > EPSILON)
					{
						f32 Hit = Dot(Object->Parallelogram.Origin - RayOrigin, Normal) / RayDDotNormal;
						if (Hit > EPSILON && (RayHit.Dist == 0 || Hit < RayHit.Dist))
						{
							v3 HitP = RayOrigin + RayDir*Hit;
							v3 AP = HitP - Object->Parallelogram.Origin;
							f32 ABDotAC = Dot(Object->Parallelogram.XAxis, Object->Parallelogram.YAxis);
							v3 ABPerp = Object->Parallelogram.YAxis - Object->Parallelogram.XAxis*(ABDotAC/LengthSq(Object->Parallelogram.XAxis));
							f32 V = Dot(AP, ABPerp)/LengthSq(ABPerp);
							if (V > 0)
							{
								v3 ACPerp = Object->Parallelogram.XAxis - Object->Parallelogram.YAxis*(ABDotAC/LengthSq(Object->Parallelogram.YAxis));
								f32 U = Dot(AP, ACPerp)/LengthSq(ACPerp);
								if (U > 0 && U < 1.0f && V < 1.0f)
								{
									// Record a hit
									RayHit.Dist = Hit;
									RayHit.Object = Object;
									RayHit.Normal = (RayDDotNormal < 0 ? Normal : -Normal);
									RayHit.UV = (uv){U, V};
								}
							}
						}
					}
				}
			} break;
			
			default:
			{
				fprintf(stderr, "Hit unknown object!\n");
			} break;
		}
	}
	
	return RayHit;
}

function void
RayTrace(scene* Scene, surface* Surface, s32 SamplesPerPixel, s32 MaxBounces, memory_arena* Arena, b32 DebugOn)
{
	b32 HitTexture = false; // Debug purposes
	s32 HitTranslucency = 0; // Debug purposes
	
	f32 PixelWidth = Scene->Camera.SurfaceWidth / (f32)Surface->Width;
	f32 PixelHeight = Scene->Camera.SurfaceHeight / (f32)Surface->Height;
	f32 SampleWidth = PixelWidth / (f32) SamplesPerPixel;
	f32 SampleHeight = PixelHeight / (f32) SamplesPerPixel;
	f32 SampleWeight = 1.0f / (SamplesPerPixel*SamplesPerPixel);
	v3 SurfaceOrigin = Scene->Camera.XAxis*(-0.5f*Scene->Camera.SurfaceWidth + 0.5f*SampleWidth) + Scene->Camera.YAxis*(-0.5f*Scene->Camera.SurfaceHeight + 0.5f*SampleHeight) - Scene->Camera.ZAxis*Scene->Camera.DistToSurface;
	
	SetAlignment(Arena, 64); // Make sure to align to cache lines to avoid false sharing
	ray_trace_stats* AllStats = PushArray(Arena, 0, ray_trace_stats); // Just find the location of the start, reserve the right number later ;)
	s32 NumThreads;
	#pragma omp parallel
	{
		if (omp_get_thread_num() == 0)
		{
			NumThreads = omp_get_num_threads();
			printf("%d Threads...\n", NumThreads);
		}
		
		ray_trace_stats Stats = {};
		#pragma omp for
		for (s32 Y = 0; Y < Surface->Height; ++Y)
		{
			random_sequence RNG = SeedRandom(4815162342ull*(Y + 1) + 1123581321ull); // Make sure each thread has own random sequence. This keeps it deterministic
			for (s32 X = 0; X < Surface->Width; ++X)
			{
				color PixelColor = {0};
				for (s32 J = 0; J < SamplesPerPixel; ++J)
				{
					for (s32 I = 0; I < SamplesPerPixel; ++I)
					{
						++Stats.SamplesComputed;
						b32 Debug = (X == 85 && Y == 180 && I == 0 && J == 0);
						
						f32 U = (f32)X*PixelWidth + (f32)I*SampleWidth;
						f32 V = (f32)Y*PixelHeight + (f32)J*SampleHeight;
						v3 SurfaceX = Scene->Camera.XAxis * U;
						v3 SurfaceY = Scene->Camera.YAxis * V;
						
						v3 RayOrigin = Scene->Camera.Origin;
						v3 RayDir = NormOrZero(SurfaceOrigin + SurfaceX + SurfaceY);
						
						color SampleColor = {1.0f, 1.0f, 1.0f};
						for (s32 Bounce = 0; Bounce < MaxBounces; ++Bounce)
						{
							++Stats.RaysCast;
							
							if (DebugOn && Debug)
							{
								printf("X(%d) Y(%d) Bounce(%d)\n",
									X, Y, Bounce);
								printf("RayOrigin(%.2f,%.2f,%.2f) RayDir(%.2f,%.2f,%.2f)\n",
									RayOrigin.X, RayOrigin.Y, RayOrigin.Z, RayDir.X, RayDir.Y, RayDir.Z);
							}
							
							ray_hit Hit = RayIntersectScene(RayOrigin, RayDir, Scene);
							Stats.ObjectsChecked += Scene->ObjectCount;
							if (Hit.Dist > 0)
							{
								RayOrigin = RayOrigin + RayDir*Hit.Dist;
								f32 RayDDotNormal = Dot(RayDir, Hit.Normal);
								
								if (DebugOn && Debug)
								{
									printf("Debug hit!\n");
									printf("ObjectType(%d)\n",
										Hit.Object->Type);
									printf("HitAt(%.2f,%.2f,%.2f) Normal(%.2f,%.2f,%.2f)\n",
										RayOrigin.X, RayOrigin.Y, RayOrigin.Z,
										Hit.Normal.X, Hit.Normal.Y, Hit.Normal.Z);
									printf("RayDDotNormal(%.2f)\n",
										RayDDotNormal);
								}
								f32 Falloff = 1.0f;
								
								// Bounce direction
								f32 Random = RandomUnilateral(&RNG);
								if (Random < Hit.Object->Translucency)
								{
									// Pass through the object
									v3 ParallelComponent = RayDir - Hit.Normal*RayDDotNormal;
									f32 RefractionCoeff = 1.0f + Hit.Object->Refraction;
									if (RayDDotNormal < 0)
									{
										RefractionCoeff = 1.0f / RefractionCoeff;
									}
									v3 OldRayDir = RayDir;
									RayDir = NormOrZero(RayDir - (1.0f - RefractionCoeff)*ParallelComponent);
									if (DebugOn && HitTranslucency < 10)
									{
										++HitTranslucency;
										Debug = true;
										printf("Translucency hit! HitAt(%.2f,%.2f,%.2f)\n",
										RayOrigin.X, RayOrigin.Y, RayOrigin.Z);
										printf("Bounce(%d) ObjectType(%d) Translucency(%.2f) RefractionCoeff(%.2f)\n",
											Bounce, Hit.Object->Type, Hit.Object->Translucency, RefractionCoeff);
										printf("Normal(%.2f,%.2f,%.2f) OldRayDir(%.2f,%.2f,%.2f) NewRayDir(%.2f,%.2f,%.2f)\n",
											Hit.Normal.X, Hit.Normal.Y, Hit.Normal.Z,
											OldRayDir.X, OldRayDir.Y, OldRayDir.Z,
											RayDir.X, RayDir.Y, RayDir.Z);
										printf("RayDDotNormal(%.2f)\n",
											RayDDotNormal);
									}
								}
								else
								{
									// Bounce off
									// Reflect like a mirror
									v3 Reflection = RayDir - Hit.Normal*(2.0f*RayDDotNormal);
									// Reflect randomly
									// v3 RandomBounce = NormOrZero(Hit.Normal + RandomUnitBallV3(&RNG));
									// if (RayDDotNormal > 0)
									// {
									// 	RandomBounce = -RandomBounce;
									// }
									RayDir = Reflection; // NormOrDefault(Lerp(RandomBounce, Hit.Object->Glossy, Reflection), Hit.Normal);
									Falloff = Abs(Dot(RayDir, Hit.Normal));
								}
								RayOrigin = RayOrigin + EPSILON*RayDir; // Try to move a little away from the surface to "de-bounce"
								
								if (Hit.Object->Texture.Index > 0)
								{
									surface* Texture = Scene->Textures + Hit.Object->Texture.Index;
									v2 SampleUV = Lerp(Hit.Object->UVMap.VertexUV[0], Hit.UV.U, Hit.Object->UVMap.VertexUV[1]) +
										Lerp(Hit.Object->UVMap.VertexUV[0], Hit.UV.V, Hit.Object->UVMap.VertexUV[2]);
									s32 SampleX = (s32)(SampleUV.U*(f32)Texture->Width) % Texture->Width;
									s32 SampleY = (s32)(SampleUV.V*(f32)Texture->Height) % Texture->Height;
									color TextureColor = Texture->Pixels[SampleY*Texture->Width + SampleX];
									
									if (DebugOn && Debug && !HitTexture && Bounce == 0)
									{
										HitTexture = true;
										printf("X(%d) Y(%d)\n",
											X, Y);
										printf("ObjectType(%d) Texture(%d)\n",
											Hit.Object->Type, Hit.Object->Texture.Index);
										printf("SampleUV(%.2f,%.2f) SampleX(%d) SampleY(%d)\n",
											SampleUV.U, SampleUV.V, SampleX, SampleY);
									}
									
									// TextureColor = (color){Hit.UV.U < 0.5f ? 0 : 1.0f, Hit.UV.V < 0.5f ? 0 : 1.0f, 0};
									
									SampleColor.R *= TextureColor.R*Falloff;
									SampleColor.G *= TextureColor.G*Falloff;
									SampleColor.B *= TextureColor.B*Falloff;
								}
								else
								{
									SampleColor.R *= Hit.Object->Color.R*Falloff;
									SampleColor.G *= Hit.Object->Color.G*Falloff;
									SampleColor.B *= Hit.Object->Color.B*Falloff;
								}
								
								if (DebugOn && Debug)
								{
									printf("Hit.Object(%d) Hit.Normal(%.2f,%.2f,%.2f) Falloff(%.2f)\n",
										Hit.Object->Type, Hit.Normal.X, Hit.Normal.Y, Hit.Normal.Z, Falloff);
								}
							}
							else
							{
								SampleColor.R *= Scene->SkyColor.R;
								SampleColor.G *= Scene->SkyColor.G;
								SampleColor.B *= Scene->SkyColor.B;
								break;
							}
						}
						
						PixelColor.R += SampleColor.R;
						PixelColor.G += SampleColor.G;
						PixelColor.B += SampleColor.B;
					}
				}
				Surface->Pixels[Y*Surface->Width + X] = PixelColor*SampleWeight;
			}
			s32 ThreadNum = omp_get_thread_num();
			AllStats[ThreadNum] = Stats;
		}
	}
	PushArray(Arena, NumThreads, ray_trace_stats); // Actually reserve the space :)
	
	ray_trace_stats OverallStats = {};
	for (s32 Index = 0; Index < NumThreads; ++Index)
	{
		printf("Thread %d: %ld rays cast, %ld objects checked, %ld samples computed\n",
			Index, AllStats[Index].RaysCast, AllStats[Index].ObjectsChecked, AllStats[Index].SamplesComputed);
		OverallStats.RaysCast += AllStats[Index].RaysCast;
		OverallStats.ObjectsChecked += AllStats[Index].ObjectsChecked;
		OverallStats.SamplesComputed += AllStats[Index].SamplesComputed;
	}
	printf("--------\n");
	printf("Overall: %ld rays cast, %ld objects checked, %ld samples computed\n",
		OverallStats.RaysCast, OverallStats.ObjectsChecked, OverallStats.SamplesComputed);
}

function surface
CreateSurface(s32 Width, s32 Height, memory_arena* Arena)
{
	surface Surface = {
		.Width = Width,
		.Height = Height,
		.Pixels = PushArray(Arena, Width * Height, color),
	};
	return Surface;
}

function void
TestRNG()
{
	random_sequence RNG = SeedRandom(987654321ull);
	s32 Buckets[100] = {};
	s32 NumTrials = 1000000;
	for (s32 Index = 0; Index < NumTrials; ++Index)
	{
		f32 Random = RandomUnilateral(&RNG);
		for (u64 BucketIndex = 0; BucketIndex < ArrayCount(Buckets); ++BucketIndex)
		{
			f32 Bound = ((f32)BucketIndex + 1.0f) / (f32)ArrayCount(Buckets);
			if (Random <= Bound)
			{
				++Buckets[BucketIndex];
				break;
			}
		}
	}
	
	// f32 Coefficient = 1.0f / NumTrials;
	printf("Random distribution:\n");
	for (u64 BucketIndex = 0; BucketIndex < ArrayCount(Buckets); ++BucketIndex)
	{
		printf("%d ", Buckets[BucketIndex]);
	}
	printf("\n");
}

typedef struct command_options
{
	b32 Error;
	const char* SceneFile;
	const char* OutputFile;
	s32 VerticalResolution;
	s32 SamplesPerPixel;
	s32 MaxBounces;
	b32 Debug;
} command_options;

function command_options
DefaultOptions()
{
	command_options Default =
	{
		false,
		"scene.scn",
		"output/render.tga",
		400,
		16,
		4,
		false,
	};
	return Default;
}

function command_options
ParseArgs(int ArgCount, char** Args)
{
	command_options Options = DefaultOptions();
	int ArgIndex = 1;
	while (!Options.Error && ArgIndex < ArgCount)
	{
		char* Arg = Args[ArgIndex];
		if (CStrEq(Arg, "-s") || CStrEq(Arg, "--scene"))
		{
			++ArgIndex;
			if (ArgIndex < ArgCount)
			{
				Options.SceneFile = Args[ArgIndex];
			}
			else
			{
				Options.Error = true;
				fprintf(stderr, "No argument given after --scene\n");
			}
		}
		else if (CStrEq(Arg, "-o") || CStrEq(Arg, "--output"))
		{
			++ArgIndex;
			if (ArgIndex < ArgCount)
			{
				Options.OutputFile = Args[ArgIndex];
			}
			else
			{
				Options.Error = true;
				fprintf(stderr, "No argument given after --output\n");
			}
		}
		else if (CStrEq(Arg, "-r") || CStrEq(Arg, "--resolution"))
		{
			++ArgIndex;
			if (ArgIndex < ArgCount)
			{
				s32 VerticalResolution = (s32)strtol(Args[ArgIndex], 0, 10);
				if (VerticalResolution > 0)
				{
					Options.VerticalResolution = VerticalResolution;
				}
				else
				{
					Options.Error = true;
					fprintf(stderr, "Invalid vertical resolution: '%s'\n", Args[ArgIndex]);
				}
			}
			else
			{
				Options.Error = true;
				fprintf(stderr, "No argument given after --resolution\n");
			}
		}
		else if (CStrEq(Arg, "-p") || CStrEq(Arg, "--samples"))
		{
			++ArgIndex;
			if (ArgIndex < ArgCount)
			{
				s32 SamplesPerPixel = (s32)strtol(Args[ArgIndex], 0, 10);
				if (SamplesPerPixel > 0)
				{
					Options.SamplesPerPixel = SamplesPerPixel;
				}
				else
				{
					Options.Error = true;
					fprintf(stderr, "Invalid samples per pixel: '%s'\n", Args[ArgIndex]);
				}
			}
			else
			{
				Options.Error = true;
				fprintf(stderr, "No argument given after --samples\n");
			}
		}
		else if (CStrEq(Arg, "-b") || CStrEq(Arg, "--bounces"))
		{
			++ArgIndex;
			if (ArgIndex < ArgCount)
			{
				s32 MaxBounces = (s32)strtol(Args[ArgIndex], 0, 10);
				if (MaxBounces > 0)
				{
					Options.MaxBounces = MaxBounces;
				}
				else
				{
					Options.Error = true;
					fprintf(stderr, "Invalid max bounces: '%s'\n", Args[ArgIndex]);
				}
			}
			else
			{
				Options.Error = true;
				fprintf(stderr, "No argument given after --bounces\n");
			}
		}
		else if (CStrEq(Arg, "-d") || CStrEq(Arg, "--debug"))
		{
			Options.Debug = true;
		}
		else
		{
			Options.Error = true;
			fprintf(stderr, "Unrecognized argument: '%s'\n", Arg);
		}
		++ArgIndex;
	}
	return Options;
}

int
main(int ArgCount, char** Args)
{
	b32 Success = true;
	
	command_options Options = ParseArgs(ArgCount, Args);
	if (!Options.Error)
	{
		printf("Options:\n");
		printf("SceneFile: '%s'\n",
			Options.SceneFile);
		printf("OutputFile: '%s'\n",
			Options.OutputFile);
		printf("VerticalResolution: %d\n",
			Options.VerticalResolution);
		printf("SamplesPerPixel: %d\n",
			Options.SamplesPerPixel);
		printf("MaxBounces: %d\n",
			Options.MaxBounces);
		memory_arena Arena = MakeArena(1024*1024*1024, 16);
		scene Scene = {};
		Success = LoadSceneFromFile(Options.SceneFile, &Scene, &Arena);
		if (Success)
		{
			f32 AspectRatio = Scene.Camera.SurfaceWidth / Scene.Camera.SurfaceHeight;
			s32 HorizontalResolution = (s32)(AspectRatio * (f32)Options.VerticalResolution);
			surface Surface = CreateSurface(HorizontalResolution, Options.VerticalResolution, &Arena);
			
			std::chrono::time_point<std::chrono::high_resolution_clock> StartTime = std::chrono::high_resolution_clock::now();
			
			RayTrace(&Scene, &Surface, Options.SamplesPerPixel, Options.MaxBounces, &Arena, Options.Debug);
			
			std::chrono::time_point<std::chrono::high_resolution_clock> EndTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> ElapsedTime = EndTime - StartTime;
			printf("\tTime:\t%6.4f (s) \n", ElapsedTime.count());
			
			Success = WriteTGA(&Surface, Options.OutputFile, &Arena);
			// surface TextureTest = Scene.Textures[2];
			// Success = WriteTGA(&TextureTest, Args[2], &Arena);
			if (!Success)
			{
				fprintf(stderr, "Error writing render to output file: '%s'\n", Options.OutputFile);
			}
		}
		else
		{
			fprintf(stderr, "Error loading scene from file: '%s'\n", Options.SceneFile);
		}
		
		// TestRNG();
	}
	else
	{
		fprintf(stderr, "Usage: %s -s <scene.scn> -o <render.tga> -r <vertical resolution> -p <samples per pixel> -b <max bounces>\n", Args[0]);
	}
	
	return !Success;
}