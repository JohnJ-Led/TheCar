#include <GL/glew.h> //GLEW Library
#include <GLFW/glfw3.h> //GLFW Library
//IO Librarys
#include <iostream> //cout, cerr
#include <fstream> //ifstream
#include <cstdlib> // EXIT_FAILURE
#include <string>
#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h> //Math library
//STBI Header
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_SIMD
#include <stb_image.h> //Image loading util func

//GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <vector>


//Fragment and Vertext Shaders
#pragma region ShaderProgram
/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core\n" #Source
#endif // !GLSL


/* Cube Vertex Shader Source Code*/
const GLchar* lightVertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)
	vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
	vertexTextureCoordinate = textureCoordinate;

	gl_Position = projection * view * vec4(vertexFragmentPos, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Cube Fragment Shader Source Code*/
const GLchar* lightFragmentShaderSource = GLSL(440,

	out vec4 fragmentColor;

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	float shininess;
};

struct DirLight {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct SpotLight {
	vec3 position;
	vec3 direction;
	float cutOff;
	float outerCutOff;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

in vec3 vertexFragmentPos;
in vec3 vertexNormal;
in vec2 vertexTextureCoordinate;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights;
uniform SpotLight spotLight;
uniform Material material;
uniform vec2 uvScale;

// function prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
	// properties
	vec3 norm = normalize(vertexNormal);
	vec3 viewDir = normalize(viewPos - vertexFragmentPos);

	// == =====================================================
	// Our lighting is set up in 3 phases: directional, point lights and an optional flashlight
	// For each phase, a calculate function is defined that calculates the corresponding color
	// per lamp. In the main() function we take all the calculated colors and sum them up for
	// this fragment's final color.
	// == =====================================================
	// phase 1: directional lighting
	vec3 result = CalcDirLight(dirLight, norm, viewDir);
	// phase 2: point lights
	result += CalcPointLight(pointLights, norm, vertexFragmentPos, viewDir);
	// phase 3: spot light
	//result += CalcSpotLight(spotLight, norm, vertexFragmentPos, viewDir);

	fragmentColor = vec4(result, 1.0);
}

// calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);
	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);
	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	// combine results
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, vertexTextureCoordinate * uvScale));
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, vertexTextureCoordinate * uvScale));
	vec3 specular = light.specular * spec * vec3(texture(material.specular, vertexTextureCoordinate * uvScale));
	return (ambient + diffuse + specular);
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - fragPos);
	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);
	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	// attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
	// combine results
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, vertexTextureCoordinate * uvScale));
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, vertexTextureCoordinate * uvScale));
	vec3 specular = light.specular * spec * vec3(texture(material.specular, vertexTextureCoordinate * uvScale));
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
	return (ambient + diffuse + specular);
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - fragPos);
	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);
	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	// attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
	// spotlight intensity
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.cutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
	// combine results
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, vertexTextureCoordinate * uvScale));
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, vertexTextureCoordinate * uvScale));
	vec3 specular = light.specular * spec * vec3(texture(material.specular, vertexTextureCoordinate * uvScale));
	ambient *= attenuation * intensity;
	diffuse *= attenuation * intensity;
	specular *= attenuation * intensity;
	return (ambient + diffuse + specular);
}
);

/*
/* Lamp Shader Source Code
const GLchar* basicVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

		//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code
const GLchar* basicFragmentShaderSource = GLSL(440,

	out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
	fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);*/

/*Vertext Shader Program*/
const GLchar* basicVertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 textCoor;

out vec4 vertexColor;
out vec2 vertexTexture;
out vec2 auvScale;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 uvScale;

void main() {
	gl_Position = projection * view * model * vec4(position, 1.0f);
	vertexColor = color;
	vertexTexture = textCoor; //reference incoming texture data
	auvScale = uvScale;
}
);

const GLchar* basicFragmentShaderSource = GLSL(440,
	out vec4 fragmentColor;

in vec4 vertexColor;
in vec2 vertexTexture;
in vec2 auvScale;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform vec2 uvScale;


void main()
{
	fragmentColor = texture(texture1, vertexTexture * auvScale) + vec4(vertexColor);
}
);
#pragma endregion

//Shader Class
#pragma region ShaderClass

