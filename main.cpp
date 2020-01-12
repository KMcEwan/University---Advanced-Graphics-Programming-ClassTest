#pragma warning(disable: 4996)
#if _DEBUG
#pragma comment(linker, "/subsystem:\"console\" /entry:\"WinMainCRTStartup\"")
#endif

#include "rt3d.h"
#include "rt3dObjLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stack>
#define DEG_TO_RADIAN 0.017453293

GLuint loadBitmap(char *fname);
GLuint loadCubeMap(const char *fname[6], GLuint *texID);
glm::vec3 moveForward(glm::vec3 pos, GLfloat angle, GLfloat d);

using namespace std;
stack<glm::mat4> mvStack;
bool shaderLoaded = false;
bool cubeLight = false;
bool moveLight = false;
bool skyboxLoaded = false;
bool secondBunny = false;
bool shiftActive = false;
bool shaderPreviousLoaded = false;				//Needed for changing second bunnys shader, if other shaders have not been on first bunny
//------------------------------------------Shaders
GLuint currentShader;
GLuint gouraudShader;
GLuint phongShader;
GLuint toonShader;
GLuint skyboxShader;
GLuint refractionShader;
GLuint reflectionShader;
GLuint bunnyShader;
GLuint mirrorShader;
//------------------------------------------Objects
GLuint meshIndexCount = 0;
GLuint meshObjects[2];
vector<GLfloat> verts, norms, tex_coords;
vector<GLuint> indices, size;
//------------------------------------------Lights & materials
rt3d::lightStruct light =
{
	{0.5, 0.5, 0.5, 1.0},
	{1.0, 1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
	{0.0, 0.0, 0.0, 1.0}
};

glm::vec4 lightPos(0.0f, 2.0f, 20.0f, 1.0f); 

rt3d::materialStruct currentMaterial;
rt3d::materialStruct bunnyMaterial;

rt3d::materialStruct material0 = {
	{0.2f, 0.5f, 0.2f, 1.0f}, // ambient
	{1.0f, 1.0f, 1.0f, 1.0f}, // diffuse
	{0.3f, 0.3f, 0.3f, 1.0f}, // specular
	0.2f					 // shininess
};

rt3d::materialStruct glass = {
	{0.2f, 0.4f, 0.2f, 0.2f}, // ambient
	{0.5f, 0.5f, 0.5f, 0.2f}, // diffuse
	{0.3f, 0.3f, 0.3f, 0.2f}, // specular
	0.2f					 // shininess
};

rt3d::materialStruct reflectionMat = {
	{0.2f, 0.4f, 0.2f, 1.0f}, // ambient
	{1.0f, 1.0f, 1.0f, 1.0f}, // diffuse
	{0.3f, 0.3f, 0.3f, 1.0f}, // specular
	0.2f					 // shininess
};

float attConstant = 1.0f;
float attLinear = 0.02f;
float attQuadratic = 0.01f;

//-----------------------------------------------------------------
glm::vec3 eye(0.0f, 3.0f, 0.0f);
glm::vec3 at(0.0f, 1.0f, -1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
GLfloat rotation = 0.0f;
//-----------------------------------------------------------------Textures
GLuint textures[2];
GLuint skybox[5];

//-----------------------------------------------------------------FBO, RBO for mirror
GLuint fboID;
GLuint depthBufID;
GLuint reflectionTex;
GLuint screenWidth = 800;
GLuint screenHeight = 600;
GLuint reflectionWidth = 1280;
GLuint reflectionHeight = 720;
static const GLenum fboAttachments[] = { GL_COLOR_ATTACHMENT0 };
static const GLenum frameBuff[] = { GL_BACK_LEFT };
glm::vec3 mirror_pos(0.0, 4.0, -17.0);
GLfloat mirrorAngle = 0.0f;

void init(void)
{
	GLuint currentShader = gouraudShader;
	//--------------------------------------------------------------------------------------------Shader init
	gouraudShader = rt3d::initShaders("../gouraud.vert", "../gouraud.frag");
	rt3d::setLight(gouraudShader, light);
	rt3d::setMaterial(gouraudShader, material0);
	GLuint uniformIndex = glGetUniformLocation(gouraudShader, "attConst");
	glUniform1f(uniformIndex, attConstant);
	uniformIndex = glGetUniformLocation(gouraudShader, "attLinear");
	glUniform1f(uniformIndex, attLinear);
	uniformIndex = glGetUniformLocation(gouraudShader, "attQuadratic");
	glUniform1f(uniformIndex, attQuadratic);

	phongShader = rt3d::initShaders("../phong.vert", "../phong.frag");
	rt3d::setLight(phongShader, light);
	rt3d::setMaterial(phongShader, material0);
	uniformIndex = glGetUniformLocation(phongShader, "attConst");
	glUniform1f(uniformIndex, attConstant);
	uniformIndex = glGetUniformLocation(phongShader, "attLinear");

	glUniform1f(uniformIndex, attLinear);
	uniformIndex = glGetUniformLocation(phongShader, "attQuadratic");
	glUniform1f(uniformIndex, attQuadratic);

	toonShader = rt3d::initShaders("../toon.vert", "../toon.frag");
	rt3d::setLight(toonShader, light);
	uniformIndex = glGetUniformLocation(toonShader, "attConst");
	glUniform1f(uniformIndex, attConstant);
	uniformIndex = glGetUniformLocation(toonShader, "attLinear");
	glUniform1f(uniformIndex, attLinear);
	uniformIndex = glGetUniformLocation(toonShader, "attQuadratic");
	glUniform1f(uniformIndex, attQuadratic);

	refractionShader = rt3d::initShaders("../refract.vert", "../refract.frag");
	rt3d::setLight(phongShader, light);
	rt3d::setMaterial(phongShader, material0);

	skyboxShader = rt3d::initShaders("../cubeMap.vert", "../cubeMap.frag");


	reflectionShader = rt3d::initShaders("../reflect.vert", "../reflect.frag");
	rt3d::setLight(phongShader, light);
	rt3d::setMaterial(phongShader, material0);

	mirrorShader = rt3d::initShaders("../textured.vert", "../textured.frag");


	//--------------------------------------------------------------------------------------------Object and bitmap load
	vector<GLfloat> verts, norms, tex_coords;
	vector<GLuint> indices;
	GLuint size;
	
	rt3d::loadObj("../resources/cube.obj", verts, norms, tex_coords, indices);
	size = indices.size();
	meshIndexCount = size;
	textures[0] = loadBitmap("../resources/fabric.bmp");
	meshObjects[0] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), tex_coords.data(), size, indices.data());

	verts.clear();
	norms.clear();
	tex_coords.clear();
	indices.clear();
	

	rt3d::loadObj("../resources/bunny-5000.obj", verts, norms, tex_coords, indices);
	size = indices.size();
	meshIndexCount = size;
	meshObjects[1] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), nullptr, size, indices.data());

	const char *cubeTexFiles[6] = {
		"../resources/skybox/Town_bk.bmp",
		"../resources/skybox/Town_ft.bmp",
		"../resources/skybox/Town_rt.bmp",
		"../resources/skybox/Town_lf.bmp",
		"../resources/skybox/Town_up.bmp",
		"../resources/skybox/Town_dn.bmp"
	};
	loadCubeMap(cubeTexFiles, &skybox[0]);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	uniformIndex = glGetUniformLocation(reflectionShader, "cubeMap");
	glUniform1i(uniformIndex, 1);
	
	uniformIndex = glGetUniformLocation(refractionShader, "texMap");
	glUniform1i(uniformIndex, 0);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox[0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);

	uniformIndex = glGetUniformLocation(reflectionShader, "cubeMap");
	glUniform1i(uniformIndex, 1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox[0]);

	// ---------------------------------------------------------------------------------FBO, RBO for mirror
	glGenFramebuffers(1, &fboID);
	glGenRenderbuffers(1, &depthBufID);
	glGenTextures(1, &reflectionTex);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboID);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBufID);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, reflectionWidth, reflectionHeight);
	glBindTexture(GL_TEXTURE_2D, reflectionTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, reflectionWidth, reflectionHeight, 0, GL_RGBA, GL_FLOAT, NULL);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionTex, 0);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufID);

	GLenum valid = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	if (valid != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer Object not complete" << std::endl;
	if (valid == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
		std::cout << "Framebuffer incomplete attachment" << std::endl;
	if (valid == GL_FRAMEBUFFER_UNSUPPORTED)
		std::cout << "FBO attachments unsupported" << std::endl;
	   
}

void draw(SDL_Window * window)
{
	if (shaderLoaded == false)
	{		
		shaderLoaded = true;
		currentShader = gouraudShader;
		currentMaterial = material0;
		bunnyShader = gouraudShader;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(1.0f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection(1.0);
	projection = glm::perspective(float(60.0f*DEG_TO_RADIAN), 800.0f / 600.0f, 1.0f, 150.0f);
	

	GLfloat scale(1.0f); 
	glm::mat4 modelview(1.0); 
	mvStack.push(modelview);	

	for (int pass = 0; pass < 2; pass++)				//First pass created reflected image, 2nd renders whole scene
	{		
		at = moveForward(eye, rotation, 1.0f);
		mvStack.top() = glm::lookAt(eye, at, up);


		if (pass == 0)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboID);
			glDrawBuffers(1, fboAttachments);
			glViewport(0, 0, reflectionWidth, reflectionHeight);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);						//Clearing FBO
			mvStack.top() = glm::translate(mvStack.top(), mirror_pos);				//mirror_pos(-5.0f, 1.5f, -5.0f);
			mvStack.top() = glm::rotate(mvStack.top(), mirrorAngle, glm::vec3(0.0f, 1.0f, 0.0f));
			mvStack.top() = glm::scale(mvStack.top(), glm::vec3(1.0f, 1.0f, -1.0f));
			mvStack.top() = glm::rotate(mvStack.top(), -mirrorAngle, glm::vec3(0.0f, 1.0f, 0.0f));
			mvStack.top() = glm::translate(mvStack.top(), -mirror_pos);
		}
		else
		{			
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glDrawBuffers(1, frameBuff);
			glViewport(0, 0, screenWidth, screenHeight);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glEnable(GL_CULL_FACE);
			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		}


		//at = moveForward(eye, rotation, 1.0f);
		//mvStack.top() = glm::lookAt(eye, at, up);
		//---------------------------------------------------------------------------------Skybox
		glUseProgram(skyboxShader);
		rt3d::setUniformMatrix4fv(skyboxShader, "projection", glm::value_ptr(projection));
		glDepthMask(GL_FALSE); 
		glm::mat3 mvRotOnlyMat3 = glm::mat3(mvStack.top());
		mvStack.push(glm::mat4(mvRotOnlyMat3));
		glCullFace(GL_FRONT); 
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox[0]);
		mvStack.top() = glm::scale(mvStack.top(), glm::vec3(1.5f, 1.5f, 1.5f));
		rt3d::setUniformMatrix4fv(skyboxShader, "modelview", glm::value_ptr(mvStack.top()));
		rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
		glCullFace(GL_BACK); 
		mvStack.pop();
		glDepthMask(GL_TRUE); 

		glUseProgram(currentShader);
		rt3d::setUniformMatrix4fv(currentShader, "projection", glm::value_ptr(projection));
	
		glm::vec4 tmp = mvStack.top()*lightPos;
		light.position[0] = tmp.x;
		light.position[1] = tmp.y;
		light.position[2] = tmp.z;
		rt3d::setLightPos(currentShader, glm::value_ptr(tmp));

		if (pass == 0)
		{
			glCullFace(GL_FRONT);
		}

		if (shaderPreviousLoaded == false)
		{
			glUseProgram(bunnyShader);
			rt3d::setUniformMatrix4fv(bunnyShader, "projection", glm::value_ptr(projection));
			rt3d::setLightPos(bunnyShader, glm::value_ptr(tmp));
		}
		
	   	   
		if (cubeLight)
		{
			//Cube for light
			glBindTexture(GL_TEXTURE_2D, textures[0]);
			mvStack.push(mvStack.top());
			mvStack.top() = glm::translate(mvStack.top(), glm::vec3(lightPos.x, lightPos.y, lightPos.z));
			mvStack.top() = glm::scale(mvStack.top(), glm::vec3(3.0, 3.0, 3.0));
			rt3d::setUniformMatrix4fv(currentShader, "modelview", glm::value_ptr(mvStack.top()));
			rt3d::setMaterial(currentShader, material0);
			rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
			mvStack.pop();
		}

		if (skyboxLoaded)
		{
			glUseProgram(currentShader);
			glm::mat4 modelMatrix(1.0);
			mvStack.push(mvStack.top());
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.0f, 1.0f, -3.0f));
			modelMatrix = glm::rotate(modelMatrix, float(0.0*DEG_TO_RADIAN), glm::vec3(1.0f, 1.0f, 1.0f));
			mvStack.top() = mvStack.top() * modelMatrix;

			int uniformIndex = glGetUniformLocation(currentShader, "modelMatrix");
			glUniformMatrix4fv(uniformIndex, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
			rt3d::setUniformMatrix4fv(currentShader, "projection", glm::value_ptr(projection));

			uniformIndex = glGetUniformLocation(currentShader, "cameraPos");
			glUniform3fv(uniformIndex, 1, glm::value_ptr(eye));
			
			if (shaderPreviousLoaded == false)
			{
				glUseProgram(bunnyShader);
				glm::mat4 modelMatrix(1.0);
				mvStack.push(mvStack.top());
				modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.0f, 1.0f, -3.0f));
				modelMatrix = glm::rotate(modelMatrix, float(0.0*DEG_TO_RADIAN), glm::vec3(1.0f, 1.0f, 1.0f));
				mvStack.top() = mvStack.top() * modelMatrix;
				uniformIndex = glGetUniformLocation(bunnyShader, "modelMatrix");
				glUniformMatrix4fv(uniformIndex, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
				rt3d::setUniformMatrix4fv(bunnyShader, "projection", glm::value_ptr(projection));

				uniformIndex = glGetUniformLocation(bunnyShader, "cameraPos");
				glUniform3fv(uniformIndex, 1, glm::value_ptr(eye));
				mvStack.pop();
			}
			mvStack.pop();
		}

		//Cube for ground plane
		glUseProgram(currentShader);
		glBindTexture(GL_TEXTURE_2D, textures[0]);
		mvStack.push(mvStack.top());
		mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-10.0f, -0.1f, -10.0f));
		mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 0.1f, 20.0f));
		rt3d::setUniformMatrix4fv(currentShader, "modelview", glm::value_ptr(mvStack.top()));
		rt3d::setMaterial(currentShader, material0);
		rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
		mvStack.pop();


		//Bunny
		glBindTexture(GL_TEXTURE_2D, textures[0]);
		mvStack.push(mvStack.top());
		mvStack.top() = glm::translate(mvStack.top(), glm::vec3(0.0, -1.0, -7.0));
		mvStack.top() = glm::scale(mvStack.top(), glm::vec3(30.0, 30.0, 30.0));
		rt3d::setUniformMatrix4fv(currentShader, "modelview", glm::value_ptr(mvStack.top()));
		rt3d::setMaterial(currentShader, currentMaterial);
		rt3d::drawIndexedMesh(meshObjects[1], meshIndexCount, GL_TRIANGLES);
		mvStack.pop();

		if (secondBunny)
		{
			glUseProgram(bunnyShader);
			glBindTexture(GL_TEXTURE_2D, textures[0]);
			mvStack.push(mvStack.top());
			mvStack.top() = glm::translate(mvStack.top(), glm::vec3(5.0, -1.0, -7.0));
			mvStack.top() = glm::scale(mvStack.top(), glm::vec3(30.0, 30.0, 30.0));
			rt3d::setUniformMatrix4fv(bunnyShader, "modelview", glm::value_ptr(mvStack.top()));
			rt3d::setMaterial(bunnyShader, bunnyMaterial);
			rt3d::drawIndexedMesh(meshObjects[1], meshIndexCount, GL_TRIANGLES);
			mvStack.pop();
		}


		glCullFace(GL_BACK);

		if (pass == 1) {
			glUseProgram(mirrorShader);
			rt3d::setUniformMatrix4fv(mirrorShader, "projection", glm::value_ptr(projection));
			mvStack.push(mvStack.top());
			mvStack.top() = glm::translate(mvStack.top(), mirror_pos);
			mvStack.top() = glm::rotate(mvStack.top(), mirrorAngle, glm::vec3(0.0f, 1.0f, 0.0f));
			mvStack.top() = glm::scale(mvStack.top(), glm::vec3(5.0f, -3.5f, 0.1f));
			rt3d::setUniformMatrix4fv(mirrorShader, "modelview", glm::value_ptr(mvStack.top()));
			
			GLuint uniformIndex = glGetUniformLocation(mirrorShader, "textureUnit0");
			glUniform1i(uniformIndex, 0);
			
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, reflectionTex);
			glCullFace(GL_BACK);
			rt3d::drawIndexedMesh(meshObjects[0], 6, GL_TRIANGLES); 
			glBindTexture(GL_TEXTURE_2D, 0); 
			mvStack.pop();
		}
	}


	mvStack.pop(); 
	glDepthMask(GL_TRUE);

	SDL_GL_SwapWindow(window);
}

