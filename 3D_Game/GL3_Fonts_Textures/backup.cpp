#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <FTGL/ftgl.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

#include <string.h>
#include <math.h>
#include <sstream>

int level =1;
int view;

double mos_x, mos_y;

using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;
	GLuint TextureBuffer;
	GLuint TextureID;

	GLenum PrimitiveMode; // GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_LINE_STRIP_ADJACENCY, GL_LINES_ADJACENCY, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_TRIANGLE_STRIP_ADJACENCY and GL_TRIANGLES_ADJACENCY
	GLenum FillMode; // GL_FILL, GL_LINE
	int NumVertices;
};
typedef struct VAO VAO;

vector<VAO*>arr_block1;
vector<glm::vec3>block1;

vector<glm::vec3>block2;
vector<glm::vec3>block4;

vector<VAO*>arr_block3;
vector<glm::vec3>block3;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID; // For use with normal shader
	GLuint TexMatrixID; // For use with texture shader
} Matrices;

struct FTGLFont {
	FTFont* font;
	GLuint fontMatrixID;
	GLuint fontColorID;
} GL3Font;

GLuint programID, fontProgramID, textureProgramID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	cout << "Compiling shader : " <<  vertex_file_path << endl;
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	cout << VertexShaderErrorMessage.data() << endl;

	// Compile Fragment Shader
	cout << "Compiling shader : " << fragment_file_path << endl;
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	cout << FragmentShaderErrorMessage.data() << endl;

	// Link the program
	cout << "Linking program" << endl;
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	cout << ProgramErrorMessage.data() << endl;

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
	cout << "Error: " << description << endl;
}

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

glm::vec3 getRGBfromHue (int hue)
{
	float intp;
	float fracp = modff(hue/60.0, &intp);
	float x = 1.0 - abs((float)((int)intp%2)+fracp-1.0);

	if (hue < 60)
		return glm::vec3(1,x,0);
	else if (hue < 120)
		return glm::vec3(x,1,0);
	else if (hue < 180)
		return glm::vec3(0,1,x);
	else if (hue < 240)
		return glm::vec3(0,x,1);
	else if (hue < 300)
		return glm::vec3(x,0,1);
	else
		return glm::vec3(1,0,x);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  1,                  // attribute 1. Color
						  3,                  // size (r,g,b)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

struct VAO* create3DTexturedObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* texture_buffer_data, GLuint textureID, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;
	vao->TextureID = textureID;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->TextureBuffer));  // VBO - textures

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->TextureBuffer); // Bind the VBO textures
	glBufferData (GL_ARRAY_BUFFER, 2*numVertices*sizeof(GLfloat), texture_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  2,                  // attribute 2. Textures
						  2,                  // size (s,t)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

void draw3DTexturedObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Bind Textures using texture units
	glBindTexture(GL_TEXTURE_2D, vao->TextureID);

	// Enable Vertex Attribute 2 - Texture
	glEnableVertexAttribArray(2);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->TextureBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle

	// Unbind Textures to be safe
	glBindTexture(GL_TEXTURE_2D, 0);
}

/* Create an OpenGL Texture from an image */
GLuint createTexture (const char* filename)
{
	GLuint TextureID;
	// Generate Texture Buffer
	glGenTextures(1, &TextureID);
	// All upcoming GL_TEXTURE_2D operations now have effect on our texture buffer
	glBindTexture(GL_TEXTURE_2D, TextureID);
	// Set our texture parameters
	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering (interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load image and create OpenGL texture
	int twidth, theight;
	unsigned char* image = SOIL_load_image(filename, &twidth, &theight, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, twidth, theight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D); // Generate MipMaps to use
	SOIL_free_image_data(image); // Free the data read from file after creating opengl texture
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess it up

	return TextureID;
}


/**************************
 * Customizable functions *
 **************************/

float x_b,y_b,prev_y,prev_x,z_b;

int x_key=0;
int y_key=0;
int t_count;
int jump =0;
float v;




float triangle_rot_dir = 1;
float rectangle_rot_dir = -1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Function is called first on GLFW_PRESS.

	if (action == GLFW_RELEASE) {
		switch (key) {
			case GLFW_KEY_C:
				rectangle_rot_status = !rectangle_rot_status;
				break;
			case GLFW_KEY_P:
				triangle_rot_status = !triangle_rot_status;
				break;
			case GLFW_KEY_X:
				// do something ..
				break;
			case GLFW_KEY_UP:
				x_key = 0;
				break;
			case GLFW_KEY_DOWN:
				x_key = 0;
				break;
			case GLFW_KEY_LEFT:
				y_key = 0;
				break;
			case GLFW_KEY_RIGHT:
				y_key = 0;
				break;
			default:
				break;
		}
	}
	else if (action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_ESCAPE:
				quit(window);
			case GLFW_KEY_UP:
				x_key = -1;
				break;
			case GLFW_KEY_DOWN:
				x_key = 1;
				break;
			case GLFW_KEY_LEFT:
				y_key = -1;
				break;
			case GLFW_KEY_RIGHT:
				y_key = 1;
				break;
			case GLFW_KEY_T:
				t_count++;
				if(t_count == 1){
					view = 1;
				}
				if(t_count == 2 & view == 1 ){
					view = 3;
					t_count = 0;
				}
				break;
			case GLFW_KEY_H:
				view = 2;
				break;
			case GLFW_KEY_SPACE:
				jump = 1;
				v = 7.5;
				break;
			default:
				break;
		}
	}
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
			quit(window);
			break;
		default:
			break;
	}
}

