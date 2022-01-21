#ifndef TEXTURE_H
#define TEXTURE_H

// Std Headers
#include <iostream>
#include <string>
#include <sstream>

using byte = unsigned char;
using uint = unsigned int;


class Texture
{
public:
	enum filter_type
	{
		NEAREST = 0, LINEAR
	};
public:
	Texture(const char *name, const char *tex_path, uint Unit);
	Texture() {};
	~Texture() = default; 

	// State Setup
	void load();
	void set_params(filter_type filter);

	// Tick Calls
	void activate();
	void bind();

	std::ostringstream debug();

public:
	bool valid_state; 
	int32_t width, height, nChannels; 
	std::string name, filePath;
	uint ID, unit;
	filter_type activeFilter; 
};


#endif