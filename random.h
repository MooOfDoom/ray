/*
 * random.h
 *
 * For random number generation
 */

typedef struct random_sequence
{
	u64 State;
} random_sequence;

function random_sequence
SeedRandom(u64 Seed)
{
	random_sequence Result = {Seed};
	return Result;
}

// From Marsaglia (https://en.wikipedia.org/wiki/Xorshift)
function u64
XORShift64s(random_sequence* RNG)
{
	u64 NextState = RNG->State;
	NextState ^= NextState >> 12;
	NextState ^= NextState << 25;
	NextState ^= NextState >> 27;
	RNG->State = NextState;
	u64 Result = NextState * 0x2545F4914F6CDD1Dull;
	return Result;
}

function u32
NextRandom(random_sequence* RNG)
{
	u64 XORShiftResult = XORShift64s(RNG);
	u32 Result = (u32)(XORShiftResult >> 32);
	return Result;
}

function f32
RandomUnilateral(random_sequence* RNG)
{
	u32 RandResult = NextRandom(RNG);
	f32 Result = (f32)RandResult / 4294967295.0f;
	return Result;
}

function f32
RandomBilateral(random_sequence* RNG)
{
	f32 Result = 2.0f*RandomUnilateral(RNG) - 1.0f;
	return Result;
}

function v3
RandomUnitBallV3(random_sequence* RNG)
{
	v3 Result = {RandomBilateral(RNG), RandomBilateral(RNG), RandomBilateral(RNG)};
	while (LengthSq(Result) > 1.0f)
	{
		Result = (v3){RandomBilateral(RNG), RandomBilateral(RNG), RandomBilateral(RNG)};
	}
	return Result;
}

function v3
RandomUnitV3(random_sequence* RNG)
{
	v3 Result = NormOrDefault(RandomUnitBallV3(RNG), (v3){0, 0, 1});
	return Result;
}