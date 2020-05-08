#version 430 core

in vec2 glPos;
out vec4 FragColor;

uniform sampler2D tex;

void main()
{
	vec2 frag_pos = (glPos + 1.0f)/2.0f;
	//FragColor = vec4(frag_pos,0.0f,1.0f);
	FragColor = texture(tex, frag_pos);
}