class Shader
{
public:
	unsigned int ID;
	// constructor generates the shader on the fly
	// ------------------------------------------------------------------------
	Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
	{
		// 1. retrieve the vertex/fragment source code from filePath
		std::string vertexCode;
		std::string fragmentCode;
		std::string geometryCode;

		const char* vShaderCode = vertexPath;
		const char* fShaderCode = fragmentPath;
		// 2. compile shaders
		unsigned int vertex, fragment;
		// vertex shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		checkCompileErrors(vertex, "VERTEX");
		// fragment Shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT");
		// if geometry shader is given, compile geometry shader
		unsigned int geometry;
		if (geometryPath != nullptr)
		{
			const char* gShaderCode = geometryCode.c_str();
			geometry = glCreateShader(GL_GEOMETRY_SHADER);
			glShaderSource(geometry, 1, &gShaderCode, NULL);
			glCompileShader(geometry);
			checkCompileErrors(geometry, "GEOMETRY");
		}
		// shader Program
		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		if (geometryPath != nullptr)
			glAttachShader(ID, geometry);
		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");
		// delete the shaders as they're linked into our program now and no longer necessery
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		if (geometryPath != nullptr)
			glDeleteShader(geometry);

	}
	// activate the shader
	// ------------------------------------------------------------------------
	void use()
	{
		glUseProgram(ID);
	}
	// utility uniform functions
	// ------------------------------------------------------------------------
	void setBool(const std::string& name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
	}
	// ------------------------------------------------------------------------
	void setInt(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setFloat(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setVec2(const std::string& name, const glm::vec2& value) const
	{
		glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec2(const std::string& name, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
	}
	// ------------------------------------------------------------------------
	void setVec3(const std::string& name, const glm::vec3& value) const
	{
		glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec3(const std::string& name, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
	}
	// ------------------------------------------------------------------------
	void setVec4(const std::string& name, const glm::vec4& value) const
	{
		glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec4(const std::string& name, float x, float y, float z, float w)
	{
		glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
	}
	// ------------------------------------------------------------------------
	void setMat2(const std::string& name, const glm::mat2& mat) const
	{
		glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat3(const std::string& name, const glm::mat3& mat) const
	{
		glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat4(const std::string& name, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

private:
	// utility function for checking shader compilation/linking errors.
	// ------------------------------------------------------------------------
	void checkCompileErrors(GLuint shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
};
#pragma endregion

//Camera Class
#pragma region CameraClass
// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum class Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// camera Attributes
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	// euler Angles
	float Yaw;
	float Pitch;
	// camera options
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	// constructor with vectors
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = position;
		WorldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}
	// constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = glm::vec3(posX, posY, posZ);
		WorldUp = glm::vec3(upX, upY, upZ);
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}

	// returns the view matrix calculated using Euler Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix() const
	{
		return glm::lookAt(Position, Position + Front, Up);
	}

	// processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard(Camera_Movement direction, float deltaTime)
	{
		float velocity = MovementSpeed * deltaTime;
		if (direction == Camera_Movement::FORWARD)
			Position += Front * velocity;
		if (direction == Camera_Movement::BACKWARD)
			Position -= Front * velocity;
		if (direction == Camera_Movement::LEFT)
			Position -= Right * velocity;
		if (direction == Camera_Movement::RIGHT)
			Position += Right * velocity;
		if (direction == Camera_Movement::UP)
			Position += Up * velocity;
		if (direction == Camera_Movement::DOWN)
			Position -= Up * velocity;
	}

	// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		// make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (Pitch > 89.0f)
				Pitch = 89.0f;
			if (Pitch < -89.0f)
				Pitch = -89.0f;
		}

		// update Front, Right and Up Vectors using the updated Euler angles
		updateCameraVectors();
	}

	// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(float yoffset)
	{
		Zoom -= (float)yoffset;
		if (Zoom < 1.0f)
			Zoom = 1.0f;
		if (Zoom > 45.0f)
			Zoom = 45.0f;
	}

private:
	// calculates the front vector from the Camera's (updated) Euler Angles
	void updateCameraVectors()
	{
		// calculate the new Front vector
		glm::vec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);
		// also re-calculate the Right and Up vector
		Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = glm::normalize(glm::cross(Right, Front));
	}
};
#pragma endregion

//Unnamed namspace
namespace {
	const char* const WINDOW_TITLE = "The Car--Maybe"; //Macro for window title

	//Variable for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	//Stores the GL data relative to a given mesh
	struct GLMesh {
		GLuint vao;		//Handle for the vertex array object
		GLuint vbos[2];	//Handles for the vertex buffer objects
		GLuint nIndices;//Number of indices of the mesh
		GLuint sideVerts;
		GLuint topVerts;
		GLuint bottomVerts;
	};

	//Main GLFW window
	GLFWwindow* gWindow = nullptr;
	//Triangle mesh data
	GLMesh gMesh;
	GLMesh gPlane;
	GLMesh gTire;
	GLMesh gWheel;
	GLMesh gWing;
	GLMesh gCHub;
	GLMesh gSpoke;
	GLMesh gBody;
	GLMesh gSides;
	GLMesh gFront;
	GLMesh gRear;
	GLMesh gCenterTop;
	GLMesh gTop;
	//Shader program
	GLuint gProgramId;
	//ZTexture ID
	GLuint texture1, texture2, texture3, texture4, texture5, texture6;
	GLuint texture7, texture8, texture9, texture10, texture11, texture12;
	GLuint texture13, texture14;
	glm::vec2 gUVScale(2.0f, 2.0f);
	GLint gTexWrapMode = GL_REPEAT;
	//camera
	Camera gCamera(glm::vec3(0.0f, 8.0f, 25.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;
	float cameraSpeed = 8.0f;
	//timing
	float gDeltaTime = 0.0f; //time between current frame and last frame
	float gLastFrame = 0.0f;
//	bool isPerspective = true;
	int kCount = 0;//Used for torus loop

}

/* User-defined Function prototypes to:
*  initialize program, set the window size,
*  redraw graphics on the window when resized,
*  and render graphics on the screen
*/
#pragma region Prototypes
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
//Input Controls
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
//Create Objects and texture
void Plane(GLMesh& mesh);
void Wing(GLMesh& mesh);
void DrawTorus(GLMesh& mesh, float r, float c, int rSeg, int cSeg, int texture, int zMulti);
void DrawCylinder(GLMesh& mesh, GLfloat radius, GLfloat height);
void DrawRectangle(GLMesh& mesh, GLfloat radius, GLfloat height);
void UDestroyMesh(GLMesh& mesh);
void DrawCube(GLMesh& mesh);
void DrawPyramid(GLMesh& mesh);
void DrawWheel(Shader ourShader, GLMesh& tMesh, GLMesh& wMesh, GLMesh& cMesh, GLMesh& sMesh, glm::vec3 loc, glm::vec3 aScale, GLfloat angle, bool sides);
void DrawCar(Shader ourShader, GLMesh& bMesh, GLMesh& fMesh, GLMesh& rMesh, GLMesh& sMesh, GLMesh& cTMesh, GLMesh& tMesh, glm::vec3 loc, glm::vec3 aScale, GLfloat angle);
//Texture Create and Destroy
bool CreateTexture(const char* filename, GLuint& textureId);
void DestroyTexture(GLuint textureID);
//Memory Clean up
//void UDestroyShaderProgram(GLuint programId);
//Push to our shader and put on screen
void URender(Shader aShader, Shader bShader);



//Image Flip
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; j++)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; i--)
		{
			unsigned char temp = image[index1];
			image[index1] = image[index2];
			image[index2] = temp;
			index1++;
			index2++;
		}
	}
}

/*File path test Credit - https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c */
inline bool exists_test0(const std::string& name) {
	std::ifstream f(name.c_str());
	return f.good();
}
#pragma endregion FileTest_flipImage_shaders




int main(int argc, char* argv[]) {


	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	//Create the Meshes
	Plane(gPlane);
	Wing(gWing);
	DrawTorus(gTire, 10.0, 30.0, 30, 36, 0, 2);
	DrawTorus(gWheel, 10.0, 28.0, 30, 36, 0, 2);
	DrawCylinder(gCHub, 10.0f, 11.0f);
	DrawRectangle(gSpoke, 4.0f, 50.0f);
	DrawRectangle(gBody, 4.0f, 11.2f);
	DrawCylinder(gFront, 10.0f, 18.0f);
	DrawCylinder(gRear, 10.0f, 18.0f);
	DrawCylinder(gSides, 15.0f, 14.5f);
	DrawCube(gCenterTop);
	DrawPyramid(gTop);
	//Initiate Shaders
	Shader lightShader(lightVertexShaderSource, lightFragmentShaderSource);
	Shader basicShader(basicVertexShaderSource, basicFragmentShaderSource);
	
	//Load texture (relative to projects directory)
	const char* texFilename[14];
	texFilename[0] = "Resources/Textures/pavement.png";
	texFilename[1] = "Resources/Textures/pavement_specular.png";
	texFilename[2] = "Resources/Textures/TruePaintColor.png";
	texFilename[3] = "Resources/Textures/TruePaintColor_specular3.png";
	texFilename[4] = "Resources/Textures/tire_tread.png";
	texFilename[5] = "Resources/Textures/tire_tread_specular.png";
	texFilename[6] = "Resources/Textures/keyshot-materials-glitter-glass.png";
	texFilename[7] = "Resources/Textures/keyshot-materials-glitter-glass_specular.png";
	texFilename[8] = "Resources/Textures/FrontNose_1.png";
	texFilename[9] = "Resources/Textures/FrontNose_specular.png";
	texFilename[10] = "Resources/Textures/BackSide.png";
	texFilename[11] = "Resources/Textures/BackSide_specular.png";
	texFilename[12] = "Resources/Textures/theSide.png";
	texFilename[13] = "Resources/Textures/theSide_specular.png";
	//keyshot-materials-glitter-glass
	//Test File path
	for (int i = 0; i < 14; i++) {
		if (exists_test0(texFilename[i]))
			std::cout << " file: \""<< texFilename[i] << "\" good" << std::endl;
		else
			std::cout << " file not good" << std::endl;
	}
	//Create Shader
	if (!CreateTexture(texFilename[0] , texture1))
	{
		std::cout << "Failed to load texture " << texFilename[0] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[1], texture2))
	{
		std::cout << "Failed to load texture " << texFilename[1] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[2], texture3))
	{
		std::cout << "Failed to load texture " << texFilename[2] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[3], texture4))
	{
		std::cout << "Failed to load texture " << texFilename[3] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[4], texture5))
	{
		std::cout << "Failed to load texture " << texFilename[4] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[5], texture6))
	{
		std::cout << "Failed to load texture " << texFilename[5] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[6], texture7))
	{
		std::cout << "Failed to load texture " << texFilename[6] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[7], texture8))
	{
		std::cout << "Failed to load texture " << texFilename[7] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[8], texture9))
	{
		std::cout << "Failed to load texture " << texFilename[8] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[9], texture10))
	{
		std::cout << "Failed to load texture " << texFilename[9] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[10], texture11))
	{
		std::cout << "Failed to load texture " << texFilename[10] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[11], texture12))
	{
		std::cout << "Failed to load texture " << texFilename[11] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[12], texture13))
	{
		std::cout << "Failed to load texture " << texFilename[12] << std::endl;
		return EXIT_FAILURE;
	}
	if (!CreateTexture(texFilename[13], texture14))
	{
		std::cout << "Failed to load texture " << texFilename[13] << std::endl;
		return EXIT_FAILURE;
	}
	//DrawTorus(gTorus, 10.0, 30.0, 30, 36, texture1, 2, 1.0f, 1.0f, 0.0f); //Not working yet
	//Sets the background color of the window to block (it will be implicitely used by glClear)

	lightShader.use();
	lightShader.setInt("material.diffuse", 0);
	lightShader.setInt("material.specular", 1);


	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//render loop
	//----------
	int f = 0;
	while (!glfwWindowShouldClose(gWindow)) {

		//input
		//-----
		float currentFrame = (float) glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		//UProcessInput(gWindow);
		glfwSetKeyCallback(gWindow, key_callback);
		//Render this frame
		
		URender(lightShader, basicShader);//Pass the difference 
		//URender(ourShader);

		glfwPollEvents();
	}


	//Release shader program
	UDestroyMesh(gPlane);
	//UDestroyMesh(gTorus);
	UDestroyMesh(gWing);
	UDestroyMesh(gWheel);
	UDestroyMesh(gCHub);
	UDestroyMesh(gTire);
	UDestroyMesh(gSpoke);
	UDestroyMesh(gCenterTop);
	UDestroyMesh(gFront);
	UDestroyMesh(gSides);
	UDestroyMesh(gRear);
	UDestroyMesh(gTop);
	DestroyTexture(texture1);
	DestroyTexture(texture2);
	DestroyTexture(texture3);
	DestroyTexture(texture4);
	DestroyTexture(texture5);
	DestroyTexture(texture6);
	DestroyTexture(texture7);
	DestroyTexture(texture8);
	DestroyTexture(texture9);
	DestroyTexture(texture10);

	exit(EXIT_SUCCESS); //Terminates the program successfully
}

//Initialize GLFW, GLEW, and create a wiundow
bool UInitialize(int argc, char* argv[], GLFWwindow** window) {

	//GLFW: initialize and configure
	//------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif // 

	//GLFW: window creation
	//---------------------
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//GLEW: initalize
	//---------------
	//NoteL if using GFLEW version 1.13 or earlier
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult) {
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	//Displays GPU OpenGL version
	std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

	return true;
}

