/*
 * parser.cpp
 *
 * Parses .scn files
 */


enum token_type
{
	Token_Null,
	Token_Error,
	
	Token_LeftParen,
	Token_RightParen,
	Token_LeftBrace,
	Token_RightBrace,
	Token_Equals,
	Token_Comma,
	Token_Minus,
	
	Token_Number,
	Token_String,
	
	Token_Textures,
	Token_Plane,
	Token_Sphere,
	Token_Triangle,
	Token_Parallelogram,
	Token_Camera,
	Token_Normal,
	Token_Displacement,
	Token_Center,
	Token_Radius,
	Token_Vertices,
	Token_Origin,
	Token_Axes,
	Token_DistToSurface,
	Token_SurfaceWidth,
	Token_SurfaceHeight,
	Token_Color,
	Token_Glossy,
	Token_Translucency,
	Token_Refraction,
	Token_Texture,
	Token_UVMap,
	Token_LookAt,
	Token_SkyColor,
	
	Token_EOF,
};

typedef struct token
{
	token_type Type;
	string String;
	s32 Line;
	s32 Column;
	float Value;
} token;

typedef struct tokenizer
{
	buffer Buffer;
	s32 Line;
	s32 Column;
	token CurrentToken;
	b32 Error;
} tokenizer;

typedef struct keyword_def
{
	string String;
	token_type Type;
} keyword_def;

#define KEYWORD(Word) {ConstString(#Word), Token_##Word}
global keyword_def KeywordDefs[] =
{
	KEYWORD(Textures),
	KEYWORD(Plane),
	KEYWORD(Sphere),
	KEYWORD(Triangle),
	KEYWORD(Parallelogram),
	KEYWORD(Camera),
	KEYWORD(Normal),
	KEYWORD(Displacement),
	KEYWORD(Center),
	KEYWORD(Radius),
	KEYWORD(Vertices),
	KEYWORD(Origin),
	KEYWORD(Axes),
	KEYWORD(DistToSurface),
	KEYWORD(SurfaceWidth),
	KEYWORD(SurfaceHeight),
	KEYWORD(Color),
	KEYWORD(Glossy),
	KEYWORD(Translucency),
	KEYWORD(Refraction),
	KEYWORD(Texture),
	KEYWORD(UVMap),
	KEYWORD(LookAt),
	KEYWORD(SkyColor),
};
#undef KEYWORD

function buffer
LoadEntireFile(const char* FileName, memory_arena* Arena)
{
	buffer Buffer = {};
	FILE* SourceFile = fopen(FileName, "rb");
	
	if (SourceFile)
	{
		fseek(SourceFile, 0, SEEK_END);
		Buffer.Count = ftell(SourceFile);
		fseek(SourceFile, 0, SEEK_SET);
		
		Buffer.Data = PushArray(Arena, Buffer.Count, u8);
		s64 BytesRead = fread(Buffer.Data, sizeof(u8), Buffer.Count, SourceFile);
		
		if (BytesRead != Buffer.Count)
		{
			fprintf(stderr, "Error reading file %s. Read %ld bytes\n", FileName, BytesRead);
		}
	}
	else
	{
		fprintf(stderr, "Error reading file %s\n", FileName);
	}
	
	return Buffer;
}

function b32
HasMoreTokens(tokenizer* Tokenizer)
{
	b32 Result = (Tokenizer->Buffer.Count > 0);
	return Result;
}

function void
Advance(tokenizer* Tokenizer)
{
	if (Tokenizer->Buffer.Data[0] == '\n')
	{
		++Tokenizer->Line;
		Tokenizer->Column = 1;
	}
	else
	{
		++Tokenizer->Column;
	}
	--Tokenizer->Buffer.Count;
	++Tokenizer->Buffer.Data;
}

function b32
AdvanceTo(tokenizer* Tokenizer, u8 Terminator)
{
	while (Tokenizer->Buffer.Count > 0 && Tokenizer->Buffer.Data[0] != Terminator)
	{
		Advance(Tokenizer);
	}
	b32 Found = (Tokenizer->Buffer.Count > 0);
	return Found;
}

