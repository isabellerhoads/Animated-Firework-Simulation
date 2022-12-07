#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Particle.h"
#include "Program.h"
#include "Texture.h"
#include "Shape.h"
//#include "WorldShape.h"

using namespace std;
using namespace Eigen;

// Stores information in data/input.txt
class DataInput
{
public:
	vector<string> textureData;
	vector< vector<string> > meshData;
	string skeletonData;
};

DataInput dataInput;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the shaders are loaded from
string DATA_DIR = ""; // where the data are loaded from

shared_ptr<Camera> camera;
//shared_ptr<WorldShape> plane;
shared_ptr<Program> prog, prog2;
shared_ptr<Texture> texture0;
vector< shared_ptr< Particle> > particles;
vector< shared_ptr<Shape> > shapes;
vector<Matrix4f> bindPoses;
vector< vector<Matrix4f> > transformations;
int frameCount;

Eigen::Vector3f grav;
float t, h;

bool keyToggles[256] = {false}; // only for English keyboards!

// This function is called when a GLFW error occurs
static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
		bool alt   = (mods & GLFW_MOD_ALT) != 0;
		camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
	}
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved((float)xmouse, (float)ymouse);
	}
}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// This function is called once to initialize the scene and OpenGL
static void init()
{
	// Create shapes
	for (const auto& mesh : dataInput.meshData) {

			auto shape = make_shared<Shape>();
			shapes.push_back(shape);
			shape->loadMesh(DATA_DIR + mesh[0]);
			shape->parseWeightData(DATA_DIR + mesh[1]);
			shape->loadBindPoses(bindPoses);
			shape->loadTransformations(transformations);
		
	}

	// Initialize time.
	glfwSetTime(0.0);
	
	// Set background color.
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);
	// Enable setting gl_PointSize from vertex shader
	glEnable(GL_PROGRAM_POINT_SIZE);
	// Enable quad creation from sprite
	glEnable(GL_POINT_SPRITE);

	prog = make_shared<Program>();
	prog->setShaderNames(RESOURCE_DIR + "vert.glsl", RESOURCE_DIR + "frag.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addAttribute("aPos");
	prog->addAttribute("aAlp");
	prog->addAttribute("aCol");
	prog->addAttribute("aSca");
	prog->addUniform("P");
	prog->addUniform("MV");
	prog->addUniform("screenSize");
	prog->addUniform("texture0");

	//prog2 = make_shared<Program>();
	//prog2->setShaderNames(RESOURCE_DIR + "BP_vert.glsl", RESOURCE_DIR + "BP_frag.glsl");
	//prog2->init();
	//prog2->addAttribute("aPos");
	//prog2->addAttribute("aNor");
	//prog2->addUniform("MV");
	//prog2->addUniform("P");
	//prog2->addUniform("MV_it");
	//prog2->addUniform("kd");
	
	camera = make_shared<Camera>();
	camera->setInitDistance(10.0f);
	
	texture0 = make_shared<Texture>();
	texture0->setFilename(RESOURCE_DIR + "sphere.jpg");
	texture0->init();
	texture0->setUnit(0);
	texture0->setWrapModes(GL_REPEAT, GL_REPEAT);
	
	for (int j = 0; j < shapes.size(); j++)
	{
		auto shape = shapes[j];
		int n = shape->getNumVerts();
		int vertIndex = 0;
		Particle::init(n);
		for (int i = 0; i < n; i++) {
			auto p = make_shared<Particle>(i);
			p->setShapeindex(j);
			particles.push_back(p);
  			Vector3f pos = shapes[0]->update(0, true, vertIndex, i) / 100;
			p->rebirth(0.0f, keyToggles, pos, Vector3f(0.0f, 1.0f, 0.0f));
			vertIndex += 3;
		}
	}

	grav << 0.0f, -9.8f, 0.0f;
	t = 0.0f;
	h = 0.01f;
	
	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'l']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	MV->pushMatrix();

	camera->applyViewMatrix(MV);
	camera->applyProjectionMatrix(P);

	//prog2->bind();
	//MV->pushMatrix();
	//MV->scale(100.0f, 1.0f, 100.0f);
	//MV->rotate(M_PI / 2, 1.0f, 0.0f, 0.0f);
	//glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
	//glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	//glUniformMatrix4fv(prog2->getUniform("MV_it"), 1, GL_FALSE, glm::value_ptr(inverse(transpose((MV->topMatrix())))));
	//glUniform3f(prog2->getUniform("kd"), 0.0f, 1.0f, 0.0f);
	//plane->draw(prog2);
	//prog2->unbind();
	//MV->popMatrix();
	
	// Draw particles
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	prog->bind();
	texture0->bind(prog->getUniform("texture0"));
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniform2f(prog->getUniform("screenSize"), (float)width, (float)height);
	Particle::draw(particles, prog);
	texture0->unbind();
	prog->unbind();
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	
	MV->popMatrix();
	P->popMatrix();
	
	GLSL::checkError(GET_FILE_LINE);
}

