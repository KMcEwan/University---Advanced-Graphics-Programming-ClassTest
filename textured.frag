#version 330

precision highp float;

uniform sampler2D textureUnit0;

in vec2 ex_TexCoord;
in vec4 proj_vertex;
layout(location = 0) out vec4 out_Color;
 
void main(void)
{
	vec4 reflectTexCoord = proj_vertex / proj_vertex.w;
	reflectTexCoord = (reflectTexCoord + 1.0) * (0.5);
	out_Color = texture(textureUnit0, reflectTexCoord.xy);

	//out_Color = texture(textureUnit0, ex_TexCoord);
}