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
Clamp01(f32 Value)
{
	f32 Result = Value;
	if (Result < 0)
	{
		Result = 0;
	}
	else if (Result > 1.0f)
	{
		Result = 1.0f;
	}
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