function token
ReadNum(tokenizer* Tokenizer)
{
	token Token = {};
	Token.Type = Token_Number;
	Token.String = SubString(Tokenizer->Buffer, 1);
	Token.Line = Tokenizer->Line;
	Token.Column = Tokenizer->Column;
	Token.Value = Tokenizer->Buffer.Data[0] - '0';
	Advance(Tokenizer);
	
	while (Tokenizer->Buffer.Count > 0 && IsNum(Tokenizer->Buffer.Data[0]))
	{
		Token.Value *= 10;
		Token.Value += Tokenizer->Buffer.Data[0] - '0';
		++Token.String.Count;
		Advance(Tokenizer);
	}
	
	if (Tokenizer->Buffer.Count > 0 && Tokenizer->Buffer.Data[0] == '.')
	{
		Advance(Tokenizer);
		++Token.String.Count;
		float PlaceValue = 0.1f;
		while (Tokenizer->Buffer.Count > 0 && IsNum(Tokenizer->Buffer.Data[0]))
		{
			Token.Value += (Tokenizer->Buffer.Data[0] - '0')*PlaceValue;
			PlaceValue *= 0.1f;
			++Token.String.Count;
			Advance(Tokenizer);
		}
	}
	
	return Token;
}

function token
ReadString(tokenizer* Tokenizer)
{
	Advance(Tokenizer);
	token Token = {};
	Token.Type = Token_String;
	Token.String = SubString(Tokenizer->Buffer, 0);
	Token.Line = Tokenizer->Line;
	Token.Column = Tokenizer->Column;
	
	while (Tokenizer->Buffer.Count > 0 && Tokenizer->Buffer.Data[0] != '"' && Tokenizer->Buffer.Data[0] != '\0')
	{
		++Token.String.Count;
		Advance(Tokenizer);
	}
	
	if (Tokenizer->Buffer.Count > 0)
	{
		Tokenizer->Buffer.Data[0] = '\0'; // Null terminate for convenience
		Advance(Tokenizer);
	}
	else
	{
		Tokenizer->Error = true;
	}
	
	return Token;
}

function token
ReadWord(tokenizer* Tokenizer)
{
	token Token = {};
	Token.String = SubString(Tokenizer->Buffer, 1);
	Token.Line = Tokenizer->Line;
	Token.Column = Tokenizer->Column;
	Advance(Tokenizer);
	
	while (Tokenizer->Buffer.Count > 0 && IsAlpha(Tokenizer->Buffer.Data[0]))
	{
		++Token.String.Count;
		Advance(Tokenizer);
	}
	
	for (u64 Index = 0; Index < ArrayCount(KeywordDefs); ++Index)
	{
		if (StringsMatch(Token.String, KeywordDefs[Index].String))
		{
			Token.Type = KeywordDefs[Index].Type;
		}
	}
	
	if (Token.Type == Token_Null)
	{
		Token.Type = Token_Error;
	}
	
	return Token;
}

function token
NextToken(tokenizer* Tokenizer)
{
	token Token = {};
	
	while (Tokenizer->Buffer.Count > 0)
	{
		while (Tokenizer->Buffer.Count > 0 && IsSpace(Tokenizer->Buffer.Data[0]))
		{
			Advance(Tokenizer);
		}
		
		u8 C = Tokenizer->Buffer.Data[0];
		if (C == '#')
		{
			if (Tokenizer->Buffer.Count > 1 && Tokenizer->Buffer.Data[1] == '{')
			{
				AdvanceTo(Tokenizer, '}');
				while (Tokenizer->Buffer.Count > 1 && Tokenizer->Buffer.Data[1] != '#')
				{
					Advance(Tokenizer);
					AdvanceTo(Tokenizer, '}');
				}
				if (Tokenizer->Buffer.Count > 1)
				{
					Advance(Tokenizer);
					Advance(Tokenizer);
				}
				else if (Tokenizer->Buffer.Count > 0)
				{
					Advance(Tokenizer);
				}
			}
			else
			{
				AdvanceTo(Tokenizer, '\n');
				if (Tokenizer->Buffer.Count > 0)
				{
					Advance(Tokenizer);
				}
			}
		}
		else if (C == '(')
		{
			Token.Type = Token_LeftParen;
			Token.String = SubString(Tokenizer->Buffer, 1);
			Token.Line = Tokenizer->Line;
			Token.Column = Tokenizer->Column;
			Advance(Tokenizer);
			break;
		}
		else if (C == ')')
		{
			Token.Type = Token_RightParen;
			Token.String = SubString(Tokenizer->Buffer, 1);
			Token.Line = Tokenizer->Line;
			Token.Column = Tokenizer->Column;
			Advance(Tokenizer);
			break;
		}
		else if (C == '{')
		{
			Token.Type = Token_LeftBrace;
			Token.String = SubString(Tokenizer->Buffer, 1);
			Token.Line = Tokenizer->Line;
			Token.Column = Tokenizer->Column;
			Advance(Tokenizer);
			break;
		}
		else if (C == '}')
		{
			Token.Type = Token_RightBrace;
			Token.String = SubString(Tokenizer->Buffer, 1);
			Token.Line = Tokenizer->Line;
			Token.Column = Tokenizer->Column;
			Advance(Tokenizer);
			break;
		}
		else if (C == '=')
		{
			Token.Type = Token_Equals;
			Token.String = SubString(Tokenizer->Buffer, 1);
			Token.Line = Tokenizer->Line;
			Token.Column = Tokenizer->Column;
			Advance(Tokenizer);
			break;
		}
		else if (C == ',')
		{
			Token.Type = Token_Comma;
			Token.String = SubString(Tokenizer->Buffer, 1);
			Token.Line = Tokenizer->Line;
			Token.Column = Tokenizer->Column;
			Advance(Tokenizer);
			break;
		}
		else if (C == '-')
		{
			Token.Type = Token_Minus;
			Token.String = SubString(Tokenizer->Buffer, 1);
			Token.Line = Tokenizer->Line;
			Token.Column = Tokenizer->Column;
			Advance(Tokenizer);
			break;
		}
		else if (C == '"')
		{
			Token = ReadString(Tokenizer);
			break;
		}
		else if (IsNum(C))
		{
			Token = ReadNum(Tokenizer);
			break;
		}
		else if (IsAlpha(C))
		{
			Token = ReadWord(Tokenizer);
			break;
		}
	}
	
	if (Token.Type == Token_Null)
	{
		Token.Type = Token_EOF;
	}
	
	Tokenizer->CurrentToken = Token;
	return Token;
}

