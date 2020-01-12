#version 330

uniform mat4 modelview;
uniform mat4 projection;

in  vec3 in_Position;
in vec2 in_TexCoord;

out vec2 ex_TexCoord;
out vec4 proj_vertex;

void main(void) 
{
	vec4 vertexPosition = modelview * vec4(in_Position,1.0);

	proj_vertex = vertexPosition * projection;// vec4(in_Position,1.0) * modelview * projection;
    gl_Position = projection * vertexPosition;

	ex_TexCoord = in_TexCoord;
}