#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <GLM/glm/glm.hpp>
#include <GLM/glm/ext.hpp>
#include <stdint.h>
#include<vector>

using byte = uint8_t;
const int ChunkDimensions = 256;

const int BlockDimensions = 16;
const int voxelsInBlock = BlockDimensions * BlockDimensions * BlockDimensions;

const int blocksPerChunk = ChunkDimensions / BlockDimensions;
//void framebuffer_size_callback(GLFWwindow* window, int width, int height);


static void GLClearErrors()
{
	while (glGetError() != GL_NO_ERROR);
}

static void GLPrintErrors()
{
	while (GLenum  error = glGetError())
	{
		std::cout << "[OpenGL Error] (" << error << std::endl;
	}
}



struct Vertex
{
	float x, y, z;

};

struct Instance
{
	byte x, y, z;
};


struct Node
{
	uint32_t data;

	bool isLeaf() {
		return data && 1;
	}

};

struct chunkPosition
{
	byte x, y, z;
};

struct Voxel
{
	byte blockId;
};

struct Block
{
	Voxel voxels[BlockDimensions][BlockDimensions][BlockDimensions];
};

struct Chunk
{
	uint16_t offsetBlock[blocksPerChunk][blocksPerChunk][blocksPerChunk];
	uint32_t indexOffset(int x, int y, int z) { offsetBlock[x][y][z] * sizeof(Block); }
};

struct ShaderData
{
	Chunk chunk;
	Block blocks[4];
};
int getVoxelIndex(int x, int y, int z)
{
	return x + BlockDimensions * y + BlockDimensions * BlockDimensions * z;
}

struct ShaderData2
{
	uint32_t offsetBlock[16][16][16];
	uint32_t blockCount = 1;
	std::vector<uint32_t> packedVoxelData;
	//byte voxels[BlockDimensions * BlockDimensions * BlockDimensions * 4];

	ShaderData2() {
		packedVoxelData.resize(voxelsInBlock);
		memset(offsetBlock, 0, sizeof(offsetBlock));
	}

	void addBlocks(uint32_t numBlocks)
	{
		packedVoxelData.resize((voxelsInBlock/4) * (blockCount+numBlocks),0);
		blockCount += numBlocks;
	}

	void setBlocks(uint32_t numBlocks)
	{
		packedVoxelData.resize((voxelsInBlock/4) * numBlocks,0);
		blockCount = numBlocks;
	}

	byte unpackPacket(uint32_t packet, uint32_t index)
	{
		uint32_t byteMask = 0xff << (index*8);
		return (packet & byteMask) >> (index*8);
	}

	uint32_t packPacket(byte data[4])
	{
		uint32_t r = data[0];
		r |= (data[1] << 8);
		r |= (data[2] << 16);
		r |= (data[3] << 24);

		return r;
	}

	byte fetchVoxel(int x, int y, int z)
	{
		uint32_t offset = offsetBlock[x/16][y/16][z/16];
		uint32_t index = voxelsInBlock * offset + getVoxelIndex(x % 16, y % 16, z % 16);
		return unpackPacket( packedVoxelData[index/4],index%4);
	}

	void setVoxel(int x, int y, int z, byte data)
	{
		uint32_t offset = offsetBlock[x / 16][y / 16][z / 16];
		if (offset == 0) { addBlocks(1); }
		uint32_t index = voxelsInBlock * offset + getVoxelIndex(x % 16, y % 16, z % 16);
		uint32_t packed = packedVoxelData[index/4];
		packed &= (~(0xff << ((index % 4) * 8)));
		packed |= data << ((index % 4) * 8);
		packedVoxelData[index/4] = packed;
		offsetBlock[x / 16][y / 16][z / 16] = blockCount;
	}