function b32
ExpectToken(tokenizer* Tokenizer, token_type Type)
{
	token Token = NextToken(Tokenizer);
	b32 Success = Token.Type == Type;
	Tokenizer->Error = !Success;
	return Success;
}

function f32
ParseNumber(tokenizer* Tokenizer)
{
	f32 Result = 0;
	b32 Negative = false;
	token Token = NextToken(Tokenizer);
	if (Token.Type == Token_Minus)
	{
		Negative = true;
		Token = NextToken(Tokenizer);
	}
	if (Token.Type == Token_Number)
	{
		Result = Negative ? -Token.Value : Token.Value;
	}
	else
	{
		Tokenizer->Error = true;
	}
	
	return Result;
}
	
function void
ParseObjectProperties(tokenizer* Tokenizer, object* Object)
{
	ExpectToken(Tokenizer, Token_LeftBrace);
	if (Tokenizer->Error)
	{
		fprintf(stderr, "(%d, %d): Invalid object property declaration. Expected '{', got '%.*s'\n", Tokenizer->CurrentToken.Line, Tokenizer->CurrentToken.Column, PrintString(Tokenizer->CurrentToken.String));
	}
	
	b32 ReadColor = false;
	b32 ReadGlossy = false;
	b32 ReadTranslucency = false;
	b32 ReadRefraction = false;
	b32 ReadTexture = false;
	b32 ReadUVMap = false;
	
	while (!Tokenizer->Error)
	{
		token Token = NextToken(Tokenizer);
		if (Token.Type == Token_RightBrace)
		{
			break;
		}
		else if (Token.Type == Token_Color)
		{
			if (!ReadColor)
			{
				ReadColor = true;
				ExpectToken(Tokenizer, Token_Equals);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 R = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 G = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 B = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid object color declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object->Color.R = R;
					Object->Color.G = G;
					Object->Color.B = B;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightBrace)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in object properties declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra color in object properties declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_Glossy)
		{
			if (!ReadGlossy)
			{
				ReadGlossy = true;
				ExpectToken(Tokenizer, Token_Equals);
				f32 Glossy = ParseNumber(Tokenizer);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid object glossy declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object->Glossy = Glossy;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightBrace)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in object properties declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra glossy in object properties declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_Translucency)
		{
			if (!ReadTranslucency)
			{
				ReadTranslucency = true;
				ExpectToken(Tokenizer, Token_Equals);
				f32 Translucency = ParseNumber(Tokenizer);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid object translucency declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object->Translucency = Translucency;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightBrace)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in object properties declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra translucency in object properties declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_Refraction)
		{
			if (!ReadRefraction)
			{
				ReadRefraction = true;
				ExpectToken(Tokenizer, Token_Equals);
				f32 Refraction = ParseNumber(Tokenizer);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid object refraction declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object->Refraction = Refraction;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightBrace)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in object properties declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra refraction in object properties declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_Texture)
		{
			if (!ReadTexture)
			{
				ReadTexture = true;
				ExpectToken(Tokenizer, Token_Equals);
				s32 Index = (s32)ParseNumber(Tokenizer);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid object texture declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object->Texture.Index = Index;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightBrace)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in object properties declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra texture in object properties declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_UVMap)
		{
			if (!ReadUVMap)
			{
				ReadUVMap = true;
				ExpectToken(Tokenizer, Token_Equals);
				ExpectToken(Tokenizer, Token_LeftParen);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 U0 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 V0 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				ExpectToken(Tokenizer, Token_Comma);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 U1 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 V1 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				ExpectToken(Tokenizer, Token_Comma);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 U2 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 V2 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				ExpectToken(Tokenizer, Token_RightParen);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid object uv map declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object->UVMap.VertexUV[0].U = U0;
					Object->UVMap.VertexUV[0].V = V0;
					Object->UVMap.VertexUV[1].U = U1;
					Object->UVMap.VertexUV[1].V = V1;
					Object->UVMap.VertexUV[2].U = U2;
					Object->UVMap.VertexUV[2].V = V2;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightBrace)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in object properties declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra uv map in object properties declaration\n", Token.Line, Token.Column);
			}
		}
		else
		{
			Tokenizer->Error = true;
			fprintf(stderr, "(%d, %d): Invalid token in object properties declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
		}
	}
}