glm::vec3 moveForward(glm::vec3 pos, GLfloat angle, GLfloat d)
{
	return glm::vec3(pos.x + d * std::sin(rotation*DEG_TO_RADIAN), pos.y, pos.z - d * std::cos(rotation*DEG_TO_RADIAN));
}

glm::vec3 moveRight(glm::vec3 pos, GLfloat angle, GLfloat d) 
{
	return glm::vec3(pos.x + d * std::cos(rotation*DEG_TO_RADIAN), pos.y, pos.z + d * std::sin(rotation*DEG_TO_RADIAN));
}

void update()
{
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_LSHIFT])
	{
		shiftActive = true;		
	}
	if (!keys[SDL_SCANCODE_LSHIFT])
	{
		shiftActive = false;
	}


	if (moveLight)
	{
		if (keys[SDL_SCANCODE_W])
		{
			eye = moveForward(eye, rotation, 0.1f);
			lightPos[2] -= 0.1;
		}
		if (keys[SDL_SCANCODE_S])
		{
			eye = moveForward(eye, rotation, -0.1f);
			lightPos[2] += 0.1;
		}
		if (keys[SDL_SCANCODE_A])
		{
			eye = moveRight(eye, rotation, -0.1f);
			lightPos[0] -= 0.1;
		}
		if (keys[SDL_SCANCODE_D])
		{
			eye = moveRight(eye, rotation, 0.1f);
			lightPos[0] += 0.1;
		}

		//Testing for light
		if (keys[SDL_SCANCODE_I])
		{
			lightPos[2] -= 0.1;
		}
		if (keys[SDL_SCANCODE_K])
		{
			lightPos[2] += 0.1;
		}
		if (keys[SDL_SCANCODE_J])
		{
			lightPos[0] -= 0.1;
		}
		if (keys[SDL_SCANCODE_L])
		{
			lightPos[0] += 0.1;
		}
		if (keys[SDL_SCANCODE_U])
		{
			lightPos[1] += 0.1;
		}
		if (keys[SDL_SCANCODE_H])
		{
			lightPos[1] -= 0.1;
		}
	}
	else
	{
		if (keys[SDL_SCANCODE_W])
		{
			eye = moveForward(eye, rotation, 0.1f);
		}
		if (keys[SDL_SCANCODE_S])
		{
			eye = moveForward(eye, rotation, -0.1f);
		}
		if (keys[SDL_SCANCODE_A])
		{
			eye = moveRight(eye, rotation, -0.1f);
		}
		if (keys[SDL_SCANCODE_D])
		{
			eye = moveRight(eye, rotation, 0.1f);
		}
	}
	
	if (shiftActive == false)
	{
		if (keys[SDL_SCANCODE_1])
		{
			currentMaterial = material0;
			material0.specular[0] = 0.3;
			material0.specular[1] = 0.3;
			material0.specular[2] = 0.3;
			material0.diffuse[0] = 1.0;
			material0.diffuse[1] = 1.0;
			material0.diffuse[2] = 1.0;
			material0.shininess = 0.2;
			currentShader = gouraudShader;
			skyboxLoaded = false;
			shaderPreviousLoaded = true;
		}
		
		if (keys[SDL_SCANCODE_2])
		{
			currentMaterial = material0;
			currentShader = phongShader;
			material0.specular[0] = 0.3;
			material0.specular[1] = 0.3;
			material0.specular[2] = 0.3;
			material0.diffuse[0] = 2.0;
			material0.diffuse[1] = 2.0;
			material0.diffuse[2] = 2.0;
			material0.shininess = 0.2;
			skyboxLoaded = false;
			shaderPreviousLoaded = true;
		}

		if (keys[SDL_SCANCODE_3])
		{
			currentMaterial = material0;
			currentShader = phongShader;
			material0.specular[0] = 3.0;
			material0.specular[1] = 3.0;
			material0.specular[2] = 3.0;
			material0.diffuse[0] = 0.2;
			material0.diffuse[1] = 0.2;
			material0.diffuse[2] = 0.2;
			material0.shininess = 3.0;
			skyboxLoaded = false;
			shaderPreviousLoaded = true;
		}

		if (keys[SDL_SCANCODE_4])
		{
			cubeLight = true;
			moveLight = true;
		}

		if (keys[SDL_SCANCODE_6])
		{
			currentShader = toonShader;
			skyboxLoaded = false;
		}

		if (keys[SDL_SCANCODE_7])
		{
			currentShader = refractionShader;
			skyboxLoaded = true;
			currentMaterial = glass;
			shaderPreviousLoaded = true;
		}

		if (keys[SDL_SCANCODE_8])
		{
			currentShader = reflectionShader;
			skyboxLoaded = true;
			currentMaterial = reflectionMat;	
			shaderPreviousLoaded = true;
		}

		if (keys[SDL_SCANCODE_9])
		{
			secondBunny = true;
			bunnyShader = gouraudShader;
			bunnyMaterial = material0;
		}
	}
	

	if ((keys[SDL_SCANCODE_LSHIFT]) && (keys[SDL_SCANCODE_1]))
	{
		bunnyShader = gouraudShader;
		bunnyMaterial = material0;
		material0.specular[0] = 0.3;
		material0.specular[1] = 0.3;
		material0.specular[2] = 0.3;
		material0.diffuse[0] = 1.0;
		material0.diffuse[1] = 1.0;
		material0.diffuse[2] = 1.0;
		material0.shininess = 0.2;
	}

	if ((keys[SDL_SCANCODE_LSHIFT]) && (keys[SDL_SCANCODE_2]))
	{
		bunnyMaterial = material0;
		bunnyShader = phongShader;
		material0.specular[0] = 0.3;
		material0.specular[1] = 0.3;
		material0.specular[2] = 0.3;
		material0.diffuse[0] = 1.0;
		material0.diffuse[1] = 1.0;
		material0.diffuse[2] = 1.0;
		material0.shininess = 0.2;
		skyboxLoaded = false;
	}

	if ((keys[SDL_SCANCODE_LSHIFT]) && (keys[SDL_SCANCODE_3]))
	{
		bunnyMaterial = material0;
		bunnyShader = phongShader;
		material0.specular[0] = 1.0;
		material0.specular[1] = 1.0;
		material0.specular[2] = 1.0;
		material0.diffuse[0] = 0.0;
		material0.diffuse[1] = 0.0;
		material0.diffuse[2] = 0.0;
		material0.shininess = 2.0;
		skyboxLoaded = false;
	}
		

	if ((keys[SDL_SCANCODE_LSHIFT]) && (keys[SDL_SCANCODE_6]))
	{
		bunnyShader = toonShader;		
		bunnyMaterial = material0;
	}


	if ((keys[SDL_SCANCODE_LSHIFT]) && (keys[SDL_SCANCODE_7]))
	{
		bunnyShader = refractionShader;
		bunnyMaterial = glass;
		skyboxLoaded = true;
	}

	if ((keys[SDL_SCANCODE_LSHIFT]) && (keys[SDL_SCANCODE_8]))
	{
		bunnyShader = reflectionShader;
		bunnyMaterial = material0;
		material0.specular[0] = 1.0;
		material0.specular[1] = 1.0;
		material0.specular[2] = 1.0;
		material0.diffuse[0] = 0.0;
		material0.diffuse[1] = 0.0;
		material0.diffuse[2] = 0.0;
		material0.shininess = 2.0;
		skyboxLoaded = true;

	}

	if (keys[SDL_SCANCODE_R]) eye.y += 0.1;
	if (keys[SDL_SCANCODE_F]) eye.y -= 0.1;

	if (keys[SDL_SCANCODE_COMMA]) rotation -= 1.0f;
	if (keys[SDL_SCANCODE_PERIOD]) rotation += 1.0f;

}