	void setVoxel(int x, int y, int z, int blockOffset, byte data)
	{
		uint32_t index = voxelsInBlock * blockOffset + getVoxelIndex(x % 16, y % 16, z % 16);
		uint32_t packed = packedVoxelData[index/4];
		packed &= (~(0xff << ((index % 4) * 8)));
		packed |= data << ((index % 4) * 8);
		packedVoxelData[index/4] = packed;

	}


};
static unsigned int CompileShader(unsigned int type, const std::string& source)
{
	unsigned int id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	
	if (result == GL_FALSE) 
	{
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*) _malloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		std::string typestring;
		if (type == GL_FRAGMENT_SHADER) { typestring = "fragment"; }
		if (type == GL_VERTEX_SHADER) { typestring = "vertex"; }
		if (type == GL_GEOMETRY_SHADER) { typestring = "geometry"; }
		std::cout << "Failed to compile " << typestring << " shader:" << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id);
		return 0;
	}
	
	return id;
}

static unsigned int CreateProgram(const std::string& vertexShader, const std::string& fragmentShader)
{
	unsigned int program = glCreateProgram();
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

static std::string parseFile(const std::string filePath) {
	std::ifstream file(filePath);
	std::string str;
	std::string content;
	while (std::getline(file, str)) {
		content.append(str + "\n");
	}
	return content;
}



float height(float x, float y)
{
	return sin(x/2.0)*8 + 8;
}


void getVoxelPos(int i)
{
	//return[1,1,1];
}




void buildSSBO(ShaderData2 *r)
{
	for (int x = 0; x < BlockDimensions; x++)
	{
		for (int y = 0; y < BlockDimensions; y++)
		{
			for (int z = 0; z < BlockDimensions; z++)
			{
				if (y < height(x, z)) 
				{
					r->setVoxel(x * 16, y * 16, z * 16, 1);
					//r->offsetBlock[x][y][z] = 1;
				}
				else
				{
					//r->offsetBlock[x][y][z] = 0;
				}

			}
		}
	}
	//r->addBlocks(2);
	for (int x = 0; x < BlockDimensions; x++)
	{
		for (int y = 0; y < BlockDimensions; y++)
		{
			for (int z = 0; z < BlockDimensions; z++)
			{
				r->setVoxel(x, y, z, 0, 0);
				r->setVoxel(x, y, z, 1, (byte) sin(x/100.0)*255.0);
			}
		}
	}
}



int main()
{
	
	const int voxelCount = ChunkDimensions * ChunkDimensions * ChunkDimensions;

	ShaderData2* shaderData = new ShaderData2();
	//shaderData->shaderData2
		
	buildSSBO(shaderData);
	
	int renderedBlockCount = 0;
	for (int x = 0; x < blocksPerChunk; x++)
	{
		for (int y = 0; y < blocksPerChunk; y++)
		{
			for (int z = 0; z < blocksPerChunk; z++)
			{
				int blockIndex = shaderData->offsetBlock[x][y][z];
				renderedBlockCount += blockIndex == 0 ? 0 : 1;

			}
		}
	}
	std::vector<byte> blockOffsets(3*renderedBlockCount, 0);

	int currentRenderedBlockCount = 0;
	for (int x = 0; x < blocksPerChunk; x++)
	{
		for (int y = 0; y < blocksPerChunk; y++)
		{
			for (int z = 0; z < blocksPerChunk; z++)
			{
				int blockIndex = shaderData->offsetBlock[x][y][z];
				if (blockIndex != 0)
				{
					blockOffsets[currentRenderedBlockCount * 3 + 0] = x;
					blockOffsets[currentRenderedBlockCount * 3 + 1] = y;
					blockOffsets[currentRenderedBlockCount * 3 + 2] = z;
					currentRenderedBlockCount++;
				}
				
			}
		}
	}
	
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "hello window", NULL, NULL);
	glfwMakeContextCurrent(window);

	glfwSwapInterval(1);



	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	//const int instanceCount = 10;

	//float cubeOffsets[instanceCount * 3];
	//for (int i = 0; i < sizeof(cubeOffsets) / sizeof(cubeOffsets[0]); i++)
	//{
	//	cubeOffsets[i] = (float)i;
	//}



	Vertex cubeVertices[8] = {
		Vertex{0,0,1},
		Vertex{1,0,1},
		Vertex{0,1,1},
		Vertex{1,1,1},
		Vertex{0,0,0},
		Vertex{1,0,0},
		Vertex{0,1,0},
		Vertex{1,1,0}
	};

	
	unsigned int cubeIndices[] =
	{
		//Top
		2, 6, 7,
		2, 3, 7,

		//Bottom
		0, 4, 5,
		0, 1, 5,

		//Left
		0, 2, 6,
		0, 4, 6,

		//Right
		1, 3, 7,
		1, 5, 7,

		//Front
		0, 2, 3,
		0, 1, 3,

		//Back
		4, 6, 7,
		4, 5, 7

	};
	unsigned int vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	unsigned int positionBuffer;
	glGenBuffers(1, &positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);


	unsigned int offsetBuffer;
	glGenBuffers(1, &offsetBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, offsetBuffer);
	glBufferData(GL_ARRAY_BUFFER,currentRenderedBlockCount * 3 * sizeof(byte), &blockOffsets[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);

	glVertexAttribIPointer(1, 3, GL_UNSIGNED_BYTE, 3 * sizeof(byte), (void*)0);
	glVertexAttribDivisor(1, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	int dataSize = sizeof(shaderData->blockCount)+(16*16*16*sizeof(uint32_t)) + shaderData->packedVoxelData.size() * sizeof(uint32_t);

	GLuint ssbo = 0;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, dataSize, shaderData, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	

	
	

	//unsigned int ibo;
	//glGenBuffers(1, &ibo);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

	std::string fragmentShader = parseFile("basic.frag");
	std::string vertexShader = parseFile("basic.vert");

	unsigned int program = CreateProgram(vertexShader,fragmentShader);
	glUseProgram(program);

	

	//GLuint block_index = 0;
	//block_index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, "voxelBuffer");


	int u_TimeLocation = glGetUniformLocation(program, "u_Time");
	int u_MVPLocation = glGetUniformLocation(program, "u_MVP");
	int u_CameraPositionLocation = glGetUniformLocation(program, "u_CameraPosition");

	glm::mat4 Projection(1.0f);
	glm::mat4 View(1.0f);
	glm::mat4 Model(1.0f);

	glm::vec3 cameraPosition = glm::vec3(4, 33, 3);

	View = glm::lookAt(
		cameraPosition, // Camera is at (4,3,3), in World Space
		glm::vec3(8, 0, 8), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	Projection = glm::perspective(1.57079632679f, 800.0f / 600.0f, 0.1f, 800.0f);
	
	std::cout << glGetString(GL_VERSION) << std::endl;

	//glViewport(0, 0, 800, 600);
	GLPrintErrors();

	std::cout << "Rendering " << renderedBlockCount << " cubes, or "<< renderedBlockCount * 12 << " triangles" << std::endl;
	std::cout << "Storing " << shaderData->blockCount << " unique blocks of voxels" << std::endl;

	while (!glfwWindowShouldClose(window))
	{
		float time = (float)glfwGetTime();
		//GLClearErrors();
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		glClearColor(63 / 255.0f, 77 / 255.0f, 99 / 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);

		glm::mat4 mvp = Projection * View * Model;
		glUniformMatrix4fv(u_MVPLocation, 1, GL_FALSE, &mvp[0][0]);
		glUniform1f(u_TimeLocation, time);
		glUniform3f(u_CameraPositionLocation, cameraPosition.x, cameraPosition.y, cameraPosition.z);

		glBindVertexArray(vao);
		glDrawElementsInstanced(GL_TRIANGLES, sizeof(cubeIndices) / sizeof(cubeIndices[0]), GL_UNSIGNED_INT, &cubeIndices, renderedBlockCount);
		glBindVertexArray(0);
		//GLPrintErrors();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteProgram(program);

	glfwTerminate();
	
	delete shaderData;
	return 0;
}
