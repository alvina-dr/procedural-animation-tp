#version 410 core


uniform vec3 LightDir;
uniform float LightStrength;
uniform float Ambient;
uniform float Specular;
uniform float SpecularPow;

uniform bool LightingEnabled;

layout(location = 0, index = 0) out vec4 FragColor;

in block
{
	vec4 Color;
	vec3 CameraSpacePosition;
	vec3 CameraSpaceNormal;
} In;

void main()
{
	if(LightingEnabled) {
		vec3 n = normalize(In.CameraSpaceNormal);
		vec3 l = normalize(LightDir);
		float ndotl =  max(dot(n, l), 0.0);
		float lightContrib = ndotl * LightStrength;
		vec3 diffuse = In.Color.xyz;

		vec3 bisect = normalize(normalize(-In.CameraSpacePosition) + l);
		float ndotb = clamp(dot(n, bisect), 0.0, 1.0);
		float spec = pow(ndotb,SpecularPow) * Specular;
		vec3 color = diffuse * (lightContrib + spec) + diffuse * Ambient ;
		FragColor = vec4(color, In.Color.a);
	} else {
		FragColor = In.Color;
	}
}