#version 330

uniform float attConst;
uniform float attLinear;
uniform float attQuadratic;


uniform mat4 modelview;
uniform mat4 projection;
uniform vec4 lightPosition;
//uniform mat3 normalmatrix;

in  vec3 in_Position;
in  vec3 in_Normal;
out vec3 screenNormal;
out vec3 viewVec;
out vec3 lightVec;

out float ex_attenuation;
out float ex_dist;

																									
void main(void)
{	
	vec4 vertexPosition = modelview * vec4(in_Position,1.0);										

	viewVec = normalize(-vertexPosition).xyz;															

	mat3 normalmatrix = transpose(inverse(mat3(modelview)));																																			
																									
	screenNormal = normalize(normalmatrix * in_Normal);
	
	lightVec = normalize(lightPosition.xyz - vertexPosition.xyz);										

    gl_Position = projection * vertexPosition;

	float att_distance = distance(vertexPosition, lightPosition);

	ex_attenuation = (1.0 / (attConst + attLinear * att_distance + attQuadratic * att_distance * att_distance));			
}












