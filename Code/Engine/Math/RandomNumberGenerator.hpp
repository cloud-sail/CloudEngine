#pragma once
class RandomNumberGenerator {
public:
    int     RollRandomIntLessThan(int maxNotInclusive);
    int     RollRandomIntInRange(int minInclusive, int maxInclusive);
    float   RollRandomFloatZeroToOne();
    float   RollRandomFloatInRange(float minInclusive, float maxInclusive);
    bool    RollRandomWithProbability(float probability);
    float   RollTimeRelatedNoise(float timeSeconds);

private:
    //unsigned int    m_seed = 0;
    //int             m_position = 0;
};