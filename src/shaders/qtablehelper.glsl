#version 430 core

#define MAX_POINTS 100

float radicalInverse(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint N = MAX_POINTS)
{
	return vec2(float(i)/float(N), radicalInverse(i));
}

uint nearest(vec2 uv, uint N = MAX_POINTS)
{
	float nearest = length(hammersley(0, N) - uv);
	uint nearestIdx = 0;
	for(uint i = 1; i < N; i++)
	{
		float len = length(hammersley(i, N) - uv);
		if(len < nearest)
		{
			nearestIdx = i;
			nearest = len;
		}
	}
	return nearestIdx;
}