bool rmos = false;
bool test = false;

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
			if (action == GLFW_RELEASE)
				triangle_rot_dir *= -1;
			break;
		case GLFW_MOUSE_BUTTON_RIGHT:
		 	if (action == GLFW_RELEASE) {
                rmos = false;
            }
            else if(action == GLFW_PRESS) 
            {
            	rmos = true;
            	test = true;
            }
            break;
			
		default:
			break;
	}
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
	int fbwidth=width, fbheight=height;
	/* With Retina display on Mac OS X, GLFW's FramebufferSize
	 is different from WindowSize */
	glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 45.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	 glLoadIdentity ();
	 gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
	// Perspective projection for 3D views
	Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 1.0f, 500.0f);

	// Ortho projection for 2D views
	//Matrices.projection = glm::ortho(-150.0f, 150.0f, -150.0f, 150.0f, -500.0f, 500.0f);
}

VAO *triangle, *rectangle , *cube , *box , *sphere, *plate, *plate_holes , *pyramid;


void createSphere ()
{
	/* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

	/* Define vertex array as used in glBegin (GL_TRIANGLES) */
	static const GLfloat vertex_buffer_data [] = {
		0.000000, -1.000000, 0.000000,
		0.723607, -0.447220, 0.525725,
		-0.276388, -0.447220, 0.850649,
        -0.894426, -0.447216, 0.000000,
        -0.276388, -0.447220, -0.850649,
 		0.723607 ,-0.447220 ,-0.525725,
 		0.276388 ,0.447220 ,0.850649,
	    -0.723607 ,0.447220 ,0.525725,
        -0.723607 ,0.447220 ,-0.525725,
		 0.276388 ,0.447220 ,-0.850649,
		 0.894426 ,0.447216 ,0.000000,
		 0.000000 ,1.000000 ,0.000000,
		 -0.162456 ,-0.850654 ,0.499995,
		 0.425323 ,-0.850654 ,0.309011,
		 0.262869 ,-0.525738 ,0.809012,
		 0.850648 ,-0.525736 ,0.000000,
		 0.425323 ,-0.850654 ,-0.309011,
		 -0.525730 ,-0.850652 ,0.000000,
		 -0.688189 ,-0.525736 ,0.499997,
		 -0.162456 ,-0.850654 ,-0.499995,
		 -0.688189 ,-0.525736 ,-0.499997,
		 0.262869 ,-0.525738 ,-0.809012,
		 0.951058 ,0.000000 ,0.309013,
		 0.951058 ,0.000000 ,-0.309013,
		 0.000000 ,0.000000 ,1.000000,
		 0.587786 ,0.000000 ,0.809017,
		 -0.951058 ,0.000000 ,0.309013,
		 -0.587786 ,0.000000, 0.809017,
		 -0.587786 ,0.000000, -0.809017,
		 -0.951058 ,0.000000, -0.309013,
		 0.587786 ,0.000000 ,-0.809017,
		 0.000000 ,0.000000 ,-1.000000,
		 0.688189 ,0.525736 ,0.499997,
		 -0.262869 ,0.525738 ,0.809012,
		 -0.850648 ,0.525736, 0.000000,
		 -0.262869 ,0.525738, -0.809012,
		 0.688189 ,0.525736, -0.499997,
		 0.162456 ,0.850654 ,0.499995,
		 0.525730 ,0.850652, 0.000000,
		 -0.425323 ,0.850654 ,0.309011,
		 -0.425323 ,0.850654, -0.309011,
		 0.162456 ,0.850654, -0.499995
	};

	static const GLfloat color_buffer_data [] = {
		1,1,1, // color 0
		1,1,1, // color 1
		1,1,1, // color 2
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		1,1,1,
		

	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	sphere = create3DObject(GL_TRIANGLES, 42, vertex_buffer_data, color_buffer_data, GL_FILL);
}

VAO* createCube (float l, float w, float h, float color[2][3])
{
	float x=l/2, y=w/2, z=h;
    // GL3 accepts only Triangles. Quads are not supported
    GLfloat vertex_buffer_data [] = {
        -x,-y,0, // triangle 1 : begin
        -x,-y, z,
        -x, y, z,
        -x,-y,0,//
        -x, y, z,
        -x, y,0,
        
        x, y, z,//
        x,-y,0,
        x, y,0,
        x,-y,0,//
        x, y, z,
        x,-y, z,
        
        x, y,0,//
        x,-y,0,
        -x,-y,0,
        x, y,0, // triangle 2 : begin
        -x,-y,0,
        -x, y,0,
        
        -x, y, z,//
        -x,-y, z,
        x,-y, z,
        x, y, z,//
        -x, y, z,
        x,-y, z,
 
        x,-y, z,//
        -x,-y,0,
        x,-y,0,
        x,-y, z,//
        -x,-y, z,
        -x,-y,0,
       
        x, y, z,//
        x, y,0,
        -x, y,0,
        x, y, z,//
        -x, y,0,
        -x, y, z,
    };
    
    GLfloat color_buffer_data [] = {
        
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 1
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 2
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 3

    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 3
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 4
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 1
        
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 1
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 2
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 3

    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 3
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 4
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 1
        
    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 1
    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 2
    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 3

    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 3
    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 4
    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 1
        
    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 1
    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 2
    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 3

    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 3
    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 4
    color[1][0]/255.0f,color[1][1]/255.0f,color[1][2]/255.0f, // color 1
        
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 1
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 2
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 3

    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 3
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 4
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 1

    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 1
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 2
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 3

    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 3
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 4
    color[0][0]/255.0f,color[0][1]/255.0f,color[0][2]/255.0f, // color 1

    };
    
    // create3DObject creates and returns a handle to a VAO that can be used later
    return create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}



// Creates the triangle object used in this sample code
void createTriangle ()
{
	/* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

	/* Define vertex array as used in glBegin (GL_TRIANGLES) */
	static const GLfloat vertex_buffer_data [] = {
		0, 1,0, // vertex 0
		-1,-1,0, // vertex 1
		1,-1,0, // vertex 2
	};

	static const GLfloat color_buffer_data [] = {
		1,0,0, // color 0
		0,1,0, // color 1
		0,0,1, // color 2
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}

// Creates the rectangle object used in this sample code
void createRectangle (GLuint textureID)
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		-1.2,-1,0, // vertex 1
		1.2,-1,0, // vertex 2
		1.2, 1,0, // vertex 3

		1.2, 1,0, // vertex 3
		-1.2, 1,0, // vertex 4
		-1.2,-1,0  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		1,0,0, // color 1
		0,0,1, // color 2
		0,1,0, // color 3

		0,1,0, // color 3
		0.3,0.3,0.3, // color 4
		1,0,0  // color 1
	};

	// Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
	static const GLfloat texture_buffer_data [] = {
		0,1, // TexCoord 1 - bot left
		1,1, // TexCoord 2 - bot right
		1,0, // TexCoord 3 - top right

		1,0, // TexCoord 3 - top right
		0,0, // TexCoord 4 - top left
		0,1  // TexCoord 1 - bot left
	};

	// create3DTexturedObject creates and returns a handle to a VAO that can be used later
	rectangle = create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
}

