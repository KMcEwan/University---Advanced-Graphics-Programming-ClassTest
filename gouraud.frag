#version 330
precision highp float;

uniform sampler2D textureUnit0;

in vec2 ex_TexCoord;
in vec4 out_Color;

out vec4 gl_FragColor;
 
void main(void) 
{
	//gl_FragColor = texture(textureUnit0, ex_TexCoord) * out_Color;
	gl_FragColor = out_Color;
}