bool stepParticles(int frame)
{
	if(keyToggles[(unsigned)' ']) {
		// This can be parallelized!
		bool explodes = false;
		int index = 0;
		double fps = 30;

		for(int i = 0; i < (int)particles.size(); ++i) 
		{
			Vector3f pos = shapes[0]->update(frame, true, index, i) / 75;
			explodes = particles[i]->step(t, h, grav, keyToggles, pos);
			index += 3;
		}
		t += h;
		return explodes;
	}
	return false;
}

void loadDataInputFile()
{
	string filename = DATA_DIR + "input.txt";
	ifstream in;
	in.open(filename);
	if (!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}
	cout << "Loading " << filename << endl;

	string line;
	while (1) {
		getline(in, line);
		if (in.eof()) {
			break;
		}
		if (line.empty()) {
			continue;
		}
		// Skip comments
		if (line.at(0) == '#') {
			continue;
		}
		// Parse lines
		string key, value;
		stringstream ss(line);
		// key
		ss >> key;
		if (key.compare("TEXTURE") == 0) {
			ss >> value;
			dataInput.textureData.push_back(value);
		}
		else if (key.compare("MESH") == 0) {
			vector<string> mesh;
			ss >> value;
			mesh.push_back(value); // obj
			ss >> value;
			mesh.push_back(value); // skin
			ss >> value;
			mesh.push_back(value); // texture
			dataInput.meshData.push_back(mesh);
		}
		else if (key.compare("SKELETON") == 0) {
			ss >> value;
			dataInput.skeletonData = value;
		}
		else {
			cout << "Unknown key word: " << key << endl;
		}
	}
	in.close();
}

void parseSkeletonData()
{
	string filename = DATA_DIR + dataInput.skeletonData;
	ifstream in;
	in.open(filename);
	if (!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}
	cout << "Loading " << filename << endl;

	string line;
	int lineIndex = 0, boneCount = 0;
	while (1)
	{
		getline(in, line);
		if (in.eof()) {
			break;
		}
		if (line.empty()) {
			continue;
		}
		// Skip comments
		if (line.at(0) == '#') {
			continue;
		}

		stringstream ss(line);
		// Parse lines
		if (lineIndex == 0)
		{
			ss >> frameCount;
			ss >> boneCount;
			lineIndex++;
			continue;
		}

		int i = 0, totalFloats = boneCount * 7;
		vector<Matrix4f> frameTransform;
		bool isAnimation = false;
		while (i < totalFloats)
		{
			float x, y, z, w;

			// store quaternion
			ss >> x;
			ss >> y;
			ss >> z;
			ss >> w;

			Quaternionf q(w, x, y, z);

			// store position
			ss >> x;
			ss >> y;
			ss >> z;

			Vector4f p(x, y, z, 1.0f);

			q.normalize();
			Matrix3f R(q);
			Matrix4f M;
			M.setIdentity();
			M.block<3, 3>(0, 0) = R;
			M.col(3) = p;

			if (lineIndex == 1)
			{
				bindPoses.push_back(M.inverse());
			}
			else
			{
				frameTransform.push_back(M);
				isAnimation = true;
			}
			i += 7;
		}
		if (isAnimation)
		{
			transformations.push_back(frameTransform);
			frameTransform.clear();
		}
		lineIndex++;
	}
	in.close();
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		cout << "Usage: A2 <SHADER DIR> <DATA DIR>" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	DATA_DIR = argv[2] + string("/");
	loadDataInputFile();
 	parseSkeletonData();

	double t1 = glfwGetTime();

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Set the window resize call back.
	glfwSetFramebufferSizeCallback(window, resize_callback);
	// Initialize scene.
	init();

	bool explodes = false;
	int frame = 0;
	float tFrame = 0.0f;
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Step particles.
		explodes = stepParticles(frame);
		
		if (explodes)
		{
			frame = ((int)floor(30 * tFrame)) % frameCount;
			tFrame += h;
		}
		else
		{
			frame = 0;
			tFrame = 0.0f;
		}

		if(!glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
			// Render scene.
			render();
			// Swap front and back buffers.
			glfwSwapBuffers(window);
		}
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
