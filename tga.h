/*
 * tga.h
 *
 * For writing the surface out to a TARGA file
 */

#pragma pack(push, 1)

typedef struct color_map_specification
{
	u16 FirstEntryIndex;
	u16 Length;
	u8 EntrySize;
} color_map_specification;

typedef struct image_specification
{
	u16 XOrigin;
	u16 YOrigin;
	u16 Width;
	u16 Height;
	u8 PixelDepth;
	u8 ImageDescriptor;
} image_specification;

enum tga_color_map_type
{
	TGA_NoColorMap,
	TGA_ColorMap,
};

enum tga_image_type
{
	TGA_NoImageData,
	TGA_UncompressedColorMapped,
	TGA_UncompressedTrueColor,
	TGA_UncompressedBlackAndWhite,
	TGA_RLEColorMapped,
	TGA_RLETrueColor,
	TGA_RLEBlackAndWhite,
};

typedef struct tga_header
{
	u8 IDLength;
	u8 ColorMapType;
	u8 ImageType;
	color_map_specification ColorMapSpecification;
	image_specification ImageSpecification;
} tga_header;

#pragma pack(pop)

function color
ColorFromRGB24(u8 R, u8 G, u8 B)
{
#ifdef NO_GAMMA_CORRECTION
	color Result =
	{
		(f32)R/255.0f,
		(f32)G/255.0f,
		(f32)B/255.0f,
	};
#else
	// Approximate gamma correction
	color Result =
	{
		(f32)(R*R)/65025.0f,
		(f32)(G*G)/65025.0f,
		(f32)(B*B)/65025.0f,
	};
#endif
	return Result;
}

function u8
U8FromColorComponent(f32 C)
{
#ifdef NO_GAMMA_CORRECTION
	u8 Result = Clamp01(C)*255.0f;
#else
	// Approximate gamma correction
	u8 Result = sqrtf(Clamp01(C))*255.0f;
#endif
	return Result;
}

function surface
LoadTGA(const char* FileName, memory_arena* Arena)
{
	surface Surface = {};
	
	buffer Buffer = {};
	FILE* SourceFile = fopen(FileName, "rb");
	
	if (SourceFile)
	{
		fseek(SourceFile, 0, SEEK_END);
		Buffer.Count = ftell(SourceFile);
		fseek(SourceFile, 0, SEEK_SET);
		
		b32 Error = false;
		temporary_memory OriginalMem = BeginTemporaryMemory(Arena);
		
		Surface.Pixels = (color*)PushArray(Arena, Buffer.Count - sizeof(tga_header), f32);
		
		temporary_memory Temp = BeginTemporaryMemory(Arena);
		
		Buffer.Data = PushArray(Arena, Buffer.Count, u8);
		s64 BytesRead = fread(Buffer.Data, sizeof(u8), Buffer.Count, SourceFile);
		
		if (BytesRead == Buffer.Count)
		{
			tga_header* Header = Consume(&Buffer, tga_header);
			if (Header->IDLength == 0 &&
				Header->ColorMapType == TGA_NoColorMap &&
				Header->ImageType == TGA_UncompressedTrueColor)
			{
				Surface.Width = Header->ImageSpecification.Width;
				Surface.Height = Header->ImageSpecification.Height;
				
				u8* SourceColor = Buffer.Data;
				color* DestPixel = Surface.Pixels;
				for (s32 Y = 0; Y < Surface.Height; ++Y)
				{
					for (s32 X = 0; X < Surface.Width; ++X)
					{
						u8 B = *SourceColor++;
						u8 G = *SourceColor++;
						u8 R = *SourceColor++;
						*DestPixel++ = ColorFromRGB24(R, G, B);
					}
				}
			}
			else
			{
				Error = true;
				fprintf(stderr, "Error reading file %s: Unsupported TARGA format\n", FileName);
			}
		}
		else
		{
			Error = true;
			fprintf(stderr, "Error reading file %s. Read %ld bytes\n", FileName, BytesRead);
		}
		
		EndTemporaryMemory(Temp);
		
		if (Error)
		{
			Surface.Pixels = 0;
			EndTemporaryMemory(OriginalMem);
		}
		else
		{
			KeepTemporaryMemory(OriginalMem);
		}
	}
	else
	{
		fprintf(stderr, "Error reading file %s\n", FileName);
	}
	
	return Surface;
}

function b32
WriteTGA(surface* Surface, const char* FileName, memory_arena* Arena)
{
	b32 Success = true;
	FILE* DestFile = fopen(FileName, "wb");
	
	if (DestFile)
	{
		temporary_memory Temp = BeginTemporaryMemory(Arena);
		tga_header Header =
		{
			.IDLength = 0,
			.ColorMapType = TGA_NoColorMap,
			.ImageType = TGA_UncompressedTrueColor,
			.ColorMapSpecification = {0},
			.ImageSpecification =
			{
				.XOrigin = 0,
				.YOrigin = 0,
				.Width = (u16)Surface->Width,
				.Height = (u16)Surface->Height,
				.PixelDepth = 24,
				.ImageDescriptor = 0x00,
			},
		};
		
		u64 ImageDataSize = Surface->Width * Surface->Height * 3;
		u8* ImageDataBuffer = PushArray(Arena, ImageDataSize, u8);
		
		if (ImageDataBuffer)
		{
			u8* DestColor = ImageDataBuffer;
			color* Pixel = Surface->Pixels;
			for (s32 Y = 0; Y < Surface->Height; ++Y)
			{
				for (s32 X = 0; X < Surface->Width; ++X)
				{
					*DestColor++ = U8FromColorComponent(Pixel->B);
					*DestColor++ = U8FromColorComponent(Pixel->G);
					*DestColor++ = U8FromColorComponent(Pixel->R);
					++Pixel;
				}
			}
			
			Success = (fwrite(&Header, sizeof(tga_header), 1, DestFile) == 1);
			if (Success)
			{
				Success = (fwrite(ImageDataBuffer, ImageDataSize, 1, DestFile) == 1);
			}
		}
		EndTemporaryMemory(Temp);
		fclose(DestFile);
	}
	
	return Success;
}