function void
ParsePlaneDecl(tokenizer* Tokenizer, scene* DestScene, memory_arena* Arena)
{
	object Object = {};
	Object.Type = Obj_Plane;
	
	ExpectToken(Tokenizer, Token_LeftParen);
	if (Tokenizer->Error)
	{
		fprintf(stderr, "(%d, %d): Invalid plane declaration. Expected '(', got '%.*s'\n", Tokenizer->CurrentToken.Line, Tokenizer->CurrentToken.Column, PrintString(Tokenizer->CurrentToken.String));
	}
	
	b32 ReadNormal = false;
	b32 ReadDisplacement = false;
	
	while (!Tokenizer->Error)
	{
		token Token = NextToken(Tokenizer);
		if (Token.Type == Token_RightParen)
		{
			break;
		}
		else if (Token.Type == Token_Normal)
		{
			if (!ReadNormal)
			{
				ReadNormal = true;
				ExpectToken(Tokenizer, Token_Equals);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 X = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Y = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Z = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid plane normal declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object.Plane.Normal.X = X;
					Object.Plane.Normal.Y = Y;
					Object.Plane.Normal.Z = Z;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightParen)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in plane declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra normal in plane declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_Displacement)
		{
			if (!ReadDisplacement)
			{
				ReadDisplacement = true;
				ExpectToken(Tokenizer, Token_Equals);
				f32 Displacement = ParseNumber(Tokenizer);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid plane displacement declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object.Plane.Displacement = Displacement;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightParen)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in plane declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra displacement in plane declaration\n", Token.Line, Token.Column);
			}
		}
		else
		{
			Tokenizer->Error = true;
			fprintf(stderr, "(%d, %d): Invalid token in plane declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
		}
	}
	
	ParseObjectProperties(Tokenizer, &Object);
	
	if (!Tokenizer->Error)
	{
		object* DestObject = PushStruct(Arena, object); // Lengthen Array
		*DestObject = Object;
		++DestScene->ObjectCount;
	}
}

