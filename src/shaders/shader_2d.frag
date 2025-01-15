#version 410 core

layout(location = 0, index = 0) out vec4 FragColor;

in block
{
	vec4 Color;
} In; 


void main()
{
	FragColor = In.Color;
}