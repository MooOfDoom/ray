/*
 * scene.h
 *
 * Structs and definitions for laying out the scene
 */

typedef struct texture_handle
{
	s32 Index;
} texture_handle;

typedef struct uv_map
{
	uv VertexUV[3];
} uv_map;

enum object_type
{
	Obj_None,
	Obj_Plane,
	Obj_Sphere,
	Obj_Triangle,
	Obj_Parallelogram,
};

typedef struct object
{
	s32 Type;
	union
	{
		struct
		{
			v3 Normal;
			f32 Displacement;
		} Plane;
		struct
		{
			v3 Center;
			f32 Radius;
		} Sphere;
		struct
		{
			v3 Vertex[3];
		} Triangle;
		struct
		{
			v3 Origin;
			union
			{
				struct
				{
					v3 XAxis;
					v3 YAxis;
				};
				v3 Axis[2];
			};
		} Parallelogram;
	};
	color Color;
	f32 Glossy;
	f32 Translucency;
	f32 Refraction;
	texture_handle Texture;
	uv_map UVMap;
} object;

typedef struct camera
{
	v3 Origin;
	union
	{
		struct
		{
			v3 XAxis;
			v3 YAxis;
			v3 ZAxis;
		};
		v3 Axis[3];
	};
	f32 DistToSurface;
	f32 SurfaceWidth;
	f32 SurfaceHeight;
} camera;

typedef struct surface
{
	s32 Width;
	s32 Height;
	color* Pixels;
} surface;

typedef struct scene
{
	s32 ObjectCount;
	s32 TextureCount;
	object* Objects;
	surface* Textures;
	camera Camera;
	color SkyColor;
} scene;

typedef struct ray_hit
{
	f32 Dist;
	object* Object;
	v3 Normal;
	v2 UV;
} ray_hit;

function camera
LookAt(v3 Origin, v3 Destination)
{
	v3 ZAxis = NormOrDefault(Origin - Destination, (v3){0, -1.0f, 0});
	v3 XAxis = NormOrDefault(Cross((v3){0, 0, 1.0f}, ZAxis), (v3){1.0f, 0, 0});
	v3 YAxis = NormOrDefault(Cross(ZAxis, XAxis), (v3){0, 0, 1.0f});
	
	camera Camera = {};
	Camera.Origin = Origin;
	Camera.XAxis = XAxis;
	Camera.YAxis = YAxis;
	Camera.ZAxis = ZAxis;
	Camera.DistToSurface = 1.0f;
	Camera.SurfaceWidth = 1.0f;
	Camera.SurfaceHeight = 1.0f;
	
	printf("X(%.2f,%.2f,%.2f) Y(%.2f,%.2f,%.2f) Z(%.2f,%.2f,%.2f)\n",
		XAxis.X, XAxis.Y, XAxis.Z, YAxis.X, YAxis.Y, YAxis.Z, ZAxis.X, ZAxis.Y, ZAxis.Z);
	
	return Camera;
}
