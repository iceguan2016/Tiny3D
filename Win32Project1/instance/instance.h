/*
 * instance.h
 *
 *  Created on: 2017-9-28
 *      Author: a
 */

#ifndef INSTANCE_H_
#define INSTANCE_H_

#include "../render/instanceDrawcall.h"
#include "instanceData.h"

class Instance {
public:
	static std::map<Mesh*, int> instanceTable;
public:
	int insId, insSingleId;
	Mesh* instanceMesh;
	int vertexCount,indexCount;
	float* vertexBuffer;
	half* normalBuffer;
	half* tangentBuffer;
	float* texcoordBuffer;
	float* texidBuffer;
	unsigned char* colorBuffer;
	unsigned short* indexBuffer;

	int instanceCount, maxInstanceCount;
	buff* modelTransform;
	bill* billboards;

	InstanceDrawcall* drawcall;
	bool isBillboard;
	bool copyData;

	Instance(InstanceData* data);
	Instance(Mesh* mesh);
	~Instance();
	void releaseInstanceData();
	void initInstanceBuffers(Object* object,int vertices,int indices,int cnt,bool copy);
	void setRenderData(InstanceData* data);
	void addObject(Object* object, int index);
	void createDrawcall();
private:
	void create(Mesh* mesh);
	void initMatrices(int cnt);
	void initBillboards(int cnt);
};

#endif /* INSTANCE_H_ */
