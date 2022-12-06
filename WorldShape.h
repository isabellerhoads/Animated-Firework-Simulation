//#pragma once
//#ifndef WORLDSHAPE_H
//#define WORLDSHAPE_H
//
//#include <string>
//#include <vector>
//#include <memory>
//
//class Program;
//
///**
// * A shape defined by a list of triangles
// * - posBuf should be of length 3*ntris
// * - norBuf should be of length 3*ntris (if normals are available)
// * - texBuf should be of length 2*ntris (if texture coords are available)
// * posBufID, norBufID, and texBufID are OpenGL buffer identifiers.
// */
//class WorldShape
//{
//public:
//	WorldShape();
//	virtual ~WorldShape();
//	void loadMesh(const std::string& meshName);
//	void fitToUnitBox();
//	void init();
//	void draw(const std::shared_ptr<Program> prog) const;
//	float get_minimum_y();
//
//private:
//	std::vector<float> posBuf;
//	std::vector<float> norBuf;
//	std::vector<float> texBuf;
//	unsigned posBufID;
//	unsigned norBufID;
//	unsigned texBufID;
//};
//
//#endif