void createplate ()
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		-5,-5,0, // vertex 1
		-5,5,0, // vertex 2
		5, -5,0, // vertex 3

		5, 5,0, // vertex 3
		5, -5,0, // vertex 4
		-5,5,0  // vertex 1



	};

	static const GLfloat color_buffer_data [] = {
		1,0,0, // color 1
		1,0,0, // color 2
		1,0,0, // color 3

		1,0,0, // color 3
		1,0,0, // color 4
		1,0,0  // color 1
	};


	// create3DTexturedObject creates and returns a handle to a VAO that can be used later
	plate = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createplate_holes ()
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		-1.5,-1.5,0, // vertex 1
		-1.5,1.5,0, // vertex 2
		1.5, -1.5,0, // vertex 3

		1.5, 1.5,0, // vertex 3
		1.5, -1.5,0, // vertex 4
		-1.5,1.5,0  // vertex 1



	};

	static const GLfloat color_buffer_data [] = {
		0,0,0.05, // color 1
		0,0,0.05,
		0,0,0.05,

		0,0,0.05,
		0,0,0.05,
		0,0,0.05
		
	};



	plate_holes = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);

	// create3DTexturedObject creates and returns a handle to a VAO that can be used later
}

void createpyramid(){

	static const GLfloat vertex_buffer_data [] = {

		-1.5,1.5,0,
		0,0,10,
		-1.5,-1.5,0,


		-1.5,1.5,0,
		0,0,10,
		1.5,1.5,0,

		1.5,1.5,0,
		0,0,10,
		1.5,-1.5,0,


		-1.5,-1.5,0,
		0,0,10,
		1.5,-1.5,0,
	};

	static const GLfloat color_buffer_data [] = {
		1,1,1,
		1,1,1,
		1,1,1,

		1,1,1,
		1,1,1,
		1,1,1,

		1,1,1,
		1,1,1,
		1,1,1,

		1,1,1,
		1,1,1,
		1,1,1


	};

	pyramid = create3DObject(GL_TRIANGLES, 12, vertex_buffer_data, color_buffer_data, GL_FILL);


}