//Function called to render a frame
void URender(Shader aShader, Shader bShader) {

	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	// Clear the frame and z buffers
	//glClearColor(66/255.0f, 119/255.0f, 166/255.0f, 1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//glBindVertexArray(gPlane.vao);

	aShader.use();
	aShader.setVec3("viewPos", gCamera.Position);
	aShader.setFloat("material.shininess", 256.0f);

	//Manually set position of light sources
	//Direction Light
	aShader.setVec3("dirLight.direction", 2.0f, -2.0f, 0.03f);
	aShader.setVec3("dirLight.ambient", 0.5f, 0.5f, 0.5f);
	aShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
	aShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
	
	aShader.setVec3("pointLights.position", 1.0f, 10.0f, 4.0f);
	aShader.setVec3("pointLights.ambient", 1.0f, 1.0f, 1.0f);
	aShader.setVec3("pointLights.diffuse", 1.0f, 1.0f, 1.0f);
	aShader.setVec3("pointLights.specular", 0.3f, 0.3f, 0.3f);
	aShader.setFloat("pointLights.constant", 1.0f);
	aShader.setFloat("pointLights.linear", 0.09f);
	aShader.setFloat("pointLights.quadratic", 0.032f);

	glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 10000.0f);
	glm::mat4 view = gCamera.GetViewMatrix();//Transforms the camera
	aShader.setMat4("projection", projection);
	aShader.setMat4("view", view);

	glm::mat4 model = glm::mat4(1.0f);
	aShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gUVScale = glm::vec2(25.0f, 25.0f);
	aShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture1);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, texture2);

	glBindVertexArray(gPlane.vao);
	model = glm::mat4(1.0f);

	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(100.0f, 1.0f, 100.0f));
	aShader.setMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, gPlane.nIndices); //Draws the triangles as Points, makes pretty cool output.
	glBindVertexArray(0);//Deactivate the Vertex Array Object

	//----------------------------------------------------------------------------------------------------------
	glBindVertexArray(gWing.vao);
	aShader.use();
	aShader.setVec3("viewPos", gCamera.Position);
	aShader.setFloat("material.shininess", 256.0f);
	aShader.setVec3("dirLight.direction", 2.0f, -2.0f, 0.03f);
	aShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
	aShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
	aShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
	aShader.setFloat("pointLights.constant", 1.0f);
	aShader.setFloat("pointLights.linear", 0.09f);
	aShader.setFloat("pointLights.quadratic", 0.032f);

	view = gCamera.GetViewMatrix();//Transforms the camera
	aShader.setMat4("projection", projection);
	aShader.setMat4("view", view);

	model = glm::mat4(1.0f);
	aShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	gUVScale = glm::vec2(1.0f, 1.0f);
	aShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture3);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture4);

	model = glm::translate(model, glm::vec3(0.0f, 4.05f, 0.6f));

	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f, 1.25f, 1.25f));
	aShader.setMat4("model", model);

	//Draws the triangle
	glDrawArrays(GL_TRIANGLES, 0, gWing.nIndices);//Draws the triangle

	//Deactivate the Vertex Array Object
	glBindVertexArray(0);//Deactivate the Vertex Array Object

	glm::vec3 wheelLocation[] = {
		glm::vec3(-2.5f, 1.0f, 1.0f),
		glm::vec3( 2.5f, 1.0f, 1.0f),
		glm::vec3(-2.5f, 1.0f, 9.0f),
		glm::vec3( 2.5f, 1.0f, 9.0f),
	};

	GLfloat wheelRotation[] = {-90.0f, 90.0f, -90.0f, 90.0f};
	bool side[] = { false,true,false,true };
	glm::vec3 wheelScale = glm::vec3(0.0225f, 0.0225f, 0.0225f);

	for (int i = 0; i < 4; i++) {
		DrawWheel(aShader, gTire, gWheel, gCHub, gSpoke, wheelLocation[i], wheelScale, wheelRotation[i], side[i]);
	}
	glm::vec3 carScale = glm::vec3(0.5f, 0.5f, 1.0f);

	glm::vec3 carLocation = glm::vec3(0.0f, 1.5f, -0.6f);

	DrawCar(aShader, gBody, gFront, gRear, gSides, gCenterTop, gTop, carLocation, carScale, 0.0f);

	//glfw: swap buffers and poll IO events (keys pressed/release, mouse moved etc.)
	glfwSwapBuffers(gWindow); //Flips the back buffer with the front buffer every frame.
}

#pragma region InputControl
//process all input: query GLFW whetehr relevant keys are pressed/released this frame and react accordingly
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {


	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	//std::cout << " Key Press Caught: key-" << key << " action type-" << action << std::endl; //Print Key Presses

	float cameraOffset = cameraSpeed * gDeltaTime;
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		gCamera.ProcessKeyboard(Camera_Movement::FORWARD, cameraOffset);
	if (key == GLFW_KEY_W && action == GLFW_REPEAT)
		gCamera.ProcessKeyboard(Camera_Movement::FORWARD, cameraOffset);
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		gCamera.ProcessKeyboard(Camera_Movement::BACKWARD, cameraOffset);
	if (key == GLFW_KEY_S && action == GLFW_REPEAT)
		gCamera.ProcessKeyboard(Camera_Movement::BACKWARD, cameraOffset);
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
		gCamera.ProcessKeyboard(Camera_Movement::LEFT, cameraOffset);
	if (key == GLFW_KEY_A && action == GLFW_REPEAT)
		gCamera.ProcessKeyboard(Camera_Movement::LEFT, cameraOffset);
	if (key == GLFW_KEY_D && action == GLFW_PRESS)
		gCamera.ProcessKeyboard(Camera_Movement::RIGHT, cameraOffset);
	if (key == GLFW_KEY_D && action == GLFW_REPEAT)
		gCamera.ProcessKeyboard(Camera_Movement::RIGHT, cameraOffset);
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		gCamera.ProcessKeyboard(Camera_Movement::DOWN, cameraOffset);
	if (key == GLFW_KEY_Q && action == GLFW_REPEAT)
		gCamera.ProcessKeyboard(Camera_Movement::DOWN, cameraOffset);
	if (key == GLFW_KEY_E && action == GLFW_PRESS)
		gCamera.ProcessKeyboard(Camera_Movement::UP, cameraOffset);
	if (key == GLFW_KEY_E && action == GLFW_REPEAT)
		gCamera.ProcessKeyboard(Camera_Movement::UP, cameraOffset);
}

/*
* Mouse Position Callback
* Process x/y corrdinates to pass to cam
*/
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos) {

	if (gFirstMouse)
	{
		gLastX = (float) xpos;
		gLastY = (float) ypos;
		gFirstMouse = false;
	}

	float xoffset = (float) xpos - gLastX;
	float yoffset = gLastY - (float) ypos; //reversed since y-coor go from bottom to top

	gLastX = (float) xpos;
	gLastY = (float) ypos;

	gCamera.ProcessMouseMovement((float)xoffset, (float)yoffset);
}
/*
* Mouse Scroll callback.
* Options in camera.h allow for zoom.
* Option here updates cameraSpeed global variable to change movement speed in key call backs.
*/
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {

	cameraSpeed += (float) yoffset;
	//If CameraSpeed is negative value Directions will reverse for keys.
	//Set min speed.
	if (cameraSpeed < 0)
		cameraSpeed = 0.1f;

}
/*
* Mouse Button Callback
* A Mouse with additional buttons may require differnt calls or if keys are assigned to the mouse dirrectly buttons could be passed through mouse. 
* Ex: Side button that is assign to a numeric key could possibly be passed through the mouse callback
*/
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {

	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			std::cout << "Left mouse button pressed" << std::endl;
		else
			std::cout << "Left mouse button released" << std::endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			std::cout << "Middle mouse button pressed" << std::endl;
		else
			std::cout << "Middle mouse button release" << std::endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			std::cout << "Right mouse button pressed" << std::endl;
		else
			std::cout << "Right mouse button release" << std::endl;
	}
	break;

	default:
		std::cout << "Unhandled mouse button event: " << button << std::endl;
		break;
	}
}

//glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}
#pragma endregion 

#pragma region ObjectFunctions
void Plane(GLMesh& mesh)
{
	// Vertex Data
	GLfloat verts[] = {
		//Positions          //Texture Coordinates
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  1.0f, 0.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  0.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  0.0f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  0.0f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  0.0f,  1.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  1.0f, 0.0f,
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNorm = 3;
	const GLuint floatsPerUV = 2;

	mesh.nIndices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNorm + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create VBO
	glGenBuffers(1, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNorm + floatsPerUV);

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNorm, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNorm)));
	glEnableVertexAttribArray(2);
}

