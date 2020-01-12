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

uniform mat4 modelview;
uniform mat4 projection;
uniform vec4 lightPosition;

uniform lightStruct light;
uniform materialStruct material;

uniform sampler2D textureUnit0;

uniform float attConst;
uniform float attLinear;
uniform float attQuadratic;

in vec3 in_Normal;
in vec3 in_Position;
in vec2 in_TexCoord;

out vec4 out_Color;
out vec2 ex_TexCoord;


void main(void)
{
	vec4 vertexPosition = modelview * vec4(in_Position,1.0);	
    gl_Position = projection * vertexPosition;
	
	vec3 viewVec = normalize(-vertexPosition).xyz;
	vec3 lightVec = normalize(lightPosition.xyz - vertexPosition.xyz);
	float dist = distance(vertexPosition, lightPosition);

	mat3 normalmatrix = transpose(inverse(mat3(modelview)));
	vec3 screenNormal = normalize(normalmatrix * in_Normal);

	vec4 ambientI = light.ambient * material.ambient;											

	
	vec4 diffuseI = light.diffuse * material.diffuse;		
	
	diffuseI = diffuseI * max(dot(normalize(screenNormal),normalize(lightVec)),0);
	
	vec3 reflectedVec = normalize(reflect(normalize(-lightVec),normalize(screenNormal)));								

	vec4 specularI = light.specular * material.specular;

	specularI = specularI * pow(max(dot(reflectedVec,viewVec),0), material.shininess);


	float attenuation = 1.0f / (attConst + attLinear * dist + attQuadratic * dist * dist);

	vec4 tmp_color =  diffuseI + specularI;

	ex_TexCoord = in_TexCoord;
	
	vec4 litColour = ambientI + vec4(tmp_color.rgb * attenuation, 1.0);

	out_Color = litColour * texture(textureUnit0, ex_TexCoord);	
}