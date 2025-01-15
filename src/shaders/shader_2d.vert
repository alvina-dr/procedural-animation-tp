#version 410 core

#define BufferAttribPosition	0
#define BufferAttribColor		1

uniform vec2 ViewportSize;

layout(location = BufferAttribPosition) in vec2 Position;
layout(location = BufferAttribColor) in vec4 Color;

out block
{
	vec4 Color;
} Out;

void main()
{
	vec2 ndcPos = (Position / ViewportSize) * 2.0 - 1.0;
	gl_Position = vec4(ndcPos, 0.0, 1.0);
	Out.Color = Color;
}