SDL_Window* setupRC(SDL_GLContext &context)
{
	SDL_Window* window;
	SDL_Init(SDL_INIT_VIDEO);


	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); 
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	window = SDL_CreateWindow("Class Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,	800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!window)
		rt3d::exitFatalError("winodw not created");

	context = SDL_GL_CreateContext(window);																
	SDL_GL_SetSwapInterval(1);																			
	return window;
}


GLuint loadBitmap(char *fname) 
{
	GLuint texID;
	glGenTextures(1, &texID);																			
	SDL_Surface *tmpSurface;																			
	tmpSurface = SDL_LoadBMP(fname);
	if (!tmpSurface)
	{
		std::cout << "Error loading bimpap" << std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, texID);																
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);									 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);									
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);								
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	SDL_PixelFormat *format = tmpSurface->format;														

	GLuint externalFormat, internalFormat;																
	if (format->Amask)
	{																				
		internalFormat = GL_RGBA;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGBA : GL_BGRA;
	}
	else 
	{
		internalFormat = GL_RGB;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGB : GL_BGR;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tmpSurface->w, tmpSurface->h, 0,	externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);																	

	SDL_FreeSurface(tmpSurface);																		
	return texID;
}

GLuint loadCubeMap(const char *fname[6], GLuint *texID)
{
	glGenTextures(1, texID); 
	GLenum sides[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
						GL_TEXTURE_CUBE_MAP_POSITIVE_X,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
						GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_Y };
	SDL_Surface *tmpSurface;

	glBindTexture(GL_TEXTURE_CUBE_MAP, *texID); 
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	GLuint externalFormat;
	for (int i = 0; i < 6; i++)
	{		
		tmpSurface = SDL_LoadBMP(fname[i]);
		if (!tmpSurface)
		{
			std::cout << "Error loading bitmap" << std::endl;
			return *texID;
		}
				
		SDL_PixelFormat *format = tmpSurface->format;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGB : GL_BGR;

		glTexImage2D(sides[i], 0, GL_RGB, tmpSurface->w, tmpSurface->h, 0, externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
		
		SDL_FreeSurface(tmpSurface);
	}
	return *texID;	
}



int main(int argc, char *argv[])
{
	SDL_Window* hWindow; 
	SDL_GLContext glContext; 
	hWindow = setupRC(glContext); 

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{ 
		std::cout << "glew Init failed, aborting." << endl;
		exit(1);
	}
	cout << glGetString(GL_VERSION) << endl;

	init();


	bool running = true; 
	SDL_Event sdlEvent;  
	while (running) 
	{	
		while (SDL_PollEvent(&sdlEvent))
		{
			if (sdlEvent.type == SDL_QUIT)
				running = false;
		}
		update();
		draw(hWindow); 
	}

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(hWindow);
	SDL_Quit();
	return 0;
}

