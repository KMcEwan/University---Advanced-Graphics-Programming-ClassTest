#version 330
precision highp float;

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

in float ex_attenuation;

in vec3 screenNormal;
in vec3 viewVec;
in vec3 lightVec;
layout(location = 0) out vec4 out_Color;
 
void main(void) {   
	
	vec4 ambientI = light.ambient * material.ambient;											

	
	vec4 diffuseI = light.diffuse * material.diffuse;											
	diffuseI = diffuseI * max(dot(normalize(screenNormal),normalize(lightVec)),0);
	
	vec3 R = normalize(reflect(normalize(-lightVec),normalize(screenNormal)));								

	vec4 specularI = light.specular * material.specular;
	specularI = specularI * pow(max(dot(R,viewVec),0), material.shininess);

	vec4 tmp_Color = (diffuseI + specularI);													 

	vec4 litColorAtt = ambientI + vec4(tmp_Color.rgb * ex_attenuation, 1.0);

	vec4 litColour = litColorAtt;
	vec4 shade1 = smoothstep(vec4(0.2),vec4(0.21),litColour);
	vec4 shade2 = smoothstep(vec4(0.2),vec4(0.41),litColour);
	vec4 shade3 = smoothstep(vec4(0.8),vec4(0.81),litColour);
	vec4 colour = max( max(0.3*shade1,0.5*shade2), shade3);
		
	if ( abs(dot(screenNormal,viewVec)) < 0.5)	
		colour = vec4(vec3(0.0),1.0);

		out_Color = colour;
}