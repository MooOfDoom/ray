/*
 * spatialpartition.h
 *
 * Spatial partition for the scene
 */

typedef struct rect3
{
	v3 Min;
	v3 Max;
} rect3;

typedef struct spatial_node
{
	rect3 Bounds;
	b32 IsLeaf;
	s32 SplitAxisIndex;
	spatial_node* Children[2];
	s32 ObjectCount;
	s32 FirstObjectIndex;
} spatial_node;

typedef struct spatial_partition
{
	spatial_node* RootNode;
	s32 ObjectCount;
	s32* ObjectIndices;
} spatial_partition;

typedef struct ray_trace_stats
{
	s64 RaysCast;
	s64 SpatialNodesChecked;
	s64 ObjectsChecked;
	s64 SamplesComputed;
	s64 Padding[4];
} ray_trace_stats;

function rect3
Union(rect3 A, rect3 B)
{
	rect3 Result =
	{
		{
			Minimum(A.Min.X, B.Min.X),
			Minimum(A.Min.Y, B.Min.Y),
			Minimum(A.Min.Z, B.Min.Z),
		},
		{
			Maximum(A.Max.X, B.Max.X),
			Maximum(A.Max.Y, B.Max.Y),
			Maximum(A.Max.Z, B.Max.Z),
		},
	};
	return Result;
}

function rect3
Intersection(rect3 A, rect3 B)
{
	rect3 Result =
	{
		{
			Maximum(A.Min.X, B.Min.X),
			Maximum(A.Min.Y, B.Min.Y),
			Maximum(A.Min.Z, B.Min.Z),
		},
		{
			Minimum(A.Max.X, B.Max.X),
			Minimum(A.Max.Y, B.Max.Y),
			Minimum(A.Max.Z, B.Max.Z),
		},
	};
	return Result;
}

function b32
IsInside(v3 P, rect3 Bounds)
{
	b32 Result =
		(P.X >= Bounds.Min.X) && (P.X <= Bounds.Max.X) &&
		(P.Y >= Bounds.Min.Y) && (P.Y <= Bounds.Max.Y) &&
		(P.Z >= Bounds.Min.Z) && (P.Z <= Bounds.Max.Z);
	return Result;
}

function void
PrintRect(rect3 A)
{
	printf("[(%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f)]",
		A.Min.X, A.Min.Y, A.Min.Z,
		A.Max.X, A.Max.Y, A.Max.Z);
}

function void
PrintNode(spatial_node* Node, s32 Indent=0)
{
	for (s32 Index = 0; Index < Indent; ++Index)
	{
		printf("\t");
	}
	printf("Bounds = ");
	PrintRect(Node->Bounds);
	printf("\n");
	for (s32 Index = 0; Index < Indent; ++Index)
	{
		printf("\t");
	}
	printf("IsLeaf = %d\n", Node->IsLeaf);
	for (s32 Index = 0; Index < Indent; ++Index)
	{
		printf("\t");
	}
	printf("SplitAxisIndex = %d\n", Node->SplitAxisIndex);
	for (s32 Index = 0; Index < Indent; ++Index)
	{
		printf("\t");
	}
	printf("ObjectCount = %d\n", Node->ObjectCount);
	for (s32 Index = 0; Index < Indent; ++Index)
	{
		printf("\t");
	}
	printf("FirstObjectIndex = %d\n", Node->FirstObjectIndex);
}

function rect3
GetObjectBoundingBox(object* Object)
{
	rect3 Result = {};
	switch (Object->Type)
	{
		case Obj_Plane:
		{
			if (Object->Plane.Normal.X == 0 && Object->Plane.Normal.Y == 0)
			{
				Result.Min = (v3){F32Min, F32Min, Object->Plane.Displacement};
				Result.Max = (v3){F32Max, F32Max, Object->Plane.Displacement};
			}
			else if (Object->Plane.Normal.Y == 0 && Object->Plane.Normal.Z == 0)
			{
				Result.Min = (v3){Object->Plane.Displacement, F32Min, F32Min};
				Result.Max = (v3){Object->Plane.Displacement, F32Max, F32Max};
			}
			else if (Object->Plane.Normal.Z == 0 && Object->Plane.Normal.X == 0)
			{
				Result.Min = (v3){F32Min, Object->Plane.Displacement, F32Min};
				Result.Max = (v3){F32Max, Object->Plane.Displacement, F32Max};
			}
			else
			{
				Result.Min = (v3){F32Min, F32Min, F32Min};
				Result.Max = (v3){F32Max, F32Max, F32Max};
			}
		} break;
		
		case Obj_Sphere:
		{
			v3 RadiusV = {Object->Sphere.Radius, Object->Sphere.Radius, Object->Sphere.Radius};
			Result.Min = Object->Sphere.Center - RadiusV;
			Result.Max = Object->Sphere.Center + RadiusV;
		} break;
		
		case Obj_Triangle:
		{
			Result.Min.X = Minimum(Minimum(Object->Triangle.Vertex[0].X, Object->Triangle.Vertex[1].X), Object->Triangle.Vertex[2].X);
			Result.Min.Y = Minimum(Minimum(Object->Triangle.Vertex[0].Y, Object->Triangle.Vertex[1].Y), Object->Triangle.Vertex[2].Y);
			Result.Min.Z = Minimum(Minimum(Object->Triangle.Vertex[0].Z, Object->Triangle.Vertex[1].Z), Object->Triangle.Vertex[2].Z);
			Result.Max.X = Maximum(Maximum(Object->Triangle.Vertex[0].X, Object->Triangle.Vertex[1].X), Object->Triangle.Vertex[2].X);
			Result.Max.Y = Maximum(Maximum(Object->Triangle.Vertex[0].Y, Object->Triangle.Vertex[1].Y), Object->Triangle.Vertex[2].Y);
			Result.Max.Z = Maximum(Maximum(Object->Triangle.Vertex[0].Z, Object->Triangle.Vertex[1].Z), Object->Triangle.Vertex[2].Z);
		} break;
		
		case Obj_Parallelogram:
		{
			v3 V0 = Object->Parallelogram.Origin;
			v3 V1 = V0 + Object->Parallelogram.XAxis;
			v3 V2 = V1 + Object->Parallelogram.YAxis;
			v3 V3 = V0 + Object->Parallelogram.YAxis;
			Result.Min.X = Minimum(Minimum(Minimum(V0.X, V1.X), V2.X), V3.X);
			Result.Min.Y = Minimum(Minimum(Minimum(V0.Y, V1.Y), V2.Y), V3.Y);
			Result.Min.Z = Minimum(Minimum(Minimum(V0.Z, V1.Z), V2.Z), V3.Z);
			Result.Max.X = Maximum(Maximum(Maximum(V0.X, V1.X), V2.X), V3.X);
			Result.Max.Y = Maximum(Maximum(Maximum(V0.Y, V1.Y), V2.Y), V3.Y);
			Result.Max.Z = Maximum(Maximum(Maximum(V0.Z, V1.Z), V2.Z), V3.Z);
		} break;
		
		default:
		{
			fprintf(stderr, "Error: Object of type %d passed to GetObjectBoundingBox!\n", Object->Type);
		} break;
	}
	return Result;
}

function rect3
GetRelativeBoundingBox(object* Object, rect3 Bounds)
{
	rect3 Result = {};
	switch (Object->Type)
	{
		case Obj_Plane:
		{
			v3 N = Object->Plane.Normal;
			f32 D = Object->Plane.Displacement;
			for (s32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
			{
				s32 AxisIndex2 = (AxisIndex + 1) % 3;
				s32 AxisIndex3 = (AxisIndex + 2) % 3;
				
				if (N.E[AxisIndex] != 0)
				{
					f32 MinMinT = (D - Bounds.Min.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Min.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
					f32 MinMaxT = (D - Bounds.Min.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Max.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
					f32 MaxMinT = (D - Bounds.Max.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Min.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
					f32 MaxMaxT = (D - Bounds.Max.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Max.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
					f32 MinT = Minimum(Minimum(Minimum(MinMinT, MinMaxT), MaxMinT), MaxMaxT);
					f32 MaxT = Maximum(Maximum(Maximum(MinMinT, MinMaxT), MaxMinT), MaxMaxT);
					Result.Min.E[AxisIndex] = Maximum(MinT, Bounds.Min.E[AxisIndex]);
					Result.Max.E[AxisIndex] = Minimum(MaxT, Bounds.Max.E[AxisIndex]);
				}
				else
				{
					Result.Min.E[AxisIndex] = Bounds.Min.E[AxisIndex];
					Result.Max.E[AxisIndex] = Bounds.Max.E[AxisIndex];
				}
			}
		} break;
		
		case Obj_Sphere:
		{
			v3 C = Object->Sphere.Center;
			f32 R = Object->Sphere.Radius;
			v3 ClampedC =
			{
				Clamp(Bounds.Min.X, C.X, Bounds.Max.X),
				Clamp(Bounds.Min.Y, C.Y, Bounds.Max.Y),
				Clamp(Bounds.Min.Z, C.Z, Bounds.Max.Z),
			};
			if (LengthSq(ClampedC - C) <= R*R)
			{
				for (s32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
				{
					v3 V = ClampedC - C;
					V.E[AxisIndex] = 0;
					f32 A = sqrtf(R*R - LengthSq(V));
					f32 MinT = C.E[AxisIndex] - A;
					f32 MaxT = C.E[AxisIndex] + A;
					Result.Min.E[AxisIndex] = Maximum(MinT, Bounds.Min.E[AxisIndex]);
					Result.Max.E[AxisIndex] = Minimum(MaxT, Bounds.Max.E[AxisIndex]);
				}
			}
			else
			{
				Result.Min = Bounds.Max;
				Result.Max = Bounds.Min;
			}
		} break;
		
		case Obj_Triangle:
		{
			v3 TestVertices[33];
			TestVertices[0] = Object->Triangle.Vertex[0];
			TestVertices[1] = Object->Triangle.Vertex[1];
			TestVertices[2] = Object->Triangle.Vertex[2];
			v3 Edges[] =
			{
				TestVertices[1] - TestVertices[0],
				TestVertices[2] - TestVertices[1],
				TestVertices[0] - TestVertices[2],
			};
			s32 TestCount = 3;
			for (s32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
			{
				for (s32 EdgeIndex = 0; EdgeIndex < 3; ++EdgeIndex)
				{
					f32 TMin = (Bounds.Min.E[AxisIndex] - TestVertices[EdgeIndex].E[AxisIndex]) / Edges[EdgeIndex].E[AxisIndex];
					f32 TMax = (Bounds.Max.E[AxisIndex] - TestVertices[EdgeIndex].E[AxisIndex]) / Edges[EdgeIndex].E[AxisIndex];
					if (TMin >= 0 && TMin <= 1.0f)
					{
						TestVertices[TestCount++] = TestVertices[EdgeIndex] + TMin*Edges[EdgeIndex];
					}
					if (TMax >= 0 && TMax <= 1.0f)
					{
						TestVertices[TestCount++] = TestVertices[EdgeIndex] + TMax*Edges[EdgeIndex];
					}
				}
			}
			v3 N = Cross(Edges[0], Edges[1]);
			if (N != (v3){0})
			{
				f32 D = Dot(TestVertices[0], N);
				f32 ABDotAC = -Dot(Edges[0], Edges[2]);
				v3 ABPerp = -Edges[2] - Edges[0]*(ABDotAC/LengthSq(Edges[0]));
				ABPerp = ABPerp/LengthSq(ABPerp);
				v3 ACPerp = Edges[0] + Edges[2]*(ABDotAC/LengthSq(Edges[2]));
				ACPerp = ACPerp/LengthSq(ACPerp);
				for (s32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
				{
					s32 AxisIndex2 = (AxisIndex + 1) % 3;
					s32 AxisIndex3 = (AxisIndex + 2) % 3;
					
					if (N.E[AxisIndex] != 0)
					{
						f32 MinMinT = (D - Bounds.Min.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Min.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
						f32 MinMaxT = (D - Bounds.Min.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Max.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
						f32 MaxMinT = (D - Bounds.Max.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Min.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
						f32 MaxMaxT = (D - Bounds.Max.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Max.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
						v3 TestV;
						TestV.E[AxisIndex] = MinMinT;
						TestV.E[AxisIndex2] = Bounds.Min.E[AxisIndex2];
						TestV.E[AxisIndex3] = Bounds.Min.E[AxisIndex3];
						v3 AP = TestV - TestVertices[0];
						f32 U = Dot(AP, ACPerp);
						f32 V = Dot(AP, ABPerp);
						if (U >= 0 && V >= 0 && U + V <= 1.0f)
						{
							TestVertices[TestCount++] = TestV;
						}
						TestV.E[AxisIndex] = MinMaxT;
						TestV.E[AxisIndex2] = Bounds.Min.E[AxisIndex2];
						TestV.E[AxisIndex3] = Bounds.Max.E[AxisIndex3];
						AP = TestV - TestVertices[0];
						U = Dot(AP, ACPerp);
						V = Dot(AP, ABPerp);
						if (U >= 0 && V >= 0 && U + V <= 1.0f)
						{
							TestVertices[TestCount++] = TestV;
						}
						TestV.E[AxisIndex] = MaxMinT;
						TestV.E[AxisIndex2] = Bounds.Max.E[AxisIndex2];
						TestV.E[AxisIndex3] = Bounds.Min.E[AxisIndex3];
						AP = TestV - TestVertices[0];
						U = Dot(AP, ACPerp);
						V = Dot(AP, ABPerp);
						if (U >= 0 && V >= 0 && U + V <= 1.0f)
						{
							TestVertices[TestCount++] = TestV;
						}
						TestV.E[AxisIndex] = MaxMaxT;
						TestV.E[AxisIndex2] = Bounds.Max.E[AxisIndex2];
						TestV.E[AxisIndex3] = Bounds.Max.E[AxisIndex3];
						AP = TestV - TestVertices[0];
						U = Dot(AP, ACPerp);
						V = Dot(AP, ABPerp);
						if (U >= 0 && V >= 0 && U + V <= 1.0f)
						{
							TestVertices[TestCount++] = TestV;
						}
					}
				}
			}
			
			Result.Min = Bounds.Max;
			Result.Max = Bounds.Min;
			// printf("Result: ");
			// PrintRect(Result);
			// printf("\n");
			// printf("Bounds: ");
			// PrintRect(Bounds);
			// printf("\n");
			// printf("Triangle:\n");
			for (s32 TestIndex = 0; TestIndex < TestCount; ++TestIndex)
			{
				// printf("(%.2f, %.2f, %.2f)", TestVertices[TestIndex].X, TestVertices[TestIndex].Y, TestVertices[TestIndex].Z);
				if (IsInside(TestVertices[TestIndex], Bounds))
				{
					// printf("[I]\n");
					if (TestVertices[TestIndex].X < Result.Min.X)
					{
						Result.Min.X = TestVertices[TestIndex].X;
					}
					if (TestVertices[TestIndex].Y < Result.Min.Y)
					{
						Result.Min.Y = TestVertices[TestIndex].Y;
					}
					if (TestVertices[TestIndex].Z < Result.Min.Z)
					{
						Result.Min.Z = TestVertices[TestIndex].Z;
					}
					if (TestVertices[TestIndex].X > Result.Max.X)
					{
						Result.Max.X = TestVertices[TestIndex].X;
					}
					if (TestVertices[TestIndex].Y > Result.Max.Y)
					{
						Result.Max.Y = TestVertices[TestIndex].Y;
					}
					if (TestVertices[TestIndex].Z > Result.Max.Z)
					{
						Result.Max.Z = TestVertices[TestIndex].Z;
					}
					// PrintRect(Result);
				}
				// printf("\n");
			}
			// printf("\nResult: ");
			// PrintRect(Result);
			// printf("\n");
		} break;
		
		case Obj_Parallelogram:
		{
			v3 TestVertices[40];
			TestVertices[0] = Object->Parallelogram.Origin;
			TestVertices[1] = TestVertices[0] + Object->Parallelogram.XAxis;
			TestVertices[2] = TestVertices[1] + Object->Parallelogram.YAxis;
			TestVertices[3] = TestVertices[0] + Object->Parallelogram.YAxis;
			v3 Edges[] =
			{
				Object->Parallelogram.XAxis,
				Object->Parallelogram.YAxis,
				-Object->Parallelogram.XAxis,
				-Object->Parallelogram.YAxis,
			};
			s32 TestCount = 4;
			for (s32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
			{
				for (s32 EdgeIndex = 0; EdgeIndex < 4; ++EdgeIndex)
				{
					f32 TMin = (Bounds.Min.E[AxisIndex] - TestVertices[EdgeIndex].E[AxisIndex]) / Edges[EdgeIndex].E[AxisIndex];
					f32 TMax = (Bounds.Max.E[AxisIndex] - TestVertices[EdgeIndex].E[AxisIndex]) / Edges[EdgeIndex].E[AxisIndex];
					if (TMin >= 0 && TMin <= 1.0f)
					{
						TestVertices[TestCount++] = TestVertices[EdgeIndex] + TMin*Edges[EdgeIndex];
					}
					if (TMax >= 0 && TMax <= 1.0f)
					{
						TestVertices[TestCount++] = TestVertices[EdgeIndex] + TMax*Edges[EdgeIndex];
					}
				}
			}
			v3 N = Cross(Edges[0], Edges[1]);
			if (N != (v3){0})
			{
				f32 D = Dot(TestVertices[0], N);
				f32 ABDotAC = Dot(Edges[0], Edges[1]);
				v3 ABPerp = Edges[1] - Edges[0]*(ABDotAC/LengthSq(Edges[0]));
				ABPerp = ABPerp/LengthSq(ABPerp);
				v3 ACPerp = Edges[0] + Edges[1]*(ABDotAC/LengthSq(Edges[1]));
				ACPerp = ACPerp/LengthSq(ACPerp);
				for (s32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
				{
					s32 AxisIndex2 = (AxisIndex + 1) % 3;
					s32 AxisIndex3 = (AxisIndex + 2) % 3;
					
					if (N.E[AxisIndex] != 0)
					{
						f32 MinMinT = (D - Bounds.Min.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Min.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
						f32 MinMaxT = (D - Bounds.Min.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Max.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
						f32 MaxMinT = (D - Bounds.Max.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Min.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
						f32 MaxMaxT = (D - Bounds.Max.E[AxisIndex2]*N.E[AxisIndex2] + Bounds.Max.E[AxisIndex3]*N.E[AxisIndex3]) / N.E[AxisIndex];
						v3 TestV;
						TestV.E[AxisIndex] = MinMinT;
						TestV.E[AxisIndex2] = Bounds.Min.E[AxisIndex2];
						TestV.E[AxisIndex3] = Bounds.Min.E[AxisIndex3];
						v3 AP = TestV - TestVertices[0];
						f32 U = Dot(AP, ACPerp);
						f32 V = Dot(AP, ABPerp);
						if (U >= 0 && U <= 1.0f && V >= 0 && V <= 1.0f)
						{
							TestVertices[TestCount++] = TestV;
						}
						TestV.E[AxisIndex] = MinMaxT;
						TestV.E[AxisIndex2] = Bounds.Min.E[AxisIndex2];
						TestV.E[AxisIndex3] = Bounds.Max.E[AxisIndex3];
						AP = TestV - TestVertices[0];
						U = Dot(AP, ACPerp);
						V = Dot(AP, ABPerp);
						if (U >= 0 && U <= 1.0f && V >= 0 && V <= 1.0f)
						{
							TestVertices[TestCount++] = TestV;
						}
						TestV.E[AxisIndex] = MaxMinT;
						TestV.E[AxisIndex2] = Bounds.Max.E[AxisIndex2];
						TestV.E[AxisIndex3] = Bounds.Min.E[AxisIndex3];
						AP = TestV - TestVertices[0];
						U = Dot(AP, ACPerp);
						V = Dot(AP, ABPerp);
						if (U >= 0 && U <= 1.0f && V >= 0 && V <= 1.0f)
						{
							TestVertices[TestCount++] = TestV;
						}
						TestV.E[AxisIndex] = MaxMaxT;
						TestV.E[AxisIndex2] = Bounds.Max.E[AxisIndex2];
						TestV.E[AxisIndex3] = Bounds.Max.E[AxisIndex3];
						AP = TestV - TestVertices[0];
						U = Dot(AP, ACPerp);
						V = Dot(AP, ABPerp);
						if (U >= 0 && U <= 1.0f && V >= 0 && V <= 1.0f)
						{
							TestVertices[TestCount++] = TestV;
						}
					}
				}
			}
			
			Result.Min = Bounds.Max;
			Result.Max = Bounds.Min;
			// printf("Result: ");
			// PrintRect(Result);
			// printf("\n");
			// printf("Bounds: ");
			// PrintRect(Bounds);
			// printf("\n");
			// printf("Parallelogram:\n");
			for (s32 TestIndex = 0; TestIndex < TestCount; ++TestIndex)
			{
				// printf("(%.2f, %.2f, %.2f)", TestVertices[TestIndex].X, TestVertices[TestIndex].Y, TestVertices[TestIndex].Z);
				if (IsInside(TestVertices[TestIndex], Bounds))
				{
					// printf("[I]\n");
					if (TestVertices[TestIndex].X < Result.Min.X)
					{
						Result.Min.X = TestVertices[TestIndex].X;
					}
					if (TestVertices[TestIndex].Y < Result.Min.Y)
					{
						Result.Min.Y = TestVertices[TestIndex].Y;
					}
					if (TestVertices[TestIndex].Z < Result.Min.Z)
					{
						Result.Min.Z = TestVertices[TestIndex].Z;
					}
					if (TestVertices[TestIndex].X > Result.Max.X)
					{
						Result.Max.X = TestVertices[TestIndex].X;
					}
					if (TestVertices[TestIndex].Y > Result.Max.Y)
					{
						Result.Max.Y = TestVertices[TestIndex].Y;
					}
					if (TestVertices[TestIndex].Z > Result.Max.Z)
					{
						Result.Max.Z = TestVertices[TestIndex].Z;
					}
					// PrintRect(Result);
				}
				// printf("\n");
			}
			// printf("\nResult: ");
			// PrintRect(Result);
			// printf("\n");
		} break;
		
		default:
		{
			fprintf(stderr, "Error: Object of type %d passed to GetRelativeBoundingBox!\n", Object->Type);
		} break;
	}
	return Result;
}

function spatial_partition
GenerateSpatialPartition(scene* Scene, memory_arena* Arena, memory_arena* ScratchArena, s32 MaxObjectsPerLeaf, s32 MaxLeafDepth, f32 MaxDistance)
{
	spatial_partition Result = {};
	if (Scene->ObjectCount > MaxObjectsPerLeaf)
	{
		temporary_memory Temp = BeginTemporaryMemory(ScratchArena);
		
		// TODO: Will we need this?
		rect3* ObjectBoundingBoxes = PushArray(ScratchArena, Scene->ObjectCount, rect3);
		ObjectBoundingBoxes[0] = GetObjectBoundingBox(Scene->Objects + 0);
		// printf("0: ");
		// PrintRect(ObjectBoundingBoxes[0]);
		rect3 RootBounds = ObjectBoundingBoxes[0];
		for (s32 Index = 1; Index < Scene->ObjectCount; ++Index)
		{
			ObjectBoundingBoxes[Index] = GetObjectBoundingBox(Scene->Objects + Index);
			// printf("\n%d: ", Index);
			// PrintRect(ObjectBoundingBoxes[Index]);
			RootBounds = Union(RootBounds, ObjectBoundingBoxes[Index]);
		}
		// printf("\n");
		
		v3 MaxDistV = {MaxDistance, MaxDistance, MaxDistance};
		rect3 CameraBounds =
		{
			Scene->Camera.Origin - MaxDistV,
			Scene->Camera.Origin + MaxDistV,
		};
		RootBounds = Intersection(RootBounds, CameraBounds);
		
		Result.RootNode = PushStruct(Arena, spatial_node);
		Result.RootNode->Bounds = RootBounds;
		Result.RootNode->IsLeaf = false;
		s64 OldAlignment = Arena->Alignment;
		SetAlignment(Arena, 1);
		
		temporary_memory CircularStart = BeginTemporaryMemory(ScratchArena);
		
		s32* TempObjectIndices = PushArray(ScratchArena, 2*Scene->ObjectCount, s32);
		
		s32 SplitAxisIndex = -1;
		s32 BestCount = Scene->ObjectCount;
		s32 SplitCountLow = 0;
		s32 LargestAxisIndex = 0;
		f32 LargestAxisSize = F32Min;
		f32 LargestSplitCountLow = 0;
		for (s32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
		{
			f32 SplitPoint = 0.5f * (RootBounds.Max.E[AxisIndex] + RootBounds.Min.E[AxisIndex]);
			s32 CountLow = 0;
			s32 CountHigh = 0;
			for (s32 Index = 0; Index < Scene->ObjectCount; ++Index)
			{
				if (ObjectBoundingBoxes[Index].Min.E[AxisIndex] < SplitPoint)
				{
					++CountLow;
				}
				
				if (ObjectBoundingBoxes[Index].Max.E[AxisIndex] >= SplitPoint)
				{
					++CountHigh;
				}
			}
			
			s32 MaxCount = Maximum(CountLow, CountHigh);
			if (MaxCount < BestCount)
			{
				BestCount = MaxCount;
				SplitCountLow = CountLow;
				SplitAxisIndex = AxisIndex;
			}
			
			f32 AxisSize = RootBounds.Max.E[AxisIndex] - RootBounds.Min.E[AxisIndex];
			if (AxisSize > LargestAxisSize)
			{
				LargestAxisIndex = AxisIndex;
				LargestAxisSize = AxisSize;
				LargestSplitCountLow = CountLow;
			}
		}
		
		if (SplitAxisIndex == -1)
		{
			SplitAxisIndex = LargestAxisIndex;
			SplitCountLow = LargestSplitCountLow;
		}
		
		Result.RootNode->SplitAxisIndex = SplitAxisIndex;
		
		f32 SplitPoint = 0.5f * (RootBounds.Max.E[SplitAxisIndex] + RootBounds.Min.E[SplitAxisIndex]);
		s32 CountLow = 0;
		s32 CountHigh = 0;
		for (s32 Index = 0; Index < Scene->ObjectCount; ++Index)
		{
			if (ObjectBoundingBoxes[Index].Min.E[SplitAxisIndex] < SplitPoint)
			{
				TempObjectIndices[CountLow] = Index;
				++CountLow;
			}
			
			if (ObjectBoundingBoxes[Index].Max.E[SplitAxisIndex] >= SplitPoint)
			{
				TempObjectIndices[SplitCountLow + CountHigh] = Index;
				++CountHigh;
			}
		}
		
		// printf("Initial: [");
		// for (s32 Index = 0; Index < CountLow; ++Index)
		// {
		// 	if (Index != 0)
		// 	{
		// 		printf(", ");
		// 	}
		// 	printf("%d", TempObjectIndices[Index]);
		// }
		// printf(" | ");
		// for (s32 Index = 0; Index < CountHigh; ++Index)
		// {
		// 	if (Index != 0)
		// 	{
		// 		printf(", ");
		// 	}
		// 	printf("%d", TempObjectIndices[CountLow + Index]);
		// }
		// printf("]\n");
		
		spatial_node* Nodes = PushArray(Arena, 2, spatial_node);
		Result.RootNode->Children[0] = Nodes + 0;
		Result.RootNode->Children[1] = Nodes + 1;
		Nodes[0].Bounds = RootBounds;
		Nodes[0].Bounds.Max.E[SplitAxisIndex] = SplitPoint;
		Nodes[0].IsLeaf = -1; // Mark as unknown
		// Nodes[0].SplitAxisIndex = SplitAxisIndex;
		Nodes[0].FirstObjectIndex = 0;
		Nodes[0].ObjectCount = CountLow;
		Nodes[1].Bounds = RootBounds;
		Nodes[1].Bounds.Min.E[SplitAxisIndex] = SplitPoint;
		Nodes[1].IsLeaf = -1; // Mark as unknown
		// Nodes[1].SplitAxisIndex = SplitAxisIndex;
		Nodes[1].FirstObjectIndex = CountLow;
		Nodes[1].ObjectCount = CountHigh;
		
		s32 ChildNodeCount = 2;
		
		s32 TotalIndexCount = CountLow + CountHigh;
		
		for (s32 Depth = 0; Depth < MaxLeafDepth; ++Depth)
		{
			// printf("Depth: %d\n", Depth);
			if (!HasRoom(ScratchArena, 2*TotalIndexCount*sizeof(s32)))
			{
				// printf("wrapped\n");
				EndTemporaryMemory(CircularStart);
				CircularStart = BeginTemporaryMemory(ScratchArena);
			}
			s32* NewTempObjectIndices = PushArray(ScratchArena, 2*TotalIndexCount, s32);
			if (NewTempObjectIndices <= TempObjectIndices && NewTempObjectIndices + 2*TotalIndexCount > TempObjectIndices)
			{
				// Can't continue, so stop splitting
				fprintf(stderr, "Insufficient scratch space in GenerateSpatialPartition\n");
				break;
			}
			
			// printf("[");
			s32 IndexCount = 0;
			b32 NodeSplit = false;
			s32 NextChildNodeCount = ChildNodeCount;
			for (s32 NodeIndex = 0; NodeIndex < ChildNodeCount; ++NodeIndex)
			{
				// printf("\tIndex: %d\n", NodeIndex);
				spatial_node* Node = Nodes + NodeIndex;
				if ((Node->IsLeaf == -1) && (Node->ObjectCount > MaxObjectsPerLeaf))
				{
					if (HasRoom(Arena, 2*sizeof(spatial_node) + 2*TotalIndexCount*sizeof(s32)))
					{
						// Split
						NodeSplit = true;
						Node->IsLeaf = false;
						s32 SplitAxisIndex = -1;
						s32 BestCount = Node->ObjectCount;
						s32 SplitCountLow = 0;
						f32 LargestAxisSize = F32Min;
						f32 LargestSplitCountLow = 0;
						// printf("Split Node %d:\n", NodeIndex);
						// PrintRect(Node->Bounds);
						// printf("\n");
						for (s32 Index = 0; Index < Node->ObjectCount; ++Index)
						{
							s32 ObjectIndex = TempObjectIndices[Node->FirstObjectIndex + Index];
							object* Object = Scene->Objects + ObjectIndex;
							ObjectBoundingBoxes[ObjectIndex] = GetRelativeBoundingBox(Object, Node->Bounds);
							// printf("\t%d: ", ObjectIndex);
							// PrintRect(ObjectBoundingBoxes[ObjectIndex]);
							// printf("\n");
						}
						for (s32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
						{
							f32 SplitPoint = 0.5f * (Node->Bounds.Max.E[AxisIndex] + Node->Bounds.Min.E[AxisIndex]);
							s32 CountLow = 0;
							s32 CountHigh = 0;
							for (s32 Index = 0; Index < Node->ObjectCount; ++Index)
							{
								s32 ObjectIndex = TempObjectIndices[Node->FirstObjectIndex + Index];
								if (ObjectBoundingBoxes[ObjectIndex].Min.E[AxisIndex] <= ObjectBoundingBoxes[ObjectIndex].Max.E[AxisIndex])
								{
									if (ObjectBoundingBoxes[ObjectIndex].Min.E[AxisIndex] < SplitPoint)
									{
										++CountLow;
									}
									
									if (ObjectBoundingBoxes[ObjectIndex].Max.E[AxisIndex] >= SplitPoint)
									{
										++CountHigh;
									}
								}
							}
							
							s32 MaxCount = Maximum(CountLow, CountHigh);
							if (MaxCount < BestCount)
							{
								BestCount = MaxCount;
								SplitCountLow = CountLow;
								SplitAxisIndex = AxisIndex;
							}
			
							f32 AxisSize = Node->Bounds.Max.E[AxisIndex] - Node->Bounds.Min.E[AxisIndex];
							if (AxisSize > LargestAxisSize)
							{
								LargestAxisIndex = AxisIndex;
								LargestAxisSize = AxisSize;
								LargestSplitCountLow = CountLow;
							}
						}
						
						if (SplitAxisIndex == -1)
						{
							SplitAxisIndex = LargestAxisIndex;
							SplitCountLow = LargestSplitCountLow;
						}
						
						Node->SplitAxisIndex = SplitAxisIndex;
						
						f32 SplitPoint = 0.5f * (Node->Bounds.Max.E[SplitAxisIndex] + Node->Bounds.Min.E[SplitAxisIndex]);
						s32 CountLow = 0;
						s32 CountHigh = 0;
						for (s32 Index = 0; Index < Node->ObjectCount; ++Index)
						{
							s32 ObjectIndex = TempObjectIndices[Node->FirstObjectIndex + Index];
							if (ObjectBoundingBoxes[ObjectIndex].Min.E[SplitAxisIndex] <= ObjectBoundingBoxes[ObjectIndex].Max.E[SplitAxisIndex])
							{
								if (ObjectBoundingBoxes[ObjectIndex].Min.E[SplitAxisIndex] < SplitPoint)
								{
									NewTempObjectIndices[IndexCount + CountLow] = ObjectIndex;
									++CountLow;
								}
								
								if (ObjectBoundingBoxes[ObjectIndex].Max.E[SplitAxisIndex] >= SplitPoint)
								{
									NewTempObjectIndices[IndexCount + SplitCountLow + CountHigh] = ObjectIndex;
									++CountHigh;
								}
							}
						}
						
						// printf(" |? ");
						// for (s32 Index = 0; Index < CountLow; ++Index)
						// {
						// 	if (Index != 0)
						// 	{
						// 		printf(", ");
						// 	}
						// 	printf("%d", NewTempObjectIndices[IndexCount + Index]);
						// }
						// printf(" |? ");
						// for (s32 Index = 0; Index < CountHigh; ++Index)
						// {
						// 	if (Index != 0)
						// 	{
						// 		printf(", ");
						// 	}
						// 	printf("%d", NewTempObjectIndices[IndexCount + CountLow + Index]);
						// }
						
						NextChildNodeCount += 2;
						spatial_node* Children = PushArray(Arena, 2, spatial_node);
						Node->Children[0] = Children + 0;
						Node->Children[1] = Children + 1;
						Children[0].Bounds = Node->Bounds;
						Children[0].Bounds.Max.E[SplitAxisIndex] = SplitPoint;
						Children[0].IsLeaf = -1; // Mark as unknown
						// Children[0].SplitAxisIndex = SplitAxisIndex;
						Children[0].FirstObjectIndex = IndexCount;
						Children[0].ObjectCount = CountLow;
						Children[1].Bounds = Node->Bounds;
						Children[1].Bounds.Min.E[SplitAxisIndex] = SplitPoint;
						Children[1].IsLeaf = -1; // Mark as unknown
						// Children[1].SplitAxisIndex = SplitAxisIndex;
						Children[1].FirstObjectIndex = IndexCount + CountLow;
						Children[1].ObjectCount = CountHigh;
						
						IndexCount += CountLow + CountHigh;
					}
					else
					{
						Node->IsLeaf = true;
						for (s32 Index = 0; Index < Node->ObjectCount; ++Index)
						{
							s32 ObjectIndex = TempObjectIndices[Node->FirstObjectIndex + Index];
							NewTempObjectIndices[IndexCount + Index] = ObjectIndex;
						}
						Node->FirstObjectIndex = IndexCount;
						
						// printf(" |L ");
						// for (s32 Index = 0; Index < Node->ObjectCount; ++Index)
						// {
						// 	if (Index != 0)
						// 	{
						// 		printf(", ");
						// 	}
						// 	printf("%d", NewTempObjectIndices[IndexCount + Index]);
						// }
						IndexCount += Node->ObjectCount;
					}
				}
				else if (Node->IsLeaf)
				{
					Node->IsLeaf = true;
					for (s32 Index = 0; Index < Node->ObjectCount; ++Index)
					{
						s32 ObjectIndex = TempObjectIndices[Node->FirstObjectIndex + Index];
						NewTempObjectIndices[IndexCount + Index] = ObjectIndex;
					}
					Node->FirstObjectIndex = IndexCount;
					
					// printf(" |L ");
					// for (s32 Index = 0; Index < Node->ObjectCount; ++Index)
					// {
					// 	if (Index != 0)
					// 	{
					// 		printf(", ");
					// 	}
					// 	printf("%d", NewTempObjectIndices[IndexCount + Index]);
					// }
					IndexCount += Node->ObjectCount;
				}
			}
			// printf("]\n");
			
			ChildNodeCount = NextChildNodeCount;
			TotalIndexCount = IndexCount;
			
			TempObjectIndices = NewTempObjectIndices;
			
			if (!NodeSplit)
			{
				break;
			}
		}
		
		for (s32 Index = 0; Index < ChildNodeCount; ++Index)
		{
			if (Nodes[Index].IsLeaf == -1)
			{
				Nodes[Index].IsLeaf = true;
			}
		}
		
		Result.ObjectIndices = (s32*)PushCopyArray(Arena, TotalIndexCount, TempObjectIndices);
		Result.ObjectCount = TotalIndexCount;
		
		SetAlignment(Arena, OldAlignment);
		EndTemporaryMemory(CircularStart);
		EndTemporaryMemory(Temp);
	}
	else
	{
		Result.ObjectCount = Scene->ObjectCount;
		Result.RootNode = PushStruct(Arena, spatial_node);
		Result.RootNode->Bounds = (rect3){{F32Min, F32Min, F32Min}, {F32Max, F32Max, F32Max}};
		Result.RootNode->IsLeaf = true;
		Result.RootNode->ObjectCount = Result.ObjectCount;
		Result.RootNode->FirstObjectIndex = 0;
		Result.ObjectIndices = PushArray(Arena, Result.ObjectCount, s32);
		for (s32 Index = 0; Index < Result.ObjectCount; ++Index)
		{
			Result.ObjectIndices[Index] = Index;
		}
	}
	return Result;
}

function ray_hit
RayIntersectScene(v3 RayOrigin, v3 RayDir, scene* Scene, spatial_partition* Partition, ray_trace_stats* Stats)
{
	ray_hit RayHit = {};
	spatial_node* Node = Partition->RootNode;
	s64 SpatialNodesChecked = 0;
	s64 ObjectsChecked = 0;
	// Walk through spatial partition until Node is a leaf containing RayOrigin
	v3 StartP = RayOrigin + EPSILON*RayDir;
	while (!Node->IsLeaf)
	{
		++SpatialNodesChecked;
		if (StartP.E[Node->SplitAxisIndex] < Node->Children[0]->Bounds.Max.E[Node->SplitAxisIndex])
		{
			Node = Node->Children[0];
		}
		else
		{
			Node = Node->Children[1];
		}
	}
	while (Node)
	{
		for (s32 Index = 0; Index < Node->ObjectCount; ++Index)
		{
			++ObjectsChecked;
			object* Object = Scene->Objects + Partition->ObjectIndices[Node->FirstObjectIndex + Index];
			switch (Object->Type)
			{
				case Obj_Plane:
				{
					f32 RayDDotNormal = Dot(RayDir, Object->Plane.Normal);
					if (Abs(RayDDotNormal) > 0)
					{
						f32 Hit = (Object->Plane.Displacement - Dot(RayOrigin, Object->Plane.Normal)) / RayDDotNormal;
						if (Hit > 0 && (RayHit.Dist == 0 || Hit < RayHit.Dist))
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
					if (Discriminant > 0)
					{
						f32 RootDisc = sqrtf(Discriminant);
						// b32 Inside = false;
						f32 Hit = -RayDDotFromCenter - RootDisc;
						if (Hit <= 0)
						{
							Hit = -RayDDotFromCenter + RootDisc;
							// Inside = true;
						}
						if (Hit > 0 && (RayHit.Dist == 0 || Hit < RayHit.Dist))
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
						if (Abs(RayDDotNormal) > 0)
						{
							f32 Hit = Dot(Object->Triangle.Vertex[0] - RayOrigin, Normal) / RayDDotNormal;
							if (Hit > 0 && (RayHit.Dist == 0 || Hit < RayHit.Dist))
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
						if (Abs(RayDDotNormal) > 0)
						{
							f32 Hit = Dot(Object->Parallelogram.Origin - RayOrigin, Normal) / RayDDotNormal;
							if (Hit > 0 && (RayHit.Dist == 0 || Hit < RayHit.Dist))
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
					fprintf(stderr, "Error: Encountered object of type %d in RayIntersectScene!\n", Object->Type);
				} break;
			}
		}
		
		// Check node bounding box
		f32 NodeExitDist = F32Max;
		if (RayDir.X > 0 && Node->Bounds.Max.X != Partition->RootNode->Bounds.Max.X)
		{
			f32 Hit = (Node->Bounds.Max.X - RayOrigin.X) / RayDir.X;
			if (Hit > 0 && (RayHit.Dist == 0 || Hit < RayHit.Dist) && Hit < NodeExitDist)
			{
				NodeExitDist = Hit;
			}
		}
		else if (RayDir.X < 0 && Node->Bounds.Min.X != Partition->RootNode->Bounds.Min.X)
		{
			f32 Hit = (Node->Bounds.Min.X - RayOrigin.X) / RayDir.X;
			if (Hit > 0 && (RayHit.Dist == 0 || Hit < RayHit.Dist) && Hit < NodeExitDist)
			{
				NodeExitDist = Hit;
			}
		}
		if (RayDir.Y > 0 && Node->Bounds.Max.Y != Partition->RootNode->Bounds.Max.Y)
		{
			f32 Hit = (Node->Bounds.Max.Y - RayOrigin.Y) / RayDir.Y;
			if (Hit > 0 && (RayHit.Dist == 0 || Hit < RayHit.Dist) && Hit < NodeExitDist)
			{
				NodeExitDist = Hit;
			}
		}
		else if (RayDir.Y < 0 && Node->Bounds.Min.Y != Partition->RootNode->Bounds.Min.Y)
		{
			f32 Hit = (Node->Bounds.Min.Y - RayOrigin.Y) / RayDir.Y;
			if (Hit > 0 && (RayHit.Dist == 0 || Hit < RayHit.Dist) && Hit < NodeExitDist)
			{
				NodeExitDist = Hit;
			}
		}
		if (RayDir.Z > 0 && Node->Bounds.Max.Z != Partition->RootNode->Bounds.Max.Z)
		{
			f32 Hit = (Node->Bounds.Max.Z - RayOrigin.Z) / RayDir.Z;
			if (Hit > 0 && (RayHit.Dist == 0 || Hit < RayHit.Dist) && Hit < NodeExitDist)
			{
				NodeExitDist = Hit;
			}
		}
		else if (RayDir.Z < 0 && Node->Bounds.Min.Z != Partition->RootNode->Bounds.Min.Z)
		{
			f32 Hit = (Node->Bounds.Min.Z - RayOrigin.Z) / RayDir.Z;
			if (Hit > 0 && (RayHit.Dist == 0 || Hit < RayHit.Dist) && Hit < NodeExitDist)
			{
				NodeExitDist = Hit;
			}
		}
		
		if (NodeExitDist != F32Max)
		{
			// Traverse spatial partition to next leaf
			// If exiting the scene, set Node = 0 and return!
			v3 NewP = RayOrigin + (NodeExitDist + EPSILON)*RayDir;
			spatial_node* NewNode = Partition->RootNode;
			// Walk through spatial partition until Node is a leaf containing NewP
			while (!NewNode->IsLeaf)
			{
				++SpatialNodesChecked;
				if (NewP.E[NewNode->SplitAxisIndex] < NewNode->Children[0]->Bounds.Max.E[NewNode->SplitAxisIndex])
				{
					NewNode = NewNode->Children[0];
				}
				else
				{
					NewNode = NewNode->Children[1];
				}
			}
			if (NewNode != Node)
			{
				Node = NewNode;
			}
			else // Couldn't find new node, so stop
			{
				Node = 0;
			}
		}
		else
		{
			Node = 0;
		}
	}
	
	Stats->SpatialNodesChecked += SpatialNodesChecked;
	Stats->ObjectsChecked += ObjectsChecked;
	++Stats->RaysCast;
	
	return RayHit;
}

function void
RayTrace(scene* Scene, spatial_partition* Partition, surface* Surface, s32 SamplesPerPixel, s32 MaxBounces, memory_arena* ScratchArena, b32 DebugOn)
{
	temporary_memory Temp = BeginTemporaryMemory(ScratchArena);
	b32 HitTexture = false; // Debug purposes
	s32 HitTranslucency = 0; // Debug purposes
	
	f32 PixelWidth = Scene->Camera.SurfaceWidth / (f32)Surface->Width;
	f32 PixelHeight = Scene->Camera.SurfaceHeight / (f32)Surface->Height;
	f32 SampleWidth = PixelWidth / (f32) SamplesPerPixel;
	f32 SampleHeight = PixelHeight / (f32) SamplesPerPixel;
	f32 SampleWeight = 1.0f / (SamplesPerPixel*SamplesPerPixel);
	v3 SurfaceOrigin = Scene->Camera.XAxis*(-0.5f*Scene->Camera.SurfaceWidth + 0.5f*SampleWidth) + Scene->Camera.YAxis*(-0.5f*Scene->Camera.SurfaceHeight + 0.5f*SampleHeight) - Scene->Camera.ZAxis*Scene->Camera.DistToSurface;
	
	s64 OldAlignment = ScratchArena->Alignment;
	SetAlignment(ScratchArena, 64); // Make sure to align to cache lines to avoid false sharing
	ray_trace_stats* AllStats = PushArray(ScratchArena, 0, ray_trace_stats); // Just find the location of the start, reserve the right number later ;)
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
			// printf("%d\n", Y);
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
							if (DebugOn && Debug)
							{
								printf("X(%d) Y(%d) Bounce(%d)\n",
									X, Y, Bounce);
								printf("RayOrigin(%.2f,%.2f,%.2f) RayDir(%.2f,%.2f,%.2f)\n",
									RayOrigin.X, RayOrigin.Y, RayOrigin.Z, RayDir.X, RayDir.Y, RayDir.Z);
							}
							
							ray_hit Hit = RayIntersectScene(RayOrigin, RayDir, Scene, Partition, &Stats);
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
									v3 RandomBounce = NormOrZero(Hit.Normal + RandomUnitBallV3(&RNG));
									if (RayDDotNormal > 0)
									{
										RandomBounce = -RandomBounce;
									}
									RayDir = NormOrDefault(Lerp(RandomBounce, Hit.Object->Glossy, Reflection), Hit.Normal);
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
	PushArray(ScratchArena, NumThreads, ray_trace_stats); // Actually reserve the space :)
	
	ray_trace_stats OverallStats = {};
	for (s32 Index = 0; Index < NumThreads; ++Index)
	{
		printf("Thread %d: %ld rays cast, %ld spatial nodes checked, %ld objects checked, %ld samples computed\n",
			Index, AllStats[Index].RaysCast, AllStats[Index].SpatialNodesChecked, AllStats[Index].ObjectsChecked, AllStats[Index].SamplesComputed);
		OverallStats.RaysCast += AllStats[Index].RaysCast;
		OverallStats.SpatialNodesChecked += AllStats[Index].SpatialNodesChecked;
		OverallStats.ObjectsChecked += AllStats[Index].ObjectsChecked;
		OverallStats.SamplesComputed += AllStats[Index].SamplesComputed;
	}
	printf("--------\n");
	printf("Overall: %ld rays cast, %ld spatial nodes checked, %ld objects checked, %ld samples computed\n",
		OverallStats.RaysCast, OverallStats.SpatialNodesChecked, OverallStats.ObjectsChecked, OverallStats.SamplesComputed);
	
	EndTemporaryMemory(Temp);
	SetAlignment(ScratchArena, OldAlignment);
}