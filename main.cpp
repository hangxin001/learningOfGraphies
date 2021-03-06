#include<GL/glew.h>
#include<GLFW/glfw3.h>
#include<iostream>
#include<string>
#include"Utils.h"
#include"Sphere.h"
#include"Torus.h"
#include<glm/gtx/string_cast.hpp>
#include<vector>
#include<glm/ext.hpp>
#include"MaterialData.h"
#include"ModelImporter.h"
#include"ViewPort.h"
using namespace std;
using namespace glm;

auto& g_ViewPort = Singleton<ViewPort>::GetInstanceRef();
auto& g_InputMng = Singleton<InputMng>::GetInstanceRef();
static MaterialDataManager g_materialDataMng;

//以后要全部优化的变量，逐步封装为各种管理类
//待优化 开始
constexpr int numVAOs = 1;
constexpr int numVBOs = 3;
GLuint renderingProgram;
GLuint vao[numVAOs];
GLuint vbo[numVBOs];
float cubeLocX,cubeLocY, cubeLocZ;

//display()中使用
GLuint mvLoc, projLoc, vLoc, mLoc, tfLoc, nLoc;
GLuint brickTexture;
GLuint globalAmbLoc, ambLoc, diffLoc, specLoc, posLoc, mAmbLoc, mDiffLoc, mSpecLoc, mShiLoc;

int width, height;
float aspect;
glm::mat4 pMat, vMat, mMat, mvMat, invTrMat;
glm::mat4 tMat, rMat;
glm::vec3 currentLightPos, lightPosV;
glm::vec3 initialLightLoc = glm::vec3(5.0f, 2.0f, 2.0f);
float lightPos[3];
float globalAmbient[4] = {0.7f,0.7f,0.7f,1.0f};
//待优化 结束
ModelImporter g_torus{};

GLuint createShaderProgram() {
	GLuint vShader = LoadShaderSource(GL_VERTEX_SHADER, "./shader/vshader.glsl", 1);
	GLuint fShader = LoadShaderSource(GL_FRAGMENT_SHADER, "./shader/fshader.glsl", 1);
	GLuint vfProgram = glCreateProgram();
	glAttachShader(vfProgram, vShader);
	glAttachShader(vfProgram, fShader);
	glLinkProgram(vfProgram);
	ProgramErrorCheck(vfProgram);
	return vfProgram;
}
void InitModelData() {
	g_torus.parseOBJ("./model/Squirtle.obj");
	auto vertex = g_torus.getVertices();
	auto texCoord = g_torus.getTextureCoordinates();
	auto normals = g_torus.getNormals();

	glGenVertexArrays(numVAOs, vao);
	glBindVertexArray(vao[0]);
	glGenBuffers(numVBOs, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(decltype(vertex)::value_type) * vertex.size(), vertex.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(decltype(texCoord)::value_type) * texCoord.size(), texCoord.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(decltype(normals)::value_type) * normals.size(), normals.data(), GL_STATIC_DRAW);
	brickTexture = loadTexture("./model/tex/zenigame_0_0.tga");
}
void init(GLFWwindow* window) {
	renderingProgram = createShaderProgram();
	//cameraX = 0.0f;	cameraY = 0.0f; cameraZ = 10;
	cubeLocX = 0.0f; cubeLocY = 0.0f;	cubeLocZ = 0.0f;
	g_ViewPort.SetCameraPos(0.0f, 0.0f, 10);
	glfwGetFramebufferSize(window, &width, &height);
	g_InputMng.RegistInputDevicesCallBakeFun(window);
	InitModelData();
}
void installLights(glm::mat4 vMatrix) {
	auto GetWhiteLight = []()->Material* {
		return g_materialDataMng.GetMaterial("whiteLight");
	};
	auto GetGold = []()->Material* {
		return g_materialDataMng.GetMaterial("gold");
	};

	lightPosV = glm::vec3(vMatrix * glm::vec4(currentLightPos, 1.0));
	lightPos[0] = lightPosV.x;
	lightPos[1] = lightPosV.y;
	lightPos[2] = lightPosV.z;

	// 在着色器中获取光源位置和材质属性
	globalAmbLoc = glGetUniformLocation(renderingProgram, "globalAmbient");
	ambLoc = glGetUniformLocation(renderingProgram, "light.ambient");
	diffLoc = glGetUniformLocation(renderingProgram, "light.diffuse");
	specLoc = glGetUniformLocation(renderingProgram, "light.specular");
	posLoc = glGetUniformLocation(renderingProgram, "light.position");
	mAmbLoc = glGetUniformLocation(renderingProgram, "material.ambient");
	mDiffLoc = glGetUniformLocation(renderingProgram, "material.diffuse");
	mSpecLoc = glGetUniformLocation(renderingProgram, "material.specular");
	mShiLoc = glGetUniformLocation(renderingProgram, "material.shininess");
	
	// 在着色器中为光源与材质统一变量赋值
	glProgramUniform4fv(renderingProgram, globalAmbLoc, 1, globalAmbient);
	glProgramUniform4fv(renderingProgram, ambLoc, 1, GetWhiteLight()->GetMaterialAmbient().data());
	glProgramUniform4fv(renderingProgram, diffLoc, 1, GetWhiteLight()->GetMaterialDiffuse().data());
	glProgramUniform4fv(renderingProgram, specLoc, 1, GetWhiteLight()->GetMaterialSpecular().data());
	glProgramUniform3fv(renderingProgram, posLoc, 1, lightPos);
	glProgramUniform4fv(renderingProgram, mAmbLoc, 1, GetGold()->GetMaterialAmbient().data());
	glProgramUniform4fv(renderingProgram, mDiffLoc, 1, GetGold()->GetMaterialDiffuse().data());
	glProgramUniform4fv(renderingProgram, mSpecLoc, 1, GetGold()->GetMaterialSpecular().data());
	glProgramUniform1f(renderingProgram, mShiLoc, GetGold()->GetMaterialShiniess());
}