//Implements the UCreateMesh function
void Wing(GLMesh& mesh) {

	// Position and Color data
	GLfloat verts[] = {
		//Right Wing Triangle
		//Bottom 
		// Vertex Positions    // Normals			//Texture Coor
		 2.5f,  2.5f, -0.5f,    0.0f, 0.0f, -1.0f,  1.0f,  1.0f,// Bottom-Top Right Vertex 0
		 2.5f,  1.5f, -0.5f,    0.0f, 0.0f, -1.0f,  1.0f,  0.0f,// Bottom-Bottom Right Vertex 1
		 2.0f,  2.5f, -0.5f,    0.0f, 0.0f, -1.0f,  0.0f,  1.0f,// Bottom-Top Left Vertex 3
		 2.0f,  2.5f, -0.5f,    0.0f, 0.0f, -1.0f,  0.0f,  1.0f,// Bottom-Top Left Vertex 3
		 2.0f,  1.5f, -0.5f,    0.0f, 0.0f, -1.0f,  0.0f,  0.0f,// Bottom-Bottom Left Vertex 2
		 2.5f,  1.5f, -0.5f,    0.0f, 0.0f, -1.0f,  1.0f,  0.0f,// Bottom-Bottom Right Vertex 1
		 //Top					    		   
		 2.25f, 2.5f,  0.5f,    0.0f, 0.0f, 1.0f,	0.0f,  1.0f,// Top-Top Right Vertex 4
		 2.25f, 2.25f, 0.5f,    0.0f, 0.0f, 1.0f,	0.0f,  0.75f,// Top-Bottom Right Vertex 5
		 2.0f,  2.5f,  0.5f,    0.0f, 0.0f, 1.0f,	1.0f,  1.0f,// Top-Top Left Vertex 7 --Top Left Top Wing Cube
		 2.0f,  2.5f,  0.5f,    0.0f, 0.0f, 1.0f,	1.0f,  1.0f,// Top-Top Left Vertex 7 --Top Left Top Wing Cube
		 2.25f, 2.25f, 0.5f,    0.0f, 0.0f, 1.0f,	0.0f,  0.75f,// Top-Bottom Right Vertex 5
		 2.0f,  2.25f, 0.5f,    0.0f, 0.0f, 1.0f,	1.0f,  0.75f,// Top-Bottom Left Vertex 6 --Top Left Bottom Wing Cube
		 //Left
		 2.5f,  2.5f, -0.5f,    1.0f, 0.0f, 0.0f,   1.0f,  1.0f,// Bottom-Top Right Vertex 0
		 2.5f,  1.5f, -0.5f,    1.0f, 0.0f, 0.0f,   1.0f,  0.0f,// Bottom-Bottom Right Vertex 1
		 2.25f, 2.25f, 0.5f,    1.0f, 0.0f, 0.0f,	0.0f,  0.75f,// Top-Bottom Right Vertex 5
		 2.25f, 2.25f, 0.5f,    1.0f, 0.0f, 0.0f,	0.0f,  0.75f,// Top-Bottom Right Vertex 5
		 2.25f,  2.5f, 0.5f,    1.0f, 0.0f, 0.0f,	0.0f,  1.0f,// Top-Top Right Vertex 4
		 2.5f,  2.5f, -0.5f,    1.0f, 0.0f, 0.0f,   1.0f,  1.0f,// Bottom-Top Right Vertex 0
		 //Right
		 2.0f,  2.5f, -0.5f,   -1.0f, 0.0f, 0.0f,   0.0f,  1.0f,// Bottom-Top Left Vertex 3
		 2.0f,  1.5f, -0.5f,   -1.0f, 0.0f, 0.0f,   0.0f,  0.0f,// Bottom-Bottom Left Vertex 2
		 2.0f,  2.25f, 0.5f,   -1.0f, 0.0f, 0.0f,	1.0f,  0.75f,// Top-Bottom Left Vertex 6 --Top Left Bottom Wing Cube
		 2.0f,  2.25f, 0.5f,   -1.0f, 0.0f, 0.0f,	1.0f,  0.75f,// Top-Bottom Left Vertex 6 --Top Left Bottom Wing Cube
		 2.0f,  2.5f,  0.5f,   -1.0f, 0.0f, 0.0f,	1.0f,  1.0f,// Top-Top Left Vertex 7 --Top Left Top Wing Cube
		 2.0f,  2.5f, -0.5f,   -1.0f, 0.0f, 0.0f,   0.0f,  1.0f,// Bottom-Top Left Vertex 3
		 //Back
		 2.5f,  2.5f, -0.5f,    0.0f, 0.0f, -1.0f,  1.0f,  1.0f,// Bottom-Top Right Vertex 0
		 2.0f,  2.5f, -0.5f,    0.0f, 0.0f, -1.0f,  0.0f,  1.0f,// Bottom-Top Left Vertex 3
		 2.25f,  2.5f,  0.5f,   0.0f, 0.0f, -1.0f,	1.0f,  0.0f,// Top-Top Right Vertex 4
		 2.0f,  2.5f,  0.5f,    0.0f, 0.0f, -1.0f,	0.0f,  0.0f,// Top-Top Left Vertex 7 --Top Left Top Wing Cube
		 2.25f,  2.5f,  0.5f,   0.0f, 0.0f, -1.0f,	1.0f,  0.0f,// Top-Top Right Vertex 4
		 2.0f,  2.5f, -0.5f,    0.0f, 0.0f, -1.0f,  0.0f,  1.0f,// Bottom-Top Left Vertex 3
		 //Front
		 2.5f,  1.5f, -0.5f,    0.0f, 0.0f, 1.0f,  1.0f,  0.0f,// Bottom-Bottom Right Vertex 1
		 2.0f,  1.5f, -0.5f,    0.0f, 0.0f, 1.0f,  0.0f,  0.0f,// Bottom-Bottom Left Vertex 2
		 2.0f,  2.25f, 0.5f,    0.0f, 0.0f, 1.0f,	0.0f,  1.0f,// Top-Bottom Left Vertex 6 --Top Left Bottom Wing Cube
		 2.0f,  2.25f, 0.5f,    0.0f, 0.0f, 1.0f,	0.0f,  1.0f,// Top-Bottom Left Vertex 6 --Top Left Bottom Wing Cube
		 2.25f, 2.25f, 0.5f,    0.0f, 0.0f, 1.0f,	1.0f,  1.0f,// Top-Bottom Right Vertex 5
		 2.5f,  1.5f, -0.5f,    0.0f, 0.0f, 1.0f,  1.0f,  0.0f,// Bottom-Bottom Right Vertex 1


		  //Left Wing Triangle						
		  //Bottom
		 -2.0f,  2.5f, -0.5f,   0.0f, 0.0f, -1.0f,	 0.0f,  0.0f, // Bottom-Top Right Vertex 8
		 -2.0f,  1.5f, -0.5f,   0.0f, 0.0f, -1.0f,	 0.0f,  1.0f, // Bottom-Bottom Right Vertex 9
		 -2.5f,  2.5f, -0.5f,   0.0f, 0.0f, -1.0f,	 1.0f,  0.0f, // Bottom-Top Left Vertex 11
		 -2.5f,  2.5f, -0.5f,   0.0f, 0.0f, -1.0f,	 1.0f,  0.0f, // Bottom-Top Left Vertex 11
		 -2.5f,  1.5f, -0.5f,   0.0f, 0.0f, -1.0f,	 1.0f,  1.0f, // Bottom-Bottom Left Vertex 10
		 -2.0f,  1.5f, -0.5f,   0.0f, 0.0f, -1.0f,	 0.0f,  1.0f, // Bottom-Bottom Right Vertex 9
		 //Top					      	   	
		 -2.0f,  2.5f,  0.5f,   0.0f, 0.0f, 1.0f,	1.0f,  0.0f, // Top-Top Right Vertex 12 
		 -2.0f,  2.25f, 0.5f,   0.0f, 0.0f, 1.0f,	1.0f,  0.25f, // Top-Bottom Right Vertex 13
		 -2.25f, 2.5f,  0.5f,   0.0f, 0.0f, 1.0f,	0.0f,  0.0f, // Top-Top Left Vertex 15 -- Top Right Top Wing Cube
		 -2.25f, 2.5f,  0.5f,   0.0f, 0.0f, 1.0f,	0.0f,  0.0f, // Top-Top Left Vertex 15 -- Top Right Top Wing Cube
		 -2.25f, 2.25f, 0.5f,   0.0f, 0.0f, 1.0f,	0.0f,  0.25f, // Top-Bottom Left Vertex 14 --Top Right Bottom Wing Cube
		 -2.0f,  2.25f, 0.5f,   0.0f, 0.0f, 1.0f,	1.0f,  0.25f, // Top-Bottom Right Vertex 13
		 //Right
		 -2.25f, 2.5f,  0.5f,   0.0f, 1.0f, 0.0f,	0.0f,  0.0f, // Top-Top Left Vertex 15 -- Top Right Top Wing Cube
		 -2.25f, 2.25f, 0.5f,   0.0f, 1.0f, 0.0f,	0.0f,  0.25f, // Top-Bottom Left Vertex 14 --Top Right Bottom Wing Cube
		 -2.5f,  1.5f, -0.5f,   0.0f, 1.0f, 0.0f,	1.0f,  1.0f, // Bottom-Bottom Left Vertex 10
		 -2.5f,  1.5f, -0.5f,   0.0f, 1.0f, 0.0f,	1.0f,  1.0f, // Bottom-Bottom Left Vertex 10
		 -2.5f,  2.5f, -0.5f,   0.0f, 1.0f, 0.0f,	1.0f,  0.0f, // Bottom-Top Left Vertex 11
		 -2.25f, 2.5f,  0.5f,   0.0f, 1.0f, 0.0f,	0.0f,  0.0f, // Top-Top Left Vertex 15 -- Top Right Top Wing Cube
		 //Left
		 -2.0f,  2.5f,  0.5f,   0.0f, -1.0f, 0.0f,	1.0f,  0.0f, // Top-Top Right Vertex 12 
		 -2.0f,  2.25f, 0.5f,   0.0f, -1.0f, 0.0f,	1.0f,  0.25f, // Top-Bottom Right Vertex 13
		 -2.0f,  1.5f, -0.5f,   0.0f, -1.0f, 0.0f,	0.0f,  1.0f, // Bottom-Bottom Right Vertex 9
		 -2.0f,  1.5f, -0.5f,   0.0f, -1.0f, 0.0f,	0.0f,  1.0f, // Bottom-Bottom Right Vertex 9
		 -2.0f,  2.5f, -0.5f,   0.0f, -1.0f, 0.0f,	0.0f,  0.0f, // Bottom-Top Right Vertex 8
		 -2.0f,  2.5f,  0.5f,   0.0f, -1.0f, 0.0f,	1.0f,  0.0f, // Top-Top Right Vertex 12 
		 //Back
		 -2.0f,  2.5f, -0.5f,   0.0f, 0.0f, -1.0f,	0.0f,  0.0f, // Bottom-Top Right Vertex 8
		 -2.5f,  2.5f, -0.5f,   0.0f, 0.0f, -1.0f,	1.0f,  0.0f, // Bottom-Top Left Vertex 11
		 -2.25f, 2.5f,  0.5f,   0.0f, 0.0f, -1.0f,	1.0f,  1.0f, // Top-Top Left Vertex 15 -- Top Right Top Wing Cube
		 -2.25f, 2.5f,  0.5f,   0.0f, 0.0f, -1.0f,	1.0f,  1.0f, // Top-Top Left Vertex 15 -- Top Right Top Wing Cube
		 -2.0f,  2.5f,  0.5f,   0.0f, 0.0f, -1.0f,	0.0f,  1.0f, // Top-Top Right Vertex 12 
		 -2.0f,  2.5f, -0.5f,   0.0f, 0.0f, -1.0f,	0.0f,  0.0f, // Bottom-Top Right Vertex 8
		 //Front
		 -2.0f,  1.5f, -0.5f,   0.0f, 0.0f, 1.0f,	0.0f,  1.0f, // Bottom-Bottom Right Vertex 9
		 -2.5f,  1.5f, -0.5f,   0.0f, 0.0f, 1.0f,	1.0f,  1.0f, // Bottom-Bottom Left Vertex 10
		 -2.25f, 2.25f, 0.5f,   0.0f, 0.0f, 1.0f,	1.0f,  0.0f, // Top-Bottom Left Vertex 14 --Top Right Bottom Wing Cube
		 -2.25f, 2.25f, 0.5f,   0.0f, 0.0f, 1.0f,	1.0f,  0.0f, // Top-Bottom Left Vertex 14 --Top Right Bottom Wing Cube
		 -2.0f,  2.25f, 0.5f,   0.0f, 0.0f, 1.0f,	0.0f,  0.0f, // Top-Bottom Right Vertex 13
		 -2.0f,  1.5f, -0.5f,   0.0f, 0.0f, 1.0f,	0.0f,  1.0f, // Bottom-Bottom Right Vertex 9

		 //Bottom of Cube Wing	 		    
		  2.0f,  2.5f,  0.25f,  0.0f, 0.0f, -1.0f,	 0.0f,  1.0f,// Bottom-Top Left Vertex 16
		  2.0f,  2.25f, 0.25f,  0.0f, 0.0f, -1.0f,	 0.0f,  0.0f,// Bottom-Bottom Left Vertex 17
		 -2.0f,  2.5f,  0.25f,  0.0f, 0.0f, -1.0f,	 1.0f,  1.0f,// Bottom-Top Right Vertex 19
		 -2.0f,  2.5f,  0.25f,  0.0f, 0.0f, -1.0f,	 1.0f,  1.0f,// Bottom-Top Right Vertex 19
		  2.0f,  2.25f, 0.25f,  0.0f, 0.0f, -1.0f,	 0.0f,  0.0f,// Bottom-Bottom Left Vertex 17
		 -2.0f,  2.25f, 0.25f,  0.0f, 0.0f, -1.0f,	 1.0f,  0.0f,// Bottom-Bottom Right Vertex 18
		 //Top
		  2.0f,  2.5f,  0.35f,  0.0f, 0.0f, 1.0f,	 0.0f,  1.0f,// Bottom-Top Left Vertex 20
		  2.0f,  2.25f, 0.35f,  0.0f, 0.0f, 1.0f,	 0.0f,  0.0f,// Bottom-Bottom Left Vertex 21
		 -2.0f,  2.5f,  0.35f,  0.0f, 0.0f, 1.0f,	 1.0f,  1.0f,// Bottom-Top Right Vertex 23
		 -2.0f,  2.5f,  0.35f,  0.0f, 0.0f, 1.0f,	 1.0f,  1.0f,// Bottom-Top Right Vertex 23
		  2.0f,  2.25f, 0.35f,  0.0f, 0.0f, 1.0f,	 0.0f,  0.0f,// Bottom-Bottom Left Vertex 21
		 -2.0f,  2.25f, 0.35f,  0.0f, 0.0f, 1.0f,	 1.0f,  0.0f,// Bottom-Bottom Right Vertex 22
		 //Front
		  2.0f, 2.25f, 0.25f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f,// Bottom-Bottom Left Vertex 17
		 -2.0f, 2.25f, 0.25f,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f,// Bottom-Bottom Right Vertex 18
		 -2.0f, 2.25f, 0.35f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f,// Bottom-Bottom Right Vertex 22
		 -2.0f, 2.25f, 0.35f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f,// Bottom-Bottom Right Vertex 22
		  2.0f, 2.25f, 0.35f,   0.0f, -1.0f, 0.0f,   0.0f, 1.0f,// Bottom-Bottom Left Vertex 21
		  2.0f, 2.25f, 0.25f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f,// Bottom-Bottom Left Vertex 17
		 //Back						
		  2.0f, 2.5f, 0.25f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,// Bottom-Top Left Vertex 16
		 -2.0f, 2.5f, 0.25f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,// Bottom-Top Right Vertex 19
		 -2.0f, 2.5f, 0.35f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,// Bottom-Top Right Vertex 23
		 -2.0f, 2.5f, 0.35f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,// Bottom-Top Right Vertex 23
		  2.0f, 2.5f, 0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,// Bottom-Top Left Vertex 20
		  2.0f, 2.5f, 0.25f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,// Bottom-Top Left Vertex 16
	};


	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	glGenVertexArrays(1, &mesh.vao); //we can also generate multiple VAOs or bufferes at the same time
	glBindVertexArray(mesh.vao);

	//Create 2 buffers: first one for the vertex datal second one for the indices
	glGenBuffers(2, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	mesh.nIndices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	//Strides between vertex coordinates is 6 (x, y, z, xn, yn, zn, u, v). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV); //The number of floats before each

	//Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void DrawTorus(GLMesh& mesh, float r, float c, int rSeg, int cSeg, int texture, int zMulti)
{

	//DrawTorus(100.0, 300.0, 6, 10, 0, 2, 1.0f, 0.0f, 0.0f); Reference what im passing.
	std::vector<GLfloat> newTorusVertices;

	const float TAU = 2.0f * (float)M_PI;

		for (int i = 0; i < rSeg; i++) {
		for (int j = 0; j <= cSeg; j++) {
			for (int k = 0; k <= 1; k++) {
				float s = (float)((i + k) % rSeg + 0.5);
				float t = (float)(j % (cSeg + 1));
				
				float x = (float)(zMulti * ((c + 0.5 * r * cos(s * TAU / rSeg)) * cos(t * TAU / cSeg)));
				float y = (float)(zMulti * ((c + 0.5 * r * cos(s * TAU / rSeg)) * sin(t * TAU / cSeg)));
				float z = (float)(zMulti * (zMulti * r * sin(s * TAU / rSeg)));
				
				float u = (i + k) / (float)rSeg;
				float v = t / (float)cSeg;
				float mag = (float)(sqrt(pow(x,2) + pow(y,2) + pow(z,2)));
				newTorusVertices.push_back(x);
				newTorusVertices.push_back(y);
				newTorusVertices.push_back(z);
				newTorusVertices.push_back(x/mag);
				newTorusVertices.push_back(y/mag);
				newTorusVertices.push_back(z/mag);
				newTorusVertices.push_back(u);
				newTorusVertices.push_back(v);
				//std::cout << "(" <<  x << "," << y << "," << z << "," << x/mag << "," << y/mag << "," << z/mag << "," << u << "," << v << ")" << std::endl;
			}
		}
	}
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nIndices = newTorusVertices.size() / (floatsPerVertex + floatsPerNormal + floatsPerUV);
	// Strides between vertex coordinates
	GLint stride = sizeof(GLfloat) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create VBO
	glGenBuffers(1, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, newTorusVertices.size() * sizeof(GLfloat), &newTorusVertices.at(0), GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU


	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(GLfloat) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(GLfloat) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);

	newTorusVertices.clear();//Memory Management
	
}

void DrawCylinder(GLMesh& mesh, GLfloat radius, GLfloat height)
{
	GLfloat x = 0.0f;
	GLfloat y = 0.0f;
	GLfloat angle = 0.0f;
	GLfloat angle_stepsize = 0.1f;
	std::vector<GLfloat> cylinderVertices;
	mesh.sideVerts = 0;
	mesh.topVerts = 0;
	mesh.bottomVerts = 0;
	float mag1 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(height, 2));
	float mag2 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(0.0, 2));
	///** Draw the tube */
	angle = 0.0;
	while (angle <= 2 * M_PI) {
		x = radius * cos(angle);
		y = radius * sin(angle);
		mag1 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(height, 2));
		mag2 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(0.0, 2));
		cylinderVertices.insert(cylinderVertices.end(), { x, y, height, x / mag1, y / mag1, height / mag1, angle, 1.0f } );//Top
		cylinderVertices.insert(cylinderVertices.end(), { x, y, 0.0f, x / mag2, y / mag2, 0.0f, angle, 0.0f });//Bottom
		angle = angle + angle_stepsize;
		mesh.sideVerts+= 2;
	}
	mag1 = (float)sqrt(pow(radius, 2) + pow(0.0, 2) + pow(height, 2));
	mag2 = (float)sqrt(pow(radius, 2) + pow(0.0, 2) + pow(0.0, 2));
	cylinderVertices.insert(cylinderVertices.end(), { radius, 0.0, height, radius / mag1, 0.0f, height / mag1, angle, 1.0f });
	cylinderVertices.insert(cylinderVertices.end(), { radius, 0.0, 0.0f, radius / mag2, 0.0f, 0.0f, angle, 0.0f });
	mesh.sideVerts+= 2;

	/** Draw the circle on top of cylinder */
	angle = 0.0;
	while (angle <= 2 * M_PI) {
		x = radius * cos(angle);
		y = radius * sin(angle);
		mag1 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(height, 2));
		cylinderVertices.insert(cylinderVertices.end(), { x, y, height, x / mag1, y / mag1, height / mag1, angle, 1.0f });
		//glVertex3f(x, y, height);
		angle = angle + angle_stepsize;
		mesh.topVerts++;
	}
	mesh.topVerts++;
	mag1 = (float)sqrt(pow(radius, 2) + pow(0.0, 2) + pow(height, 2));
	cylinderVertices.insert(cylinderVertices.end(), { radius, 0.0, height, radius / mag1, 0.0f, height / mag1, angle, 1.0f });
	
	angle = 0.0;
	while (angle <= 2 * M_PI) {
		x = radius * cos(angle);
		y = radius * sin(angle);

		mag1 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(0.0, 2));
		cylinderVertices.insert(cylinderVertices.end(), { x, y, 0.0, x / mag1, y / mag1, 0.0, angle, 1.0f });
		angle = angle + angle_stepsize;
		mesh.bottomVerts++;
	}
	mag1 = (float)sqrt(pow(radius, 2) + pow(0.0, 2) + pow(height, 2));
	cylinderVertices.insert(cylinderVertices.end(), { radius, 0.0, height, radius / mag1, 0.0f, height / mag1, angle, 1.0f });
	mesh.bottomVerts++;

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nIndices = cylinderVertices.size() / (floatsPerVertex + floatsPerNormal + floatsPerUV);
	// Strides between vertex coordinates
	GLint stride = sizeof(GLfloat) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create VBO
	glGenBuffers(1, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, cylinderVertices.size() * sizeof(GLfloat), &cylinderVertices.at(0), GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU


	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(GLfloat) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(GLfloat) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);

	cylinderVertices.clear();//Memory management

}

