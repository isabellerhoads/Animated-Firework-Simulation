#include "Particle.h"

#include <iostream>

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Texture.h"
#include "Shape.h"

using namespace std;
using namespace Eigen;

vector<float> Particle::posBuf;
vector<float> Particle::colBuf;
vector<float> Particle::alpBuf;
vector<float> Particle::scaBuf;
GLuint Particle::posBufID;
GLuint Particle::colBufID;
GLuint Particle::alpBufID;
GLuint Particle::scaBufID;

// Before this constructor is called, posBuf must be a valid vector<float>.
// I.e., Particle::init(n) must be called first.
Particle::Particle(int index) :
	color(&colBuf[3*index]),
	scale(scaBuf[index]),
	x(&posBuf[3*index]),
	alpha(alpBuf[index])
{
	// Random fixed properties
	color << randFloat(0.5f, 1.0f), randFloat(0.5f, 1.0f), randFloat(0.5f, 1.0f);
	scale = 0.025f;
	
	// Send color data to GPU
	glBindBuffer(GL_ARRAY_BUFFER, colBufID);
	glBufferSubData(GL_ARRAY_BUFFER, 3*index*sizeof(float), 3*sizeof(float), color.data());
	
	// Send scale data to GPU
	glBindBuffer(GL_ARRAY_BUFFER, scaBufID);
	glBufferSubData(GL_ARRAY_BUFFER, index*sizeof(float), sizeof(float), &scale);
}

Particle::~Particle()
{
}

void Particle::rebirth(float t, const bool *keyToggles, Vector3f p0, Vector3f v0)
{
	m = 1.0f;
	alpha = 1.0f;
	
	//
	// <IMPLEMENT ME>
	// Replace these initial conditions
	//

	Vector3f start = 0.001 * p0;
	//Vector3f v0 = p0.normalized();

	d = randFloat(0.0f, 3.0f);
	// x << p0(0) * 0.5, p0(1) * 0.5, p0(2) * 0.5;
	x << start(0), start(1), start(2);
	v << 0.0f, 1.0f, 0.0f;
	lifespan = 2.4f;
	tExplode = lifespan;

	//
	// </IMPLEMENT ME>
	//

	tEnd = t + lifespan;
}

void Particle::explode(float tExplode, float h, const Vector3f& g, Vector3f pos)
{
	float scale = 10000 * (lifespan - tExplode);

	//cout << "v: " << v << endl;
	//cout << "x: " << x << endl;
	Vector3f f = scale * (pos - v);
	v += (h / m) * f;

	x += h * v;

	v = pos;
}

bool Particle::step(float t, float h, const Vector3f &g, const bool *keyToggles, Vector3f pos)
{
	if(t > tEnd) {
		rebirth(t, keyToggles, pos, Vector3f(0.0f, 1.0f, 0.0f));
	}
	// Update alpha based on current time
	alpha = (tEnd-t)/lifespan;
	float tStep = tEnd - t;

	if (tStep == 1.14)
	{
		v = pos;
	}

	if (tStep < 1.14)
	{
		explode(tExplode, h, g, pos);
		tExplode -= h;
		return true;
	}
	else
	{
		x += h * v;
		return false;
	}
}

float Particle::randFloat(float l, float h)
{
	float r = rand() / (float)RAND_MAX;
	return (1.0f - r) * l + r * h;
}

void Particle::init(int n)
{
	posBuf.resize(3*n);
	colBuf.resize(3*n);
	alpBuf.resize(n);
	scaBuf.resize(n);
	
	for(int i = 0; i < n; ++i) {
		posBuf[3*i+0] = 0.0f;
		posBuf[3*i+1] = 0.0f;
		posBuf[3*i+2] = 0.0f;
		colBuf[3*i+0] = 1.0f;
		colBuf[3*i+1] = 1.0f;
		colBuf[3*i+2] = 1.0f;
		alpBuf[i] = 1.0f;
		scaBuf[i] = 1.0f;
	}

	// Generate buffer IDs
	GLuint bufs[4];
	glGenBuffers(4, bufs);
	posBufID = bufs[0];
	colBufID = bufs[1];
	alpBufID = bufs[2];
	scaBufID = bufs[3];
	
	// Send color buffer to GPU
	glBindBuffer(GL_ARRAY_BUFFER, colBufID);
	glBufferData(GL_ARRAY_BUFFER, colBuf.size()*sizeof(float), &colBuf[0], GL_STATIC_DRAW);
	
	// Send scale buffer to GPU
	glBindBuffer(GL_ARRAY_BUFFER, scaBufID);
	glBufferData(GL_ARRAY_BUFFER, scaBuf.size()*sizeof(float), &scaBuf[0], GL_STATIC_DRAW);
	
	assert(glGetError() == GL_NO_ERROR);
}

void Particle::draw(const vector< shared_ptr<Particle> > &particles,
					shared_ptr<Program> prog)
{
	// Enable, bind, and send position array
	glEnableVertexAttribArray(prog->getAttribute("aPos"));
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_DYNAMIC_DRAW);
	glVertexAttribPointer(prog->getAttribute("aPos"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	// Enable, bind, and send alpha array
	glEnableVertexAttribArray(prog->getAttribute("aAlp"));
	glBindBuffer(GL_ARRAY_BUFFER, alpBufID);
	glBufferData(GL_ARRAY_BUFFER, alpBuf.size()*sizeof(float), &alpBuf[0], GL_DYNAMIC_DRAW);
	glVertexAttribPointer(prog->getAttribute("aAlp"), 1, GL_FLOAT, GL_FALSE, 0, 0);
	
	// Enable and bind color array
	glEnableVertexAttribArray(prog->getAttribute("aCol"));
	glBindBuffer(GL_ARRAY_BUFFER, colBufID);
	glVertexAttribPointer(prog->getAttribute("aCol"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	// Enable and bind scale array
	glEnableVertexAttribArray(prog->getAttribute("aSca"));
	glBindBuffer(GL_ARRAY_BUFFER, scaBufID);
	glVertexAttribPointer(prog->getAttribute("aSca"), 1, GL_FLOAT, GL_FALSE, 0, 0);
	
	// Draw
	glDrawArrays(GL_POINTS, 0, 3*particles.size());
	
	// Disable and unbind
	glDisableVertexAttribArray(prog->getAttribute("aSca"));
	glDisableVertexAttribArray(prog->getAttribute("aCol"));
	glDisableVertexAttribArray(prog->getAttribute("aAlp"));
	glDisableVertexAttribArray(prog->getAttribute("aPos"));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
