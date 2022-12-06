#include <iostream>
#include <fstream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

#include "Shape.h"

using namespace std;
using namespace Eigen;

Shape::Shape()
{

}

Shape::~Shape()
{

}

void Shape::loadMesh(const string& meshName)
{
	// Load geometry
		// This works only if the OBJ file has the same indices for v/n/t.
		// In other words, the 'f' lines must look like:
		// f 70/70/70 41/41/41 67/67/67
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &errStr, meshName.c_str());
	if (!rc) {
		cerr << errStr << endl;
	}
	else {
		posBuf = attrib.vertices;
		norBuf = attrib.normals;
		texBuf = attrib.texcoords;
		assert(posBuf.size() == norBuf.size());
		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			const tinyobj::mesh_t& mesh = shapes[s].mesh;
			size_t index_offset = 0;
			for (size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
				size_t fv = mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = mesh.indices[index_offset + v];
				}
				index_offset += fv;
				// per-face material (IGNORE)
				//shapes[s].mesh.material_ids[f];
			}
		}
	}

	numVerts = posBuf.size() / 3;

	float max = 0.0;
	for (int i = 1; i < posBuf.size(); i += 3)
	{
		if (posBuf[i] > max)
		{
			max = posBuf[i];
		}
	}

	// shift mesh to origin
	offset = max / 2.0;
}

Vector3f Shape::getVertex(int i)
{
	Vector3f vertex;
	vertex << posBuf[i], posBuf[i + 1], posBuf[i + 2];
	return vertex;
}

Vector3f Shape::update(int k, bool isMoving, int vertIndex, int i)
{
	// TODO: CPU skinning calculations.
	// After computing the new positions and normals, send the new data
	// to the GPU by copying and pasting the relevant code from the
	// init() function.

	/*
	for every vertex i to totalVerts
		x0 = bindPos position at i
		J = influences at position i
		x = (0, 0, 0)
		for every bone j to J
			M0 = bind transformation at bone j
			Mk = transformation at bone j at frame k
			w = skinning weight at bone j at frame k
			x += w * Mk * inverse(M0) * x0
	*/
	if (isMoving)
	{
		//if (k == 30)
		//{
		//	cout << endl;
		//}
		vector<Matrix4f> products = getProduct(k);

		Vector4f x0(posBuf[vertIndex], posBuf[vertIndex + 1], posBuf[vertIndex + 2], 1.0f);
		Vector4f n0(norBuf[vertIndex], norBuf[vertIndex + 1], norBuf[vertIndex + 2], 0.0f);
		vector<int> J = influences[i];
		vector<float> W = weights[i];

		Vector3f x(0.0f, 0.0f, 0.0f);
		Vector3f n(0.0f, 0.0f, 0.0f);

		for (int j = 0; j < J.size(); j++)
		{
			int bone = J[j];
			float wij = W[j];

			Matrix4f M0 = bindPoses[bone];
			Matrix4f Mk = transformations[k][bone];
			Matrix4f prod = Mk * M0;
			//Matrix4f prod = products[bone];
			// prod(3, 1) = prod(3, 1) - offset;
			Vector4f xij = wij * prod * x0;
			Vector4f nij = wij * prod * n0;
			x += Vector3f(xij(0), xij(1), xij(2));
			n += Vector3f(nij(0), nij(1), nij(2));
		}

		x(1) = x(1) - offset;

		return x;
	}
}


std::vector<Eigen::Matrix4f> Shape::getProduct(int k)
{
	vector<Matrix4f> products;
	for (int j = 0; j < bindPoses.size(); j++)
	{
		Matrix4f M0 = bindPoses[j];
		Matrix4f Mk = transformations[k][j];
		products.push_back(Mk * M0);
	}
	return products;
}

void Shape::parseWeightData(std::string filename)
{
	ifstream in;
	in.open(filename);
	if (!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}
	cout << "Loading " << filename << endl;

	string line;
	int lineIndex = 0, numVertices, numBones, numWeights;
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
		if (lineIndex == 0) {

			ss >> numVertices;
			ss >> numBones;
			ss >> numWeights;
			lineIndex++;
			continue;
		}

		int numInfluences;
		ss >> numInfluences;

		vector<int> influence;
		vector<float> weight;
		for (int i = 0; i < numInfluences; i++)
		{
			int infl;
			float wt;
			ss >> infl;
			ss >> wt;
			influence.push_back(infl);
			weight.push_back(wt);
		}
		for (int i = 0; i < (numWeights - numInfluences); i++)
		{
			influence.push_back(0);
			weight.push_back(0);
		}

		this->influences.push_back(influence);
		this->weights.push_back(weight);
	}
}