void DrawRectangle(GLMesh& mesh, GLfloat radius, GLfloat height) {
	GLfloat x = 0.0f;
	GLfloat y = 0.0f;
	int numSlices = 4;
	GLfloat angle = 0.0f;
	GLfloat angle_stepsize = 2.0f * (float)M_PI/(float)numSlices;
	std::vector<GLfloat> cylinderVertices;
	mesh.sideVerts = 0;
	mesh.topVerts = 0;
	mesh.bottomVerts = 0;
	float mag1 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(height, 2));
	float mag2 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(0.0, 2));
	///** Draw the tube */
	angle = 0.0;
	while (angle <= numSlices+1) {
		x = radius * cos(angle);
		y = radius * sin(angle);
		mag1 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(height, 2));
		mag2 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(0.0, 2));
		cylinderVertices.insert(cylinderVertices.end(), { x, y, height, x / mag1, y / mag1, height / mag1, angle, 1.0f });//Top
		cylinderVertices.insert(cylinderVertices.end(), { x, y, 0.0f, x / mag2, y / mag2, 0.0f, angle, 0.0f });//Bottom
		angle = angle + angle_stepsize;
		mesh.sideVerts += 2;
	}
	mag1 = (float)sqrt(pow(radius, 2) + pow(0.0, 2) + pow(height, 2));
	mag2 = (float)sqrt(pow(radius, 2) + pow(0.0, 2) + pow(0.0, 2));
	cylinderVertices.insert(cylinderVertices.end(), { radius, 0.0, height, radius / mag1, 0.0f, height / mag1, angle, 1.0f });
	cylinderVertices.insert(cylinderVertices.end(), { radius, 0.0, 0.0f, radius / mag2, 0.0f, 0.0f, angle, 0.0f });
	mesh.sideVerts += 2;

	/** Draw the circle on top of cylinder */
	angle = 0.0;
	while (angle <= numSlices+1) {
		x = radius * cos(angle);
		y = radius * sin(angle);
		mag1 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(height, 2));
		cylinderVertices.insert(cylinderVertices.end(), { x, y, height, x / mag1, y / mag1, height / mag1, angle, 1.0f });
		//glVertex3f(x, y, height);
		angle = angle + angle_stepsize;
		mesh.topVerts++;
	}
	mesh.topVerts++;
	mag1 = (float)sqrt(pow(radius, 2) + pow(0.0, 2) + pow(height, 2));
	cylinderVertices.insert(cylinderVertices.end(), { radius, 0.0, height, radius / mag1, 0.0f, height / mag1, angle, 1.0f });
	angle = 0.0;
	while (angle <= numSlices+1) {
		x = radius * cos(angle);
		y = radius * sin(angle);

		mag1 = (float)sqrt(pow(x, 2) + pow(y, 2) + pow(0.0, 2));
		cylinderVertices.insert(cylinderVertices.end(), { x, y, 0.0, x / mag1, y / mag1, 0.0, angle, 1.0f });
		angle = angle + angle_stepsize;
		mesh.bottomVerts++;
	}
	mag1 = (float)sqrt(pow(radius, 2) + pow(0.0, 2) + pow(height, 2));
	cylinderVertices.insert(cylinderVertices.end(), { radius, 0.0, 0.0f, radius / mag1, 0.0f, 0.0f, angle, 1.0f });
	mesh.bottomVerts++;

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nIndices = cylinderVertices.size() / (floatsPerVertex + floatsPerNormal + floatsPerUV);
	// Strides between vertex coordinates
	GLint stride = sizeof(GLfloat) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create VBO
	glGenBuffers(1, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, cylinderVertices.size() * sizeof(GLfloat), &cylinderVertices.at(0), GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU


	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(GLfloat) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(GLfloat) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);

	cylinderVertices.clear();//Memory management

}

