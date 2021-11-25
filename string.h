/*
 * string.h
 *
 * String functions
 */

typedef struct string
{
	s64 Count;
	u8* Data;
} string, buffer;

#define ConstString(Str) (string){sizeof(Str) - 1, (u8*)(Str)}
#define PrintString(Str) (s32)(Str.Count), Str.Data

#define Consume(Buf, Struct) (Struct*)ConsumeSize((Buf), sizeof(Struct))

function u8*
ConsumeSize(buffer* Buffer, s64 ByteCount)
{
	u8* Result = Buffer->Data;
	Buffer->Data += ByteCount;
	Buffer->Count -= ByteCount;
	return Result;
}

function s64
CStrLen(const char* CStr)
{
	s64 Result = 0;
	while (*CStr++)
	{
		++Result;
	}
	return Result;
}

function b32
CStrEq(const char* A, const char* B)
{
	while (*A && (*A == *B))
	{
		++A;
		++B;
	}
	b32 Result = (*A == *B);
	return Result;
}

#define WrapZ(CStr) (string){CStrLen(CStr), (u8*)CStr}

function b32
IsSpace(u8 C)
{
	b32 Result = (C == ' ' || C == '\t' || C == '\n' || C == '\r');
	return Result;
}

function b32
IsNum(u8 C)
{
	b32 Result = (C >= '0' && C <= '9');
	return Result;
}

function b32
IsAlpha(u8 C)
{
	b32 Result = (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z');
	return Result;
}

function string
SubString(string A, s64 Count)
{
	string Result = A;
	if (Result.Count > Count)
	{
		Result.Count = Count;
	}
	return Result;
}

function b32
StartsWith(string A, string B)
{
	b32 Result = (A.Count >= B.Count);
	if (Result)
	{
		for (s64 Index = 0; Index < B.Count; ++Index)
		{
			if (A.Data[Index] != B.Data[Index])
			{
				Result = false;
				break;
			}
		}
	}
	return Result;
}

function b32
StringsMatch(string A, string B)
{
	b32 Result = (A.Count == B.Count);
	if (Result)
	{
		for (s64 Index = 0; Index < B.Count; ++Index)
		{
			if (A.Data[Index] != B.Data[Index])
			{
				Result = false;
				break;
			}
		}
	}
	return Result;
}