float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

float angle,v_j;

int p;
/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
	// clear the color and depth in the frame buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);
	// Eye - Location of camera. Don't change unless you are sure!!

	static float prev_x;
	
		if(test)
		{
			prev_x = mos_x; 
			test = false;
		}
		if(rmos)
		{
			angle+= (mos_x - prev_x)*(M_PI)/300;
			prev_x = mos_x;
		}

	if(view!=1){
		t_count = 0;
	}

	switch (view) {
			case 1:
				Matrices.view = glm::lookAt(glm::vec3(0.0000001,0,300), glm::vec3(0,0,0), glm::vec3(0,0,1));
				break;
			case 2:
				Matrices.view = glm::lookAt(glm::vec3(180*1.414*cos(angle),-180*1.414*sin(angle),200), glm::vec3(0,0,0), glm::vec3(0,0,1)); 
				break;
			case 3:
				Matrices.view = glm::lookAt(glm::vec3(200,0,200), glm::vec3(0,0,0), glm::vec3(0,0,1));
				break;
			case GLFW_KEY_LEFT:
				y_key = -1;
				break;
			case GLFW_KEY_RIGHT:
				y_key = 1;
				break;
			}

	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model

	// Load identity to model matrix
	for( int i=0;i < arr_block1.size();i++){
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (block1[i]);
		Matrices.model *=  (translateRectangle );
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(arr_block1[i]);
	}	

	for( int i=0;i < arr_block3.size();i++){
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (block3[i]);
		Matrices.model *=  (translateRectangle );
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(arr_block3[i]);
	}
	for( int i=0;i < block4.size();i++){
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle2 = glm::translate (block4[i]);
		Matrices.model *=  (translateRectangle2 );
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(plate);

		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateplate1 = glm::translate(glm::vec3(2,2,0.1));
		Matrices.model *=  (translateRectangle2 * translateplate1);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(plate_holes);

		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateplate2 = glm::translate(glm::vec3(2,-2,0.1));
		Matrices.model *=  (translateRectangle2 * translateplate2);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(plate_holes);

		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateplate3 = glm::translate(glm::vec3(-2,2,0.1));
		Matrices.model *=  (translateRectangle2 * translateplate3);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(plate_holes);

		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateplate4 = glm::translate(glm::vec3(-2,-2,0.1));
		Matrices.model *=  (translateRectangle2 * translateplate4);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(plate_holes);



		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translatepl1 = glm::translate(glm::vec3(2,2,0.1));
		Matrices.model *=  (translateRectangle2 * translatepl1);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(pyramid);

		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translatepl2 = glm::translate(glm::vec3(2,-2,0.1));
		Matrices.model *=  (translateRectangle2 * translatepl2);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(pyramid);

		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translatepl3 = glm::translate(glm::vec3(-2,2,0.1));
		Matrices.model *=  (translateRectangle2 * translateplate3);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(pyramid);

		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translatepl4 = glm::translate(glm::vec3(-2,-2,0.1));
		Matrices.model *=  (translateRectangle2 * translatepl4);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(pyramid);


	}

	
	


	if(p!=2){
		x_b =  x_b + 5*x_key;
		y_b =  y_b + 5*y_key;

	}


	for ( int i=0;i< block3.size();i++){

		if(abs(x_b-block3[i][0])<10 && abs(y_b-block3[i][1])<10 && p!=2 )
		{
			
			p = 1;
			
		}
	}

	for ( int i=0;i< block2.size();i++){

		if(abs(x_b-block2[i][0])<10 && abs(y_b-block2[i][1])<10 && z_b <=20 )
		{
			
			p = 2;
			x_b = block2[i][0];
			y_b = block2[i][1];
			
		}
	}

	if(p==1){

		x_b =  x_b - 5*x_key;
		y_b =  y_b - 5*y_key;
		z_b = 20;
		p = 0;
	}
	if(p==2){

		z_b += -2;

	}

	if(z_b < -500){

		x_b = 80;
		y_b = -80;
		z_b = 20;
		p = 0;
	}
	if (jump == 1 & p!= 2){

		v -= 1.5; 
		z_b += v;
		if(z_b <= 20){
			z_b = 20;
			jump = 0;
		}


	}

		
	/*Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateRectangle = glm::translate (glm::vec3(x_b,y_b,z_b));
	Matrices.model *=  (translateRectangle );
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(box);*/

	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateRectangle = glm::translate (glm::vec3(x_b,y_b,z_b));
	Matrices.model *=  (translateRectangle );
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(box);

	/* Render your scene */