void DrawPyramid(GLMesh& mesh) {

	// Position and Color data
	GLfloat verts[] = {
		//Positions          //Normals
		// ------------------------------------------------------
		//Back Face          //Negative Z Normal  Texture Coords.
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
		0.25f, 0.5f, -0.25f, 0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		0.25f, 0.5f, -0.25f, 0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
	   -0.25f, 0.5f, -0.25f, 0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

	   //Front Face         //Positive Z Normal
	  -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
	   0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
	   0.25f, 0.5f,  0.25f, 0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
	   0.25f, 0.5f,  0.25f, 0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
	  -0.25f, 0.5f,  0.25f, 0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
	  -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

	  //Left Face          //Negative X Normal
	 -0.25f, 0.5f,  0.25f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
	 -0.25f, 0.5f, -0.25f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
	 -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	 -0.25f, 0.5f,  0.25f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

	 //Right Face         //Positive X Normal
	 0.25f,  0.5f,  0.25f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
	 0.25f,  0.5f, -0.25f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
	 0.5f,  -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 0.5f,  -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 0.5f,  -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	 0.25f,  0.5f,  0.25f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

	 //Bottom Face        //Negative Y Normal
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

	//Top Face           //Positive Y Normal
   -0.25f,  0.5f, -0.25f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
	0.25f,  0.5f, -0.25f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
	0.25f,  0.5f,  0.25f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
	0.25f,  0.5f,  0.25f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
   -0.25f,  0.5f,  0.25f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
   -0.25f,  0.5f, -0.25f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
	};


	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nIndices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
	// Strides between vertex coordinates
	GLint stride = sizeof(GLfloat) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create VBO
	glGenBuffers(1, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU


	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(GLfloat) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(GLfloat) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void DrawCube(GLMesh& mesh) {

	// Position and Color data
	GLfloat verts[] = {
		//Positions          //Normals
		// ------------------------------------------------------
		//Back Face          //Negative Z Normal  Texture Coords.
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
	   -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

	   //Front Face         //Positive Z Normal
	  -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
	   0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
	   0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
	   0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
	  -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
	  -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

	  //Left Face          //Negative X Normal
	 -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
	 -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
	 -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	 -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

	 //Right Face         //Positive X Normal
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

	 //Bottom Face        //Negative Y Normal
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

	//Top Face           //Positive Y Normal
   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
	0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
	0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
	0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
   -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
	};


	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nIndices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
	// Strides between vertex coordinates
	GLint stride = sizeof(GLfloat) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create VBO
	glGenBuffers(1, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU


	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(GLfloat) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(GLfloat) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}


void DrawWheel(Shader ourShader, GLMesh& tMesh, GLMesh& wMesh, GLMesh& cMesh, GLMesh& sMesh, glm::vec3 loc, glm::vec3 aScale, GLfloat angle, bool sides) {
	
	glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 10000.0f);
	glm::mat4 view = gCamera.GetViewMatrix();//Transforms the camera
	glm::mat4 model = glm::mat4(1.0f);
	//Create Tire
	glBindVertexArray(tMesh.vao);
	//Tire Lighting
	ourShader.use();
	ourShader.setVec3("viewPos", gCamera.Position);
	ourShader.setFloat("material.shininess", 9.99f);
	ourShader.setVec3("pointLights.position", 0.0f, 6.0f, -3.0f);
	ourShader.setVec3("pointLights.ambient", 0.02f, 0.02f, 0.02f);
	ourShader.setVec3("pointLights.diffuse", 0.01f, 0.01f, 0.01f);
	ourShader.setVec3("pointLights.specular", 0.4f, 0.4f, 0.4f);
	ourShader.setFloat("pointLights.constant", 1.0f);
	ourShader.setFloat("pointLights.linear", 0.09f);
	ourShader.setFloat("pointLights.quadratic", 0.032f);

	view = gCamera.GetViewMatrix();//Transforms the camera
	ourShader.setMat4("projection", projection);
	ourShader.setMat4("view", view);

	model = glm::mat4(1.0f);
	ourShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	gUVScale = glm::vec2(3.0f, 3.0f);
	ourShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture5);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture6);


	model = glm::translate(model, loc);
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, aScale);
	ourShader.setMat4("model", model);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, tMesh.nIndices);//Draws the triangle


	glBindVertexArray(0);//Deactivate the Vertex Array Object
	//Create Wheel
	glBindVertexArray(wMesh.vao);
	ourShader.use();
	ourShader.setVec3("viewPos", gCamera.Position);
	ourShader.setFloat("material.shininess", 256.0f);
	ourShader.setVec3("pointLights.position", 0.0f, 6.0f, -3.0f);
	ourShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
	ourShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
	ourShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
	ourShader.setFloat("pointLights.constant", 1.0f);
	ourShader.setFloat("pointLights.linear", 0.09f);
	ourShader.setFloat("pointLights.quadratic", 0.032f);

	view = gCamera.GetViewMatrix();//Transforms the camera
	ourShader.setMat4("projection", projection);
	ourShader.setMat4("view", view);

	model = glm::mat4(1.0f);
	ourShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	gUVScale = glm::vec2(1.0f, 1.0f);
	ourShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture3);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture4);


	model = glm::translate(model, loc);
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, aScale);
	ourShader.setMat4("model", model);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, wMesh.nIndices);//Draws the triangle


	glBindVertexArray(0);//Deactivate the Vertex Array Object
	//Create Center Hub of Wheel
	glBindVertexArray(cMesh.vao);
	ourShader.use();
	ourShader.setVec3("viewPos", gCamera.Position);
	ourShader.setFloat("material.shininess", 256.0f);
	ourShader.setVec3("pointLights.position", 0.0f, 6.0f, -3.0f);
	ourShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
	ourShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
	ourShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
	ourShader.setFloat("pointLights.constant", 1.0f);
	ourShader.setFloat("pointLights.linear", 0.09f);
	ourShader.setFloat("pointLights.quadratic", 0.032f);

	view = gCamera.GetViewMatrix();//Transforms the camera
	ourShader.setMat4("projection", projection);
	ourShader.setMat4("view", view);

	model = glm::mat4(1.0f);
	ourShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gUVScale = glm::vec2(1.0f, 1.0f);
	ourShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture3);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture4);
	glm::vec3 moveHub;
	if(sides)
		moveHub = glm::vec3(15.0f, 0.0f, 0.0f) * aScale;
	else
		moveHub = glm::vec3(-15.0f, 0.0f, 0.0f) * aScale;

	model = glm::translate(model, loc + moveHub);
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, aScale);
	ourShader.setMat4("model", model);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, cMesh.sideVerts);//Draws the triangle
	glDrawArrays(GL_TRIANGLE_FAN, cMesh.sideVerts, cMesh.topVerts);//Draws the triangle
	glDrawArrays(GL_TRIANGLE_FAN, cMesh.sideVerts + cMesh.topVerts, cMesh.bottomVerts);//Draws the triangle

	glBindVertexArray(0);//Deactivate the Vertex Array Object
	//Spokes Loop
	GLfloat step = 0.0f;
	for (int i = 0; i < 8; i++) {
		//Spoke
		glBindVertexArray(sMesh.vao);
		ourShader.use();
		ourShader.setVec3("viewPos", gCamera.Position);
		ourShader.setFloat("material.shininess", 256.0f);
		ourShader.setVec3("pointLights.position", 0.0f, 6.0f, -3.0f);
		ourShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
		ourShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
		ourShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
		ourShader.setFloat("pointLights.constant", 1.0f);
		ourShader.setFloat("pointLights.linear", 0.09f);
		ourShader.setFloat("pointLights.quadratic", 0.032f);

		view = gCamera.GetViewMatrix();//Transforms the camera
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

		model = glm::mat4(1.0f);
		ourShader.setMat4("model", model);
		//Bind diffuse map
		glActiveTexture(GL_TEXTURE0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		gUVScale = glm::vec2(1.0f, 1.0f);
		ourShader.setVec2("uvScale", gUVScale);
		glBindTexture(GL_TEXTURE_2D, texture3);
		//bind specular map
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture4);
		if(sides)
			moveHub = glm::vec3(22.0f, 0.0f, 0.0f) * aScale;
		else
			moveHub = glm::vec3(-22.0f, 0.0f, 0.0f) * aScale;

		model = glm::translate(model, loc + moveHub);
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::rotate(model, glm::radians(step), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, aScale);

		ourShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, sMesh.sideVerts);//Draws the triangle
		glDrawArrays(GL_TRIANGLE_FAN, sMesh.sideVerts, sMesh.topVerts);//Draws the triangle
		glDrawArrays(GL_TRIANGLE_FAN, sMesh.sideVerts + sMesh.topVerts, sMesh.bottomVerts);//Draws the triangle

		glBindVertexArray(0);//Deactivate the Vertex Array Object
		step += 45.0f;
	}
}

