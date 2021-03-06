#version 330

struct lightStruct
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

struct materialStruct
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform lightStruct light;
uniform materialStruct material;
uniform sampler2D textureUnit0;

uniform samplerCube cubeMap;
uniform sampler2D texMap;

in vec3 screenNormal;
in vec3 viewVec;
in vec3 lightVec;
in vec2 ex_TexCoord;

in vec3 ex_WorldNorm;
in vec3 ex_WorldView;

layout(location = 0) out vec4 out_Color;
 
void main(void) 
{   	
	vec4 ambientI = light.ambient * material.ambient;	
	
	vec4 diffuseI = light.diffuse * material.diffuse;											
	
	diffuseI = diffuseI * max(dot(normalize(screenNormal),normalize(lightVec)),0);
	
	vec3 R = normalize(reflect(normalize(-lightVec),normalize(screenNormal)));					

	vec4 specularI = light.specular * material.specular;
	
	specularI = specularI * pow(max(dot(R,viewVec),0), material.shininess);
																								
	vec4 tmp_Color = (diffuseI + specularI); 
	
	vec4 litColour = ambientI + tmp_Color;

	vec3 reflectTexCoord = reflect(-ex_WorldView, normalize(ex_WorldNorm));

	out_Color = (texture(cubeMap, reflectTexCoord) * texture(texMap, ex_TexCoord)) * litColour;					
}