function void
ParseSphereDecl(tokenizer* Tokenizer, scene* DestScene, memory_arena* Arena)
{
	object Object = {};
	Object.Type = Obj_Sphere;
	
	ExpectToken(Tokenizer, Token_LeftParen);
	if (Tokenizer->Error)
	{
		fprintf(stderr, "(%d, %d): Invalid sphere declaration. Expected '(', got '%.*s'\n", Tokenizer->CurrentToken.Line, Tokenizer->CurrentToken.Column, PrintString(Tokenizer->CurrentToken.String));
	}
	
	b32 ReadCenter = false;
	b32 ReadRadius = false;
	
	while (!Tokenizer->Error)
	{
		token Token = NextToken(Tokenizer);
		if (Token.Type == Token_RightParen)
		{
			break;
		}
		else if (Token.Type == Token_Center)
		{
			if (!ReadCenter)
			{
				ReadCenter = true;
				ExpectToken(Tokenizer, Token_Equals);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 X = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Y = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Z = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid sphere center declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object.Sphere.Center.X = X;
					Object.Sphere.Center.Y = Y;
					Object.Sphere.Center.Z = Z;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightParen)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in sphere declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra normal in sphere declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_Radius)
		{
			if (!ReadRadius)
			{
				ReadRadius = true;
				ExpectToken(Tokenizer, Token_Equals);
				f32 Radius = ParseNumber(Tokenizer);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid sphere radius declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object.Sphere.Radius = Radius;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightParen)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in sphere declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra displacement in sphere declaration\n", Token.Line, Token.Column);
			}
		}
		else
		{
			Tokenizer->Error = true;
			fprintf(stderr, "(%d, %d): Invalid token in sphere declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
		}
	}
	
	ParseObjectProperties(Tokenizer, &Object);
	
	if (!Tokenizer->Error)
	{
		object* DestObject = PushStruct(Arena, object); // Lengthen Array
		*DestObject = Object;
		++DestScene->ObjectCount;
	}
}

function void
ParseTriangleDecl(tokenizer* Tokenizer, scene* DestScene, memory_arena* Arena)
{
	object Object = {};
	Object.Type = Obj_Triangle;
	
	ExpectToken(Tokenizer, Token_LeftParen);
	if (Tokenizer->Error)
	{
		fprintf(stderr, "(%d, %d): Invalid triangle declaration. Expected '(', got '%.*s'\n", Tokenizer->CurrentToken.Line, Tokenizer->CurrentToken.Column, PrintString(Tokenizer->CurrentToken.String));
	}
	
	b32 ReadVertices = false;
	
	while (!Tokenizer->Error)
	{
		token Token = NextToken(Tokenizer);
		if (Token.Type == Token_RightParen)
		{
			break;
		}
		else if (Token.Type == Token_Vertices)
		{
			if (!ReadVertices)
			{
				ReadVertices = true;
				ExpectToken(Tokenizer, Token_Equals);
				ExpectToken(Tokenizer, Token_LeftParen);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 X0 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Y0 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Z0 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				ExpectToken(Tokenizer, Token_Comma);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 X1 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Y1 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Z1 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				ExpectToken(Tokenizer, Token_Comma);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 X2 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Y2 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Z2 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				ExpectToken(Tokenizer, Token_RightParen);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid triangle vertices declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object.Triangle.Vertex[0].X = X0;
					Object.Triangle.Vertex[0].Y = Y0;
					Object.Triangle.Vertex[0].Z = Z0;
					Object.Triangle.Vertex[1].X = X1;
					Object.Triangle.Vertex[1].Y = Y1;
					Object.Triangle.Vertex[1].Z = Z1;
					Object.Triangle.Vertex[2].X = X2;
					Object.Triangle.Vertex[2].Y = Y2;
					Object.Triangle.Vertex[2].Z = Z2;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightParen)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in triangle declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra vertices in triangle declaration\n", Token.Line, Token.Column);
			}
		}
		else
		{
			Tokenizer->Error = true;
			fprintf(stderr, "(%d, %d): Invalid token in triangle declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
		}
	}
	
	ParseObjectProperties(Tokenizer, &Object);
	
	if (!Tokenizer->Error)
	{
		object* DestObject = PushStruct(Arena, object); // Lengthen Array
		*DestObject = Object;
		++DestScene->ObjectCount;
	}
}