void DrawCar(Shader ourShader, GLMesh& bMesh, GLMesh& fMesh, GLMesh& rMesh, GLMesh& sMesh, GLMesh& cTMesh, GLMesh& tMesh, glm::vec3 loc, glm::vec3 aScale, GLfloat angle) {

	glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 10000.0f);
	glm::mat4 view = gCamera.GetViewMatrix();//Transforms the camera
	glm::mat4 model = glm::mat4(1.0f);
	#pragma region carBody
	//Create Body
	glBindVertexArray(bMesh.vao);
	//Lighting
	ourShader.use();
	ourShader.setVec3("viewPos", gCamera.Position);
	ourShader.setFloat("material.shininess", 256.0f);
	ourShader.setVec3("dirLight.direction", 2.0f, -2.0f, 0.03f);
	ourShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
	ourShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
	ourShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
	ourShader.setFloat("pointLights.constant", 1.0f);
	ourShader.setFloat("pointLights.linear", 0.09f);
	ourShader.setFloat("pointLights.quadratic", 0.032f);

	view = gCamera.GetViewMatrix();//Transforms the camera
	ourShader.setMat4("projection", projection);
	ourShader.setMat4("view", view);

	model = glm::mat4(1.0f);
	ourShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	gUVScale = glm::vec2(3.0f, 3.0f);
	ourShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture3);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture4);


	model = glm::translate(model, loc);
	model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, aScale);
	ourShader.setMat4("model", model);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, bMesh.nIndices);//Draws the triangle


	glBindVertexArray(0);//Deactivate the Vertex Array Object

	//Center Top
	glBindVertexArray(cTMesh.vao);
	//Lighting
	ourShader.use();
	ourShader.setVec3("viewPos", gCamera.Position);
	ourShader.setFloat("material.shininess", 256.0f);
	ourShader.setVec3("dirLight.direction", 2.0f, -2.0f, 0.03f);
	ourShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
	ourShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
	ourShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
	ourShader.setFloat("pointLights.constant", 1.0f);
	ourShader.setFloat("pointLights.linear", 0.09f);
	ourShader.setFloat("pointLights.quadratic", 0.032f);

	view = gCamera.GetViewMatrix();//Transforms the camera
	ourShader.setMat4("projection", projection);
	ourShader.setMat4("view", view);

	model = glm::mat4(1.0f);
	ourShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	gUVScale = glm::vec2(1.0f, 1.0f);
	ourShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture13);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture14);


	model = glm::translate(model, glm::vec3(0.0f, 3.0f, 5.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(6.0f, 0.80f, 11.5f));
	ourShader.setMat4("model", model);

	glDrawArrays(GL_TRIANGLES, 0, cTMesh.nIndices);//Draws the triangle


	glBindVertexArray(0);//Deactivate the Vertex Array Object

	//Top
	glBindVertexArray(tMesh.vao);
	//Lighting
	ourShader.use();
	ourShader.setVec3("viewPos", gCamera.Position);
	ourShader.setFloat("material.shininess", 256.0f);
	ourShader.setVec3("dirLight.direction", 2.0f, -2.0f, 0.03f);
	ourShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
	ourShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
	ourShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
	ourShader.setFloat("pointLights.constant", 1.0f);
	ourShader.setFloat("pointLights.linear", 0.09f);
	ourShader.setFloat("pointLights.quadratic", 0.032f);

	view = gCamera.GetViewMatrix();//Transforms the camera
	ourShader.setMat4("projection", projection);
	ourShader.setMat4("view", view);

	model = glm::mat4(1.0f);
	ourShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	gUVScale = glm::vec2(3.0f, 3.0f);
	ourShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture7);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture8);


	model = glm::translate(model, glm::vec3(0.0f, 4.29f, 5.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(6.0f, 1.76f, 11.5f));
	ourShader.setMat4("model", model);

	glDrawArrays(GL_TRIANGLES, 0, tMesh.nIndices);//Draws the triangle
	glBindVertexArray(0);//Deactivate the Vertex Array Object

	glBindVertexArray(cTMesh.vao);
	//Lighting
	ourShader.use();
	ourShader.setVec3("viewPos", gCamera.Position);
	ourShader.setFloat("material.shininess", 256.0f);
	ourShader.setVec3("dirLight.direction", 2.0f, -2.0f, 0.03f);
	ourShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
	ourShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
	ourShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
	ourShader.setFloat("pointLights.constant", 1.0f);
	ourShader.setFloat("pointLights.linear", 0.09f);
	ourShader.setFloat("pointLights.quadratic", 0.032f);

	view = gCamera.GetViewMatrix();//Transforms the camera
	ourShader.setMat4("projection", projection);
	ourShader.setMat4("view", view);

	model = glm::mat4(1.0f);
	ourShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	gUVScale = glm::vec2(3.0f, 3.0f);
	ourShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture3);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture4);

	model = glm::translate(model, glm::vec3(0.0f, 4.29f, 5.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(3.0f, 1.761f, 5.75f));
	ourShader.setMat4("model", model);

	glDrawArrays(GL_TRIANGLES, 0, cTMesh.nIndices);//Draws the triangle


	glBindVertexArray(0);//Deactivate the Vertex Array Object

	glBindVertexArray(tMesh.vao);
	//Lighting
	ourShader.use();
	ourShader.setVec3("viewPos", gCamera.Position);
	ourShader.setFloat("material.shininess", 256.0f);
	ourShader.setVec3("dirLight.direction", 2.0f, -2.0f, 0.03f);
	ourShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
	ourShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
	ourShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
	ourShader.setFloat("pointLights.constant", 1.0f);
	ourShader.setFloat("pointLights.linear", 0.09f);
	ourShader.setFloat("pointLights.quadratic", 0.032f);

	view = gCamera.GetViewMatrix();//Transforms the camera
	ourShader.setMat4("projection", projection);
	ourShader.setMat4("view", view);

	model = glm::mat4(1.0f);
	ourShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	gUVScale = glm::vec2(3.0f, 3.0f);
	ourShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture3);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture4);


	model = glm::translate(model, glm::vec3(0.0f, 4.29f, 5.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(6.0f, 1.76f, 11.5f));
	ourShader.setMat4("model", model);

	glDrawArrays(GL_LINES, 0, tMesh.nIndices);//Draws the triangle


	glBindVertexArray(0);//Deactivate the Vertex Array Object
	glBindVertexArray(0);//Deactivate the Vertex Array Object
	//Draw Wheel Front Well
	glBindVertexArray(fMesh.vao);
	ourShader.use();
	ourShader.setVec3("viewPos", gCamera.Position);
	ourShader.setFloat("material.shininess", 256.0f);
	ourShader.setVec3("dirLight.direction", 2.0f, -2.0f, 0.03f);
	ourShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
	ourShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
	ourShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
	ourShader.setFloat("pointLights.constant", 1.0f);
	ourShader.setFloat("pointLights.linear", 0.09f);
	ourShader.setFloat("pointLights.quadratic", 0.032f);

	view = gCamera.GetViewMatrix();//Transforms the camera
	ourShader.setMat4("projection", projection);
	ourShader.setMat4("view", view);

	model = glm::mat4(1.0f);
	ourShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	gUVScale = glm::vec2(1.0f, 1.0f);
	ourShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture3);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture4);


	model = glm::translate(model, glm::vec3(-3.0f, 1.0f, 9.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, aScale/3.0f);
	ourShader.setMat4("model", model);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, fMesh.sideVerts/2);//Draws the triangle

	glBindVertexArray(0);//Deactivate the Vertex Array Object

	glBindVertexArray(rMesh.vao);
	ourShader.use();
	ourShader.setVec3("viewPos", gCamera.Position);
	ourShader.setFloat("material.shininess", 256.0f);
	ourShader.setVec3("dirLight.direction", 2.0f, -2.0f, 0.03f);
	ourShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
	ourShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
	ourShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
	ourShader.setFloat("pointLights.constant", 1.0f);
	ourShader.setFloat("pointLights.linear", 0.09f);
	ourShader.setFloat("pointLights.quadratic", 0.032f);

	view = gCamera.GetViewMatrix();//Transforms the camera
	ourShader.setMat4("projection", projection);
	ourShader.setMat4("view", view);

	model = glm::mat4(1.0f);
	ourShader.setMat4("model", model);
	//Bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	gUVScale = glm::vec2(1.0f, 1.0f);
	ourShader.setVec2("uvScale", gUVScale);
	glBindTexture(GL_TEXTURE_2D, texture3);
	//bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture4);


	model = glm::translate(model, glm::vec3(-3.0f, 1.0f, 1.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, aScale / 3.0f);
	ourShader.setMat4("model", model);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, rMesh.sideVerts / 2);//Draws the triangle

	glBindVertexArray(0);//Deactivate the Vertex Array Object
	#pragma endregion

	#pragma region carsides
	glm::vec3 sideLocations[] = {
		glm::vec3( 2.5f,  1.5f, 2.6f),
		glm::vec3(-2.5f,  1.5f, 2.6f),
		glm::vec3( 0.0f,  3.4f, 10.6f),
		glm::vec3( 0.0f,  0.5f, -0.6f),
	};

	glm::vec3 sideScale[] = {
	glm::vec3( 0.25f,  0.1f, 1.0f),
	glm::vec3( 0.25f,  0.1f, 1.0f),
	glm::vec3( 0.6f,   0.5f, 0.6f),
	glm::vec3( 0.6f,   0.5f, 0.6f),
	};
	glm::vec3 angleDirection[] = {
	glm::vec3(0.0f,  0.0f, 1.0f),
	glm::vec3(0.0f,  0.0f, 1.0f),
	glm::vec3(1.0f,  0.0f, 0.0f),
	glm::vec3(1.0f,  0.0f, 0.0f),
	};
	GLfloat angles[] = {-90.0f, 90.0f, 90.0f, -90.0f};
	
	GLuint textureMaps[][2] = {
		{texture13,texture14},
		{texture13,texture14},
		{texture9,texture10},
		{texture11,texture12},
	};
	glm::vec2 scaleUV[] = {
	glm::vec2(1.0f, 6.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(0.3f, 1.0f),
	glm::vec2(0.3f, 1.0f),
	};

	for (int i = 0; i < 4; i++) {
		//Draw Left Side
		glBindVertexArray(sMesh.vao);
		ourShader.use();
		ourShader.setVec3("viewPos", gCamera.Position);
		ourShader.setFloat("material.shininess", 256.0f);
		ourShader.setVec3("dirLight.direction", 2.0f, -2.0f, 0.03f);
		ourShader.setVec3("pointLights.ambient", 0.25f, 0.25f, 0.25f);
		ourShader.setVec3("pointLights.diffuse", 0.4f, 0.4f, 0.4f);
		ourShader.setVec3("pointLights.specular", 0.774597f, 0.774597f, 0.774597f);
		ourShader.setFloat("pointLights.constant", 1.0f);
		ourShader.setFloat("pointLights.linear", 0.09f);
		ourShader.setFloat("pointLights.quadratic", 0.032f);

		view = gCamera.GetViewMatrix();//Transforms the camera
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

		model = glm::mat4(1.0f);
		ourShader.setMat4("model", model);
		//Bind diffuse map
		glActiveTexture(GL_TEXTURE0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		gUVScale = scaleUV[i];
		ourShader.setVec2("uvScale", gUVScale);
		glBindTexture(GL_TEXTURE_2D, textureMaps[i][0]);
		//bind specular map
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textureMaps[i][1]);


		model = glm::translate(model, sideLocations[i]);
		model = glm::rotate(model, glm::radians(angles[i]), angleDirection[i]);
		model = glm::scale(model, sideScale[i] / 3.0f);
		ourShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, sMesh.sideVerts / 2);//Draws the triangle
		glDrawArrays(GL_TRIANGLE_FAN, sMesh.sideVerts, sMesh.topVerts / 2);//Draws the triangle
		glDrawArrays(GL_TRIANGLE_FAN, sMesh.sideVerts + sMesh.topVerts, sMesh.bottomVerts / 2);//Draws the triangle

		glBindVertexArray(0);//Deactivate the Vertex Array Object
	}
	#pragma endregion

}

void UDestroyMesh(GLMesh& mesh) {

	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(2, mesh.vbos);
}

/*Texture Creation*/
bool CreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;


	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		//Set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//Set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << channels << " channels." << std::endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		//glBindTexture(GL_TEXTURE_2D, 0); //Unbind the texture

		return true;
	}

	//Error loading the image
	return false;
}


void DestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}
#pragma endregion