/*	glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
	glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
	glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
	Matrices.model *= triangleTransform;
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	// Copy MVP to normal shaders
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(triangle);



	// Render with texture shaders now
	glUseProgram(textureProgramID);

	// Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
	// glPopMatrix ();
	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateRectangle = glm::translate (glm::vec3(2, 0, 0));        // glTranslatef
	glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translateRectangle * rotateRectangle);
	MVP = VP * Matrices.model;

	// Copy MVP to texture shaders
	glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);

	// Set the texture sampler to access Texture0 memory
	glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DTexturedObject(rectangle);

	// Increment angles
	float increments = 1;

	// Render font on screen
	static int fontScale = 0;
	float fontScaleValue = 0.75 + 0.25*sinf(fontScale*M_PI/180.0f);
	glm::vec3 fontColor = getRGBfromHue (fontScale);



	// Use font Shaders for next part of code
	glUseProgram(fontProgramID);
	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Transform the text
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateText = glm::translate(glm::vec3(-3,2,0));
	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText * scaleText);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

	// Render font
	GL3Font.font->Render("Round n Round we go !!");


	//camera_rotation_angle++; // Simulating camera rotation
	triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
	rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;

	// font size and color changes
	fontScale = (fontScale + 1) % 360;*/

}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
	GLFWwindow* window; // window desciptor/handle

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval( 1 );

	/* --- register callbacks with GLFW --- */

	/* Register function to handle window resizes */
	/* With Retina display on Mac OS X GLFW's FramebufferSize
	 is different from WindowSize */
	glfwSetFramebufferSizeCallback(window, reshapeWindow);
	glfwSetWindowSizeCallback(window, reshapeWindow);

	/* Register function to handle window close */
	glfwSetWindowCloseCallback(window, quit);

	/* Register function to handle keyboard input */
	glfwSetKeyCallback(window, keyboard);      // general keyboard input
	glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

	/* Register function to handle mouse click */
	glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

	return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
	// Load Textures
	// Enable Texture0 as current texture memory
	glActiveTexture(GL_TEXTURE0);
	// load an image file directly as a new OpenGL texture
	// GLuint texID = SOIL_load_OGL_texture ("beach.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS); // Buggy for OpenGL3
	GLuint textureID = createTexture("beach2.png");
	// check for an error during the load process
	if(textureID == 0 )
		cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;

	// Create and compile our GLSL program from the texture shaders
	textureProgramID = LoadShaders( "TextureRender.vert", "TextureRender.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");


	/* Objects should be created before any other gl function and shaders */
	// Create the models
	createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	createRectangle (textureID);
	createSphere();
	createplate();
	createplate_holes();
	createpyramid();



	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL3.vert", "Sample_GL3.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (window, width, height);

	// Background color of the scene
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Initialise FTGL stuff
	const char* fontfile = "arial.ttf";
	GL3Font.font = new FTExtrudeFont(fontfile); // 3D extrude style rendering

	if(GL3Font.font->Error())
	{
		cout << "Error: Could not load font `" << fontfile << "'" << endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Create and compile our GLSL program from the font shaders
	fontProgramID = LoadShaders( "fontrender.vert", "fontrender.frag" );
	GLint fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform;
	fontVertexCoordAttrib = glGetAttribLocation(fontProgramID, "vertexPosition");
	fontVertexNormalAttrib = glGetAttribLocation(fontProgramID, "vertexNormal");
	fontVertexOffsetUniform = glGetUniformLocation(fontProgramID, "pen");
	GL3Font.fontMatrixID = glGetUniformLocation(fontProgramID, "MVP");
	GL3Font.fontColorID = glGetUniformLocation(fontProgramID, "fontColor");

	GL3Font.font->ShaderLocations(fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform);
	GL3Font.font->FaceSize(1);
	GL3Font.font->Depth(0);
	GL3Font.font->Outset(0, 0);
	GL3Font.font->CharMap(ft_encoding_unicode);

	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

}
void  platform(){

	string line;
	float y=20,x;
	ifstream file;
	stringstream val;
	string add;
	add = ".txt";
	val << level;

	angle = (M_PI)/4;
	view = 2;

	string final =  val.str()+add;
	file.open(final.c_str());

	arr_block1.clear();
	arr_block3.clear();

	block1.clear();
	block2.clear();
	block3.clear();
	block4.clear();

	float cl[2][3];
	cl[0][0]=33;
	cl[0][1]=102;
	cl[0][2]=0;
	cl[1][0]=101;
	cl[1][1]=255;
	cl[1][2]=26;

	float cl2[2][3];
	cl2[0][0]=33;
	cl2[0][1]=102;
	cl2[0][2]=100;
	cl2[1][0]=33;
	cl2[1][1]=102;
	cl2[1][2]=0;


	box = createCube(10,10,10,cl2);

	x_b = 80;
	y_b = -80;
	z_b = 20;

	while(getline(file, line) && y >= 0)
		{
			//cout << "oh" << level << line << endl;
			int x = 0;
			while(x < line.length() && x < 20)
			{
				switch(line[x])
				{
					case 'x':
						arr_block1.push_back(createCube(10,10,20,cl));
						block1.push_back(glm::vec3(float(x*10)-100, y*10-100, 0));
						break;
					case 'o':			
						block2.push_back(glm::vec3(float(x*10)-100, y*10-100, 0));
						break;
					case 'e':
						arr_block3.push_back(createCube(10,10,30,cl));
						block3.push_back(glm::vec3(float(x*10)-100, y*10-100, 0));
						break;
					case 'p':			
						block4.push_back(glm::vec3(float(x*10)-100, y*10-100, 20));
						break;
					default:
						break;
				}

				x++;
			}
			y--;
		}
	file.close();





 
	

	cube = createCube(10,10,20,cl);


}

int main (int argc, char** argv)
{
	int width = 600;
	int height = 600;

	GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

	double last_update_time = glfwGetTime(), current_time;
	platform();


	/* Draw in loop */
	while (!glfwWindowShouldClose(window)) {

		// OpenGL Draw commands

		draw();



		// Swap Frame Buffer in double buffering
		glfwSwapBuffers(window);

		// Poll for Keyboard and mouse events
		glfwPollEvents();

		glfwGetCursorPos(window, &mos_x, &mos_y);
  	
  	    //glfwSetScrollCallback(window, scroll_callback);

		// Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
		current_time = glfwGetTime(); // Time in seconds
		if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
			// do something every 0.5 seconds ..
			last_update_time = current_time;
		}
	}

	glfwTerminate();
	exit(EXIT_SUCCESS);
}

