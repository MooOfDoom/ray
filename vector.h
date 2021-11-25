/*
 * vector.h
 *
 * Vector math
 */

typedef union v2
{
	struct
	{
		f32 X;
		f32 Y;
	};
	struct
	{
		f32 U;
		f32 V;
	};
	f32 E[2];
} v2, uv;

typedef union v3
{
	struct
	{
		f32 X;
		f32 Y;
		f32 Z;
	};
	struct
	{
		f32 R;
		f32 G;
		f32 B;
	};
	struct
	{
		f32 U;
		f32 V;
		f32 W;
	};
	f32 E[3];
} v3, color;

// Scalar

function f32
Lerp(f32 A, f32 T, f32 B)
{
	f32 Result = A*(1.0f - T) + B*T;
	return Result;
}

// v2

function b32
operator==(const v2& A, const v2& B)
{
	b32 Result = (A.X == B.X && A.Y == B.Y);
	return Result;
}

function b32
operator!=(const v2& A, const v2& B)
{
	b32 Result = (A.X != B.X || A.Y != B.Y);
	return Result;
}

function v2
operator+(const v2& A, const v2& B)
{
	v2 Result =
	{
		A.X + B.X,
		A.Y + B.Y,
	};
	return Result;
}

function v2
operator-(const v2& A, const v2& B)
{
	v2 Result =
	{
		A.X - B.X,
		A.Y - B.Y,
	};
	return Result;
}

function v2
operator-(const v2& A)
{
	v2 Result =
	{
		-A.X,
		-A.Y,
	};
	return Result;
}

function v2
operator*(v2 A, f32 R)
{
	v2 Result =
	{
		R*A.X,
		R*A.Y,
	};
	return Result;
}

function v2
operator/(v2 A, f32 R)
{
	v2 Result =
	{
		A.X/R,
		A.Y/R,
	};
	return Result;
}

function v2
operator*(f32 R, v2 A)
{
	v2 Result =
	{
		R*A.X,
		R*A.Y,
	};
	return Result;
}

function f32
Dot(v2 A, v2 B)
{
	f32 Result = A.X*B.X + A.Y*B.Y;
	return Result;
}

function f32
LengthSq(v2 A)
{
	f32 Result = Dot(A, A);
	return Result;
}

function f32
Length(v2 A)
{
	f32 Result = sqrtf(LengthSq(A));
	return Result;
}

function v2
NormOrZero(v2 A)
{
	v2 Result = {0};
	f32 Len = Length(A);
	if (Len > EPSILON)
	{
		Result = A/Len;
	}
	return Result;
}

function v2
NormOrDefault(v2 A, v2 Default)
{
	v2 Result = Default;
	f32 Len = Length(A);
	if (Len > EPSILON)
	{
		Result = A/Len;
	}
	return Result;
}

function v2
Perp(v2 A)
{
	v2 Result =
	{
		-A.Y,
		A.X,
	};
	return Result;
}

function v2
Lerp(v2 A, f32 T, v2 B)
{
	v2 Result = A*(1.0f - T) + B*T;
	return Result;
}

// v3

function b32
operator==(const v3& A, const v3& B)
{
	b32 Result = (A.X == B.X && A.Y == B.Y && A.Z == B.Z);
	return Result;
}

function b32
operator!=(const v3& A, const v3& B)
{
	b32 Result = (A.X != B.X || A.Y != B.Y || A.Z != B.Z);
	return Result;
}

function v3
operator+(const v3& A, const v3& B)
{
	v3 Result =
	{
		A.X + B.X,
		A.Y + B.Y,
		A.Z + B.Z,
	};
	return Result;
}

function v3
operator-(const v3& A, const v3& B)
{
	v3 Result =
	{
		A.X - B.X,
		A.Y - B.Y,
		A.Z - B.Z,
	};
	return Result;
}

function v3
operator-(const v3& A)
{
	v3 Result =
	{
		-A.X,
		-A.Y,
		-A.Z,
	};
	return Result;
}

function v3
operator*(v3 A, f32 R)
{
	v3 Result =
	{
		R*A.X,
		R*A.Y,
		R*A.Z,
	};
	return Result;
}

function v3
operator/(v3 A, f32 R)
{
	v3 Result =
	{
		A.X/R,
		A.Y/R,
		A.Z/R,
	};
	return Result;
}

function v3
operator*(f32 R, v3 A)
{
	v3 Result =
	{
		R*A.X,
		R*A.Y,
		R*A.Z,
	};
	return Result;
}

function f32
Dot(v3 A, v3 B)
{
	f32 Result = A.X*B.X + A.Y*B.Y + A.Z*B.Z;
	return Result;
}

function f32
LengthSq(v3 A)
{
	f32 Result = Dot(A, A);
	return Result;
}

function f32
Length(v3 A)
{
	f32 Result = sqrtf(LengthSq(A));
	return Result;
}

function v3
NormOrZero(v3 A)
{
	v3 Result = {0};
	f32 Len = Length(A);
	if (Len > EPSILON)
	{
		Result = A/Len;
	}
	return Result;
}

function v3
NormOrDefault(v3 A, v3 Default)
{
	v3 Result = Default;
	f32 Len = Length(A);
	if (Len > EPSILON)
	{
		Result = A/Len;
	}
	return Result;
}

function v3
Cross(v3 A, v3 B)
{
	v3 Result =
	{
		A.Y*B.Z - A.Z*B.Y,
		A.Z*B.X - A.X*B.Z,
		A.X*B.Y - A.Y*B.X,
	};
	return Result;
}

function v3
Lerp(v3 A, f32 T, v3 B)
{
	v3 Result = A*(1.0f - T) + B*T;
	return Result;
}
