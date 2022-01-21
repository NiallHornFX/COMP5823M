#ifndef SHADER_H
#define SHADER_H

// Ext Headers
// GLEW
#include "ext/GLEW/glew.h" 
// GLM
#include "ext/glm/glm.hpp"
#include "ext/glm/gtc/type_ptr.hpp"

// Std Headers
#include <string>
#include <vector>

// Info : Basic Shader class, that defines a shader program containg a Vertex and Fragment stage. 

class Shader
{
public:
	Shader() {};
	Shader(const char *name, const char *vertparth, const char *fragpath);
	~Shader();

	void load();
	void use();

	// Set Shader Uniforms
	void setBool (const std::string &name, bool value)          const;
	void setInt  (const std::string &name, int value)           const;
	void setFloat(const std::string &name, float value)         const;
	void setVec  (const std::string &name, glm::vec3 value)     const;
	void setMat3 (const std::string &name, glm::mat3x3 value)   const;
	void setMat4 (const std::string &name, glm::mat4x4 value)   const;

	// Debug
	void check_compile(unsigned int &shader, std::string type);
	void check_link();
	void debug_vert();
	void debug_frag();

public:
	std::string name;
	GLuint ID; 
	bool valid_state;

private:
	std::string vert_path, frag_path;
	std::string vert_shader_code, frag_shader_code;
};

#endif 