function void
ParseParallelogramDecl(tokenizer* Tokenizer, scene* DestScene, memory_arena* Arena)
{
	object Object = {};
	Object.Type = Obj_Parallelogram;
	
	ExpectToken(Tokenizer, Token_LeftParen);
	if (Tokenizer->Error)
	{
		fprintf(stderr, "(%d, %d): Invalid parallelogram declaration. Expected '(', got '%.*s'\n", Tokenizer->CurrentToken.Line, Tokenizer->CurrentToken.Column, PrintString(Tokenizer->CurrentToken.String));
	}
	
	b32 ReadOrigin = false;
	b32 ReadAxes = false;
	
	while (!Tokenizer->Error)
	{
		token Token = NextToken(Tokenizer);
		if (Token.Type == Token_RightParen)
		{
			break;
		}
		else if (Token.Type == Token_Origin)
		{
			if (!ReadOrigin)
			{
				ReadOrigin = true;
				ExpectToken(Tokenizer, Token_Equals);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 X = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Y = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Z = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid parallelogram origin declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object.Parallelogram.Origin.X = X;
					Object.Parallelogram.Origin.Y = Y;
					Object.Parallelogram.Origin.Z = Z;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightParen)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in parallelogram declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra origin in parallelogram declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_Axes)
		{
			if (!ReadAxes)
			{
				ReadAxes = true;
				ExpectToken(Tokenizer, Token_Equals);
				ExpectToken(Tokenizer, Token_LeftParen);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 X0 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Y0 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Z0 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				ExpectToken(Tokenizer, Token_Comma);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 X1 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Y1 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Z1 = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				ExpectToken(Tokenizer, Token_RightParen);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid parallelogram axes declaration\n", Token.Line, Token.Column);
				}
				else
				{
					Object.Parallelogram.XAxis.X = X0;
					Object.Parallelogram.XAxis.Y = Y0;
					Object.Parallelogram.XAxis.Z = Z0;
					Object.Parallelogram.YAxis.X = X1;
					Object.Parallelogram.YAxis.Y = Y1;
					Object.Parallelogram.YAxis.Z = Z1;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightParen)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in parallelogram declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra axes in parallelogram declaration\n", Token.Line, Token.Column);
			}
		}
		else
		{
			Tokenizer->Error = true;
			fprintf(stderr, "(%d, %d): Invalid token in parallelogram declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
		}
	}
	
	ParseObjectProperties(Tokenizer, &Object);
	
	if (!Tokenizer->Error)
	{
		object* DestObject = PushStruct(Arena, object); // Lengthen Array
		*DestObject = Object;
		++DestScene->ObjectCount;
	}
}

