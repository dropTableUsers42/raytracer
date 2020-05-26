#version 430 core

in vec2 glPos;
out vec4 FragColor;

uniform sampler2D tex;

uniform float exposure;

void main()
{
	const float gamma = 2.2;
	vec2 frag_pos = (glPos + 1.0f)/2.0f;
	vec3 hdrColor = texture(tex, frag_pos).rgb;

	//reinhard
	vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

	mapped = pow(mapped, vec3(1.0/gamma));

	FragColor = vec4(mapped, 1.0);
}