void display(GLFWwindow* window, double currentTime) {
	using namespace glm;
	auto rotate = [](vector<glm::vec4>& vecTrianglesVertex, float rad) {
		for (auto v = vecTrianglesVertex.begin(); v != vecTrianglesVertex.end(); ++v) {
			*v = buildRotateMat(rad, RotateAxis::Z) * (*v);
		}
	};
	auto scale = [](vector<glm::vec4>& vecTrianglesVertex, float scaleX, float scaleY, float scaleZ) {
		for (auto v = vecTrianglesVertex.begin(); v != vecTrianglesVertex.end(); ++v) {
			*v = buildScaleMat(scaleX, scaleY, scaleZ) * (*v);
		}
	};
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(renderingProgram);
	
	mvLoc = glGetUniformLocation(renderingProgram, "mv_matrix");
	projLoc = glGetUniformLocation(renderingProgram, "proj_matrix");
	vLoc = glGetUniformLocation(renderingProgram, "v_matrix");
	mLoc = glGetUniformLocation(renderingProgram, "m_matrix");
	nLoc = glGetUniformLocation(renderingProgram, "norm_matrix");

	auto angle = g_ViewPort.GetCameraAngle();
	auto angleX = std::get<0>(angle);
	auto angleY = std::get<1>(angle);
	/*	DEBUG_ANGLE
	auto debugAngle = g_ViewPort.GetCameraAngle();
	auto debugAngleX = std::get<0>(debugAngle);
	auto denigAngleY = std::get<1>(debugAngle);
	g_ViewPort.SetCameraRotate(ViewPort::CameraRotateAxis::X, 1);
	g_ViewPort.SetCameraRotate(ViewPort::CameraRotateAxis::Y, 1);
	*/
	pMat = g_ViewPort.GetPMat();
	
	vMat = g_ViewPort.GetVMat();
	mMat = glm::translate(glm::mat4(f1), glm::vec3(cubeLocX, cubeLocY, cubeLocZ));
	

	currentLightPos = glm::vec3(initialLightLoc.x, initialLightLoc.y, initialLightLoc.z);
	installLights(vMat);
	mvMat = vMat * mMat;

	invTrMat = glm::transpose(glm::inverse(mvMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));


	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, brickTexture);


	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDrawArraysInstanced(GL_TRIANGLES, 0, g_torus.getNumVertices(), 1);

}
int main(int argc, char** argv) {
	g_ViewPort.Init(400, 400, 1.0472f, 0.1f, 100.0f);
	if (!glfwInit())	exit(EXIT_FAILURE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	GLFWwindow* windows = glfwCreateWindow(g_ViewPort.GetScreenWidth(), g_ViewPort.GetScreenHeight(), "OPENGL", NULL, NULL);
	glfwMakeContextCurrent(windows);
	if (glewInit() != GLEW_OK)
		exit(EXIT_FAILURE);
	glfwSwapInterval(1);
	init(windows);
	
	while (!glfwWindowShouldClose(windows)) {
		display(windows, glfwGetTime());
		glfwSwapBuffers(windows);
		glfwWaitEvents();
	}
	glfwDestroyWindow(windows);
	glfwTerminate();
}