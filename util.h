/*
 * util.h
 *
 * For useful, general purpose tools
 */

#define function static
#define global static

#define OffsetOf(Struct, Member) (((u8*)&((Struct*)0)->Member) - (u8*)0)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

function s64
AlignUp(s64 Value, s64 Alignment)
{
	s64 Result = ((Value + Alignment - 1) / Alignment) * Alignment;
	return Result;
}

function b32
IsPow2(s64 Value)
{
	b32 Result = (Value != 0) && ((Value & (Value - 1)) == 0);
	return Result;
}

function f32
Clamp(f32 MinV, f32 Value, f32 MaxV)
{
	f32 Result = Value;
	if (Result < MinV)
	{
		Result = MinV;
	}
	else if (Result > MaxV)
	{
		Result = MaxV;
	}
	return Result;
}

function f32
Clamp01(f32 Value)
{
	f32 Result = Clamp(0, Value, 1.0f);
	return Result;
}

function f32
Abs(f32 Value)
{
	f32 Result = Value;
	if (Result < 0)
	{
		Result = -Value;
	}
	return Result;
}

function f32
Minimum(f32 A, f32 B)
{
	f32 Result = A;
	if (A > B)
	{
		Result = B;
	}
	return Result;
}

function f32
Maximum(f32 A, f32 B)
{
	f32 Result = A;
	if (A < B)
	{
		Result = B;
	}
	return Result;
}
