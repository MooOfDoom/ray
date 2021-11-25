/*
 * imagadata.h
 *
 * Image data (note: upside-down)
 */

#define B 0,0,0,
#define W 1,1,1,

color CheckerBoardData[] =
{
	B W
	W B
};

#undef B
#undef W

#define I .8,.8,.8,
#define J .7,.7,.7,
#define D .5,.4,.2,
#define L .7,.5,.2,

color BrickData[] =
{
	I J I J I J I J I J I J I J I J
	J I J J J J J J J J J J J J J J
	D D I J D D D D D D D D D D D D
	D L J J D L D L D L D L D L D L
	L L I J D D L D L D L D L D L D
	D L J J D L D L D L D L D L D L
	L L I J D D L D L D L D L D L D
	L L J I D L L L L L L L L L L L
	I J I J I J I J I J I J I J I J
	J J J J J J J J J I J J J J J J
	D D D D D D D D D D I J D D D D
	D L D L D L D L D L J J D L D L
	L D L D L D L D L L I J D D L D
	D L D L D L D L D L J J D L D L
	L D L D L D L D L L I J D D L D
	L L L L L L L L L L J I D L L L
};

#undef I
#undef J
#undef D
#undef L
