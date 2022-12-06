#pragma once
#ifndef SHAPE_H
#define SHAPE_H

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

#include <memory>
#include <vector>
#include <string>

class Shape
{
public:
	Shape(); 
	virtual ~Shape();
	void loadMesh(const std::string& meshName);
	Eigen::Vector3f update(int k, bool isMoving, int vertIndex, int i);
	int getNumVerts() { return numVerts; }
	Eigen::Vector3f getVertex(int i);
	void loadBindPoses(std::vector <Eigen::Matrix4f> binds) { bindPoses = binds; }
	void loadTransformations(std::vector < std::vector<Eigen::Matrix4f> > transforms) { transformations = transforms; }
	void parseWeightData(std::string filename);
	std::vector<Eigen::Matrix4f> getProduct(int k);

private:
	std::vector<float> texBuf;
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	int numVerts;
	float offset;

	std::vector < std::vector<int> > influences;
	std::vector < std::vector<float> > weights;
	std::vector < Eigen::Matrix4f > bindPoses;
	std::vector < std::vector < Eigen::Matrix4f >> transformations;
};

#endif