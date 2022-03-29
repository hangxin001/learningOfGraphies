#pragma once
#ifndef __TESTOPENGL__UTILS__
#define __TESTOPENGL__UTILS__
#include<GL/glew.h>
#include<GLFW/glfw3.h>
#include<string>
#include<iostream>
#include<fstream>
#include<assert.h>
#include<type_traits>
#include<glm/glm.hpp>
#include<SOIL2/SOIL2.h>
#include<vector>
enum class RotateAxis {
	X = 0,
	Y = 1,
	Z = 2,
};
constexpr auto f0 = 0.0;
constexpr auto f1 = 1.0;


std::string ReadGLSL(std::string filePath);

void ProgramErrorCheck(GLint program);

GLuint LoadShaderSource(GLuint flag, std::string shaderSourcePath, GLsizei count);	//����shader�ļ�

glm::mat4 buildTranslateMat(float x, float y, float z);

glm::mat4 buildRotateMat(float rad, RotateAxis axis);

glm::mat4 buildScaleMat(float x, float y, float z);

GLuint loadTexture(const char* texImagePath);

GLuint loadTexture(const std::string& texImagePath);

std::vector<float> VertexToArray(const std::vector<glm::vec3>& vertexData, const std::vector<int>& indicesData);

std::vector<float> TexCoordsToArray(const std::vector<glm::vec2>& vertexData, const std::vector<int>& indicesData);

#endif // !__AUXFUN__
