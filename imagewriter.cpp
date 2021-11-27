/*
 * Ray
 *
 * Usage: ray <scene.scn> <render.tga>
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define EPSILON 0.00001f

#include "types.h"
#include "util.h"
#include "memory.h"
#include "string.h"
#include "vector.h"
#include "scene.h"
#include "tga.h"

#include "imagedata.h"

int
main(int ArgCount, char** Args)
{
	b32 Success = true;
	
	memory_arena Arena = MakeArena(1024*1024*1024, 16);
	surface Surface = {};
	Surface.Width = 2;
	Surface.Height = 2;
	Surface.Pixels = (color*)CheckerBoardData;
	
	Success = WriteTGA(&Surface, "data/checkerboard.tga", &Arena);
	Surface.Width = 16;
	Surface.Height = 16;
	Surface.Pixels = (color*)BrickData;
	Success = WriteTGA(&Surface, "data/bricks.tga", &Arena);
	
	return !Success;
}
