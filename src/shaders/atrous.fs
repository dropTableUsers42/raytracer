#version 430 core

in vec2 glPos;
out vec4 FragColor;

layout(location = 0) uniform sampler2D colMap;
layout(location = 1) uniform sampler2D normMap;
layout(location = 2) uniform sampler2D posMap;

uniform float c_phi;
uniform float n_phi;
uniform float p_phi;
uniform int stepwidth;

#define KERNEL_SIZE 9
uniform float kernel[KERNEL_SIZE];
uniform ivec2 offset[KERNEL_SIZE];

void main()
{
	vec2 frag_pos = (glPos + 1.0)/2.0;
	
    vec4 sum = vec4(0.0);
    ivec2 tx = ivec2(frag_pos * textureSize(colMap, 0));
    vec4 cval = texelFetch(colMap, tx, 0);
    vec4 nval = texelFetch(normMap, tx, 0);
    vec4 pval = texelFetch(posMap, tx, 0);

    float cum_w = 0.0;

    for(int i=0; i<KERNEL_SIZE;i++)
    {
        ivec2 uv = tx + offset[i] * stepwidth;
        vec4 ctmp = texelFetch(colMap, uv, 0);
        if(any(isnan(ctmp)))
        {
            continue;
        }
        vec4 t = cval - ctmp;
        float c_w = min(exp(-dot(t, t)/c_phi), 1.0);
        vec4 ntmp = texelFetch(normMap, uv, 0);
        t = nval - ntmp;
        float dist2 = max(dot(t, t)/(stepwidth * stepwidth), 0.0);
        float n_w = min(exp(-dist2/n_phi), 1.0);
        vec4 ptmp = texelFetch(posMap, uv, 0);
        t = pval - ptmp;
        float p_w = min(exp(-dot(t, t)/p_phi), 1.0);
        float weight = c_w * n_w * p_w;
        sum += ctmp * weight * kernel[i];
        cum_w += weight * kernel[i];
    }
    FragColor = sum/cum_w;
}
