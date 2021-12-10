/*
 * memory.h
 *
 * For everything to do with memory management
 */

typedef struct memory_arena
{
	s64 Capacity;
	s64 Allocated;
	u8* Start;
	s64 Alignment;
	s32 TempCount;
} memory_arena;

typedef struct temporary_memory
{
	memory_arena* Arena;
	s64 InitialAllocated;
	s32 TempCount;
} temporary_memory;

function memory_arena
MakeArena(s64 Capacity, s64 Alignment)
{
	assert(Capacity > 0);
	assert(Alignment > 0);
	assert(IsPow2(Alignment));
	memory_arena Arena =
	{
		.Capacity = Capacity,
		.Allocated = 0,
		.Start = (u8*)calloc(Capacity, 1),
		.Alignment = Alignment,
		.TempCount = 0,
	};
	
	if(!Arena.Start)
	{
		fprintf(stderr, "Fatal Error: Ran out of memory attempting to create an arena for %ld bytes.\n", Capacity);
		exit(1);
	}
	
	return Arena;
}

function void
SetAlignment(memory_arena* Arena, s64 Alignment)
{
	assert(Alignment > 0);
	assert(IsPow2(Alignment));
	Arena->Alignment = Alignment;
}

#define PushStruct(Arena, Struct) (Struct*)PushSize((Arena), sizeof(Struct))
#define PushArray(Arena, Count, Struct) (Struct*)PushSize((Arena), (Count)*sizeof(Struct))

function void*
PushSize(memory_arena* Arena, s64 Size)
{
	u8* NewAllocStart = (u8*)AlignUp((s64)(Arena->Start + Arena->Allocated), Arena->Alignment);
	if(NewAllocStart + Size - Arena->Start > Arena->Capacity)
	{
		fprintf(stderr, "Fatal Error: Ran out of memory attempting to allocate %ld bytes.\n", Size);
		exit(1);
	}
	Arena->Allocated = NewAllocStart + Size - Arena->Start;
	
	return NewAllocStart;
}

function b32
HasRoom(memory_arena* Arena, s64 Size)
{
	u8* NewAllocStart = (u8*)AlignUp((s64)(Arena->Start + Arena->Allocated), Arena->Alignment);
	b32 Result = (NewAllocStart + Size - Arena->Start <= Arena->Capacity);
	return Result;
}

#define PushCopyArray(Arena, Count, Array) PushCopyArray_(Arena, Count, sizeof(Array[0]), (u8*)Array)

function u8*
PushCopyArray_(memory_arena* Arena, s64 Count, s64 ElemSize, u8* SourceArray)
{
	s64 Size = Count * ElemSize;
	u8* NewAllocStart = (u8*)PushSize(Arena, Size);
	for (s64 Index = 0; Index < Size; ++Index)
	{
		NewAllocStart[Index] = SourceArray[Index];
	}
	return NewAllocStart;
}

function temporary_memory
BeginTemporaryMemory(memory_arena* Arena)
{
	temporary_memory Temp =
	{
		.Arena = Arena,
		.InitialAllocated = Arena->Allocated,
		.TempCount = Arena->TempCount++,
	};
	return Temp;
}

function void
EndTemporaryMemory(temporary_memory Temp)
{
	Temp.Arena->Allocated = Temp.InitialAllocated;
	--Temp.Arena->TempCount;
	assert(Temp.TempCount == Temp.Arena->TempCount);
}

function void
KeepTemporaryMemory(temporary_memory Temp)
{
	--Temp.Arena->TempCount;
	assert(Temp.TempCount == Temp.Arena->TempCount);
}