function void
ParseCameraDecl(tokenizer* Tokenizer, scene* DestScene, memory_arena* Arena)
{
	ExpectToken(Tokenizer, Token_LeftParen);
	if (Tokenizer->Error)
	{
		fprintf(stderr, "(%d, %d): Invalid camera declaration. Expected '(', got '%.*s'\n", Tokenizer->CurrentToken.Line, Tokenizer->CurrentToken.Column, PrintString(Tokenizer->CurrentToken.String));
	}
	
	// Default camera
	DestScene->Camera.Origin = {0, 0, 0};
	DestScene->Camera.XAxis = {1, 0, 0};
	DestScene->Camera.YAxis = {0, 0, 1};
	DestScene->Camera.ZAxis = {0, -1, 0};
	DestScene->Camera.DistToSurface = 1.0f;
	DestScene->Camera.SurfaceWidth = 1.0f;
	DestScene->Camera.SurfaceHeight = 1.0f;
	
	b32 ReadOrigin = false;
	b32 ReadDistToSurface = false;
	b32 ReadSurfaceWidth = false;
	b32 ReadSurfaceHeight = false;
	
	while (!Tokenizer->Error)
	{
		token Token = NextToken(Tokenizer);
		if (Token.Type == Token_RightParen)
		{
			break;
		}
		else if (Token.Type == Token_Origin)
		{
			if (!ReadOrigin)
			{
				ReadOrigin = true;
				ExpectToken(Tokenizer, Token_Equals);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 X = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Y = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Z = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid camera origin declaration\n", Token.Line, Token.Column);
				}
				else
				{
					DestScene->Camera.Origin.X = X;
					DestScene->Camera.Origin.Y = Y;
					DestScene->Camera.Origin.Z = Z;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightParen)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in camera declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra origin in camera declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_DistToSurface)
		{
			if (!ReadDistToSurface)
			{
				ReadDistToSurface = true;
				ExpectToken(Tokenizer, Token_Equals);
				f32 DistToSurface = ParseNumber(Tokenizer);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid camera dist-to-surface declaration\n", Token.Line, Token.Column);
				}
				else
				{
					DestScene->Camera.DistToSurface = DistToSurface;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightParen)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in camera declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra dist-to-surface in camera declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_SurfaceWidth)
		{
			if (!ReadSurfaceWidth)
			{
				ReadSurfaceWidth = true;
				ExpectToken(Tokenizer, Token_Equals);
				f32 SurfaceWidth = ParseNumber(Tokenizer);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid camera surface width declaration\n", Token.Line, Token.Column);
				}
				else
				{
					DestScene->Camera.SurfaceWidth = SurfaceWidth;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightParen)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in camera declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra surface width in camera declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_SurfaceHeight)
		{
			if (!ReadSurfaceHeight)
			{
				ReadSurfaceHeight = true;
				ExpectToken(Tokenizer, Token_Equals);
				f32 SurfaceHeight = ParseNumber(Tokenizer);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid camera surface height declaration\n", Token.Line, Token.Column);
				}
				else
				{
					DestScene->Camera.SurfaceHeight = SurfaceHeight;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightParen)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in camera declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra surface height in camera declaration\n", Token.Line, Token.Column);
			}
		}
		else
		{
			Tokenizer->Error = true;
			fprintf(stderr, "(%d, %d): Invalid token in camera declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
		}
	}
	
	ExpectToken(Tokenizer, Token_LeftBrace);
	if (Tokenizer->Error)
	{
		fprintf(stderr, "(%d, %d): Invalid camera declaration. Expected '{', got '%.*s'\n", Tokenizer->CurrentToken.Line, Tokenizer->CurrentToken.Column, PrintString(Tokenizer->CurrentToken.String));
	}
	
	b32 ReadLookAt = false;
	b32 ReadSkyColor = false;
	
	while (!Tokenizer->Error)
	{
		token Token = NextToken(Tokenizer);
		if (Token.Type == Token_RightBrace)
		{
			break;
		}
		else if (Token.Type == Token_LookAt)
		{
			if (!ReadLookAt)
			{
				ReadLookAt = true;
				ExpectToken(Tokenizer, Token_Equals);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 X = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Y = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 Z = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid camera look-at declaration\n", Token.Line, Token.Column);
				}
				else
				{
					v3 Destination = {X, Y, Z};
					DestScene->Camera.ZAxis = NormOrDefault(DestScene->Camera.Origin - Destination, (v3){0, -1.0f, 0});
					DestScene->Camera.XAxis = NormOrDefault(Cross((v3){0, 0, 1.0f}, DestScene->Camera.ZAxis), (v3){1.0f, 0, 0});
					DestScene->Camera.YAxis = NormOrDefault(Cross(DestScene->Camera.ZAxis, DestScene->Camera.XAxis), (v3){0, 0, 1.0f});
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightBrace)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in camera declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra look-at in camera declaration\n", Token.Line, Token.Column);
			}
		}
		else if (Token.Type == Token_SkyColor)
		{
			if (!ReadSkyColor)
			{
				ReadSkyColor = true;
				ExpectToken(Tokenizer, Token_Equals);
				ExpectToken(Tokenizer, Token_LeftParen);
				f32 R = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 G = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_Comma);
				f32 B = ParseNumber(Tokenizer);
				ExpectToken(Tokenizer, Token_RightParen);
				
				if (Tokenizer->Error)
				{
					fprintf(stderr, "(%d, %d): Invalid camera sky color declaration\n", Token.Line, Token.Column);
				}
				else
				{
					DestScene->SkyColor.R = R;
					DestScene->SkyColor.G = G;
					DestScene->SkyColor.B = B;
				}
				Token = NextToken(Tokenizer);
				if (Token.Type == Token_RightBrace)
				{
					break;
				}
				else if (Token.Type != Token_Comma)
				{
					Tokenizer->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in camera declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
			}
			else
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Extra sky color in camera declaration\n", Token.Line, Token.Column);
			}
		}
		else
		{
			Tokenizer->Error = true;
			fprintf(stderr, "(%d, %d): Invalid token in camera declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
		}
	}
}

