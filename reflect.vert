#version 330


uniform mat4 modelview;
uniform mat4 projection;
uniform vec4 lightPosition;

uniform mat4 modelMatrix;
uniform vec3 cameraPos;


in  vec3 in_Position;
in  vec3 in_Normal;
out vec3 screenNormal;
out vec3 viewVec;
out vec3 lightVec;

in vec2 in_TexCoord;
out vec2 ex_TexCoord;

out vec3 ex_WorldNorm;
out vec3 ex_WorldView;

void main(void) 
{	
	
	vec4 vertexPosition = modelview * vec4(in_Position,1.0);										
		
	viewVec = normalize(-vertexPosition).xyz;															
	
	mat3 normalmatrix = transpose(inverse(mat3(modelview)));										
	screenNormal = normalize(normalmatrix * in_Normal);
	
	lightVec = normalize(lightPosition.xyz - vertexPosition.xyz);										

	ex_TexCoord = in_TexCoord;

    gl_Position = projection * vertexPosition;

	vec3 worldPos = (modelMatrix * vec4(in_Position,1.0)).xyz;		
	
	mat3 normalworldmatrix = transpose(inverse(mat3(modelMatrix)));

	ex_WorldNorm = normalworldmatrix * in_Normal;

	ex_WorldView = cameraPos - worldPos;

}