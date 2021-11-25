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
	assert(Arena.Start);
	
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
	assert(NewAllocStart + Size - Arena->Start <= Arena->Capacity);
	Arena->Allocated = NewAllocStart + Size - Arena->Start;
	
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