function void
LoadTextures(tokenizer* Tokenizer, scene* DestScene, memory_arena* Arena)
{
	s32 TextureCount = 0;
	ExpectToken(Tokenizer, Token_LeftBrace);
	tokenizer FirstPassTok = *Tokenizer;
	tokenizer* FirstPass = &FirstPassTok;
	while (HasMoreTokens(FirstPass) && !FirstPass->Error)
	{
		token Token = NextToken(FirstPass);
		if (Token.Type == Token_RightBrace)
		{
			break;
		}
		else if (Token.Type == Token_Number)
		{
			s32 Index = (s32)Token.Value;
			if ((f32)Index != Token.Value)
			{
				FirstPass->Error = true;
				fprintf(stderr, "(%d, %d): Texture index not an integer: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
			}
			else if (Index >= 1)
			{
				ExpectToken(FirstPass, Token_Equals);
				Token = NextToken(FirstPass);
				if (Token.Type != Token_String)
				{
					FirstPass->Error = true;
					fprintf(stderr, "(%d, %d): Invalid token in texture declaration. Expected string, got '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
				}
				
				if (!FirstPass->Error)
				{
					if (Index > TextureCount)
					{
						TextureCount = Index;
					}
					Token = NextToken(FirstPass);
					if (Token.Type == Token_RightBrace)
					{
						break;
					}
					else if (Token.Type != Token_Comma)
					{
						FirstPass->Error = true;
						fprintf(stderr, "(%d, %d): Invalid token in textures declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
					}
				}
			}
			else
			{
				FirstPass->Error = true;
				fprintf(stderr, "(%d, %d): Texture index out of range: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
			}
		}
		else
		{
			FirstPass->Error = true;
			fprintf(stderr, "(%d, %d): Invalid token in textures declaration: '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
		}
	}
	
	Tokenizer->Error = FirstPass->Error;
	
	if (!Tokenizer->Error)
	{
		DestScene->TextureCount = TextureCount;
		if (TextureCount > 0)
		{
			DestScene->Textures = PushArray(Arena, TextureCount, surface);
		}
	}
	
	while (HasMoreTokens(Tokenizer) && !Tokenizer->Error)
	{
		token Token = NextToken(Tokenizer);
		if (Token.Type == Token_RightBrace)
		{
			break;
		}
		else if (Token.Type == Token_Number)
		{
			s32 Index = (s32)Token.Value - 1;
			ExpectToken(Tokenizer, Token_Equals);
			Token = NextToken(Tokenizer);
			DestScene->Textures[Index] = LoadTGA((const char*)Token.String.Data, Arena);
			if (DestScene->Textures[Index].Pixels == 0)
			{
				Tokenizer->Error = true;
				fprintf(stderr, "(%d, %d): Could not load texture from file '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
			}
			Token = NextToken(Tokenizer);
			if (Token.Type == Token_RightBrace)
			{
				break;
			}
		}
	}
}

function b32
LoadSceneFromFile(const char* FileName, scene* DestScene, memory_arena* Arena, memory_arena* ScratchArena)
{
	b32 Success = true;
	temporary_memory Temp = BeginTemporaryMemory(ScratchArena);
	buffer SceneBuffer = LoadEntireFile(FileName, ScratchArena);
	
	if (SceneBuffer.Data)
	{
		tokenizer Tokenizer = {};
		Tokenizer.Buffer = SceneBuffer;
		Tokenizer.Line = 1;
		Tokenizer.Column = 1;
		
		// First read texture data if available
		
		token Token = NextToken(&Tokenizer);
		if (Token.Type == Token_Textures)
		{
			LoadTextures(&Tokenizer, DestScene, Arena);
			Token = NextToken(&Tokenizer);
		}
		
		if (!Tokenizer.Error)
		{
			s64 OldAlignment = Arena->Alignment;
			SetAlignment(Arena, 16);
			DestScene->Objects = PushArray(Arena, 0, object);
			SetAlignment(Arena, 1);
			
			while (HasMoreTokens(&Tokenizer) && !Tokenizer.Error)
			{
				if (Token.Type == Token_Plane)
				{
					ParsePlaneDecl(&Tokenizer, DestScene, Arena);
				}
				else if (Token.Type == Token_Sphere)
				{
					ParseSphereDecl(&Tokenizer, DestScene, Arena);
				}
				else if (Token.Type == Token_Triangle)
				{
					ParseTriangleDecl(&Tokenizer, DestScene, Arena);
				}
				else if (Token.Type == Token_Parallelogram)
				{
					ParseParallelogramDecl(&Tokenizer, DestScene, Arena);
				}
				else if (Token.Type == Token_Camera)
				{
					ParseCameraDecl(&Tokenizer, DestScene, Arena);
				}
				else if (Token.Type == Token_EOF)
				{
					break;
				}
				else
				{
					Tokenizer.Error = true;
					fprintf(stderr, "(%d, %d): Expected object declaration, got '%.*s'\n", Token.Line, Token.Column, PrintString(Token.String));
					break;
				}
				Token = NextToken(&Tokenizer);
			}
			
			SetAlignment(Arena, OldAlignment);
		}
		
		Success = !Tokenizer.Error;
		EndTemporaryMemory(Temp);
	}
	else
	{
		Success = false;
	}
	
	return Success;
}
