#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <cstdlib>

int RandomNumberGenerator::RollRandomIntLessThan(int maxNotInclusive)
{
    return rand() % maxNotInclusive;
}

int RandomNumberGenerator::RollRandomIntInRange(int minInclusive, int maxInclusive)
{
    return minInclusive + rand() % (maxInclusive - minInclusive + 1);
}

float RandomNumberGenerator::RollRandomFloatZeroToOne()
{
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

float RandomNumberGenerator::RollRandomFloatInRange(float minInclusive, float maxInclusive)
{
    return minInclusive + (maxInclusive - minInclusive) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
}

bool RandomNumberGenerator::RollRandomWithProbability(float probability)
{
    return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) <= probability;
}

float RandomNumberGenerator::RollTimeRelatedNoise(float timeSeconds)
{
    return SinRadians(2.f * 20.f * timeSeconds) + SinRadians( 3.1415926535897932384626433832795f * 20.f * timeSeconds);
}
