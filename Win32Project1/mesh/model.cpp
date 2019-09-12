#include "model.h"
#include "../constants/constants.h"
#include "../material/materialManager.h"
#include "../util/util.h"
#include <stdlib.h>
#include <string.h>

Model::Model(const char* obj, const char* mtl, int vt, bool simple) :Mesh() {
	vertexCount = 0;
	indexCount = 0;
	vertices = NULL;
	normals = NULL;
	tangents = NULL;
	texcoords = NULL;
	materialids = NULL;
	indices = NULL;
	mats.clear();
	if (simple) loadModelSimple(obj, mtl, vt);
	else loadModel(obj, mtl, vt);
	caculateExData();
}

Model::Model(const Model& rhs) :Mesh(rhs) {
	if (rhs.vertexCount > 0) {
		vertexCount = rhs.vertexCount;
		vertices = new vec4[vertexCount];
		normals = new vec3[vertexCount];
		tangents = new vec3[vertexCount];
		texcoords = new vec2[vertexCount];
		materialids = new int[vertexCount];
		for (int i = 0; i < vertexCount; i++) {
			vertices[i] = rhs.vertices[i];
			normals[i] = rhs.normals[i];
			tangents[i] = rhs.tangents[i];
			texcoords[i] = rhs.texcoords[i];
			materialids[i] = rhs.materialids[i];
		}
	}

	if (rhs.indexCount > 0) {
		indexCount = rhs.indexCount;
		indices = (int*)malloc(indexCount * sizeof(int));
		memcpy(indices, rhs.indices, indexCount * sizeof(int));
	}

	for (int i = 0; i < rhs.mats.size(); ++i)
		mats.push_back(rhs.mats[i]);

	caculateExData();
}

Model::~Model() {
	mats.clear();
}

void Model::loadModel(const char* obj,const char* mtl,int vt) {
	loader=new ObjLoader(obj,mtl,vt);
	initFaces();
	delete loader;
	loader=NULL;
}

void Model::loadModelSimple(const char* obj,const char* mtl,int vt) {
	loader=new ObjLoader(obj,mtl,vt);
	initFacesWidthIndices();
	delete loader;
	loader=NULL;
}

void Model::initFaces() {
	vertexCount=loader->faceCount*3;
	vertices=new vec4[vertexCount];
	normals=new vec3[vertexCount];
	tangents = new vec3[vertexCount];
	texcoords=new vec2[vertexCount];
	materialids = new int[vertexCount];

	for (int i=0;i<loader->faceCount;i++) {
		vec3 p1(loader->vArr[loader->fvArr[i][0]-1][0],
				loader->vArr[loader->fvArr[i][0]-1][1],
				loader->vArr[loader->fvArr[i][0]-1][2]);
		vec3 p2(loader->vArr[loader->fvArr[i][1]-1][0],
				loader->vArr[loader->fvArr[i][1]-1][1],
				loader->vArr[loader->fvArr[i][1]-1][2]);
		vec3 p3(loader->vArr[loader->fvArr[i][2]-1][0],
				loader->vArr[loader->fvArr[i][2]-1][1],
				loader->vArr[loader->fvArr[i][2]-1][2]);

		vec3 n1(loader->vnArr[loader->fnArr[i][0]-1][0],
				loader->vnArr[loader->fnArr[i][0]-1][1],
				loader->vnArr[loader->fnArr[i][0]-1][2]);
		vec3 n2(loader->vnArr[loader->fnArr[i][1]-1][0],
				loader->vnArr[loader->fnArr[i][1]-1][1],
				loader->vnArr[loader->fnArr[i][1]-1][2]);
		vec3 n3(loader->vnArr[loader->fnArr[i][2]-1][0],
				loader->vnArr[loader->fnArr[i][2]-1][1],
				loader->vnArr[loader->fnArr[i][2]-1][2]);

		vec2 c1(loader->vtArr[loader->ftArr[i][0]-1][0],
				loader->vtArr[loader->ftArr[i][0]-1][1]);
		vec2 c2(loader->vtArr[loader->ftArr[i][1]-1][0],
				loader->vtArr[loader->ftArr[i][1]-1][1]);
		vec2 c3(loader->vtArr[loader->ftArr[i][2]-1][0],
				loader->vtArr[loader->ftArr[i][2]-1][1]);

		vertices[i * 3 + 0].x = p1.x; vertices[i * 3 + 0].y = p1.y; vertices[i * 3 + 0].z = p1.z; vertices[i * 3 + 0].w = 1;
		vertices[i * 3 + 1].x = p2.x; vertices[i * 3 + 1].y = p2.y; vertices[i * 3 + 1].z = p2.z; vertices[i * 3 + 1].w = 1;
		vertices[i * 3 + 2].x = p3.x; vertices[i * 3 + 2].y = p3.y; vertices[i * 3 + 2].z = p3.z; vertices[i * 3 + 2].w = 1;

		normals[i * 3 + 0].x = n1.x; normals[i * 3 + 0].y = n1.y; normals[i * 3 + 0].z = n1.z;
		normals[i * 3 + 1].x = n2.x; normals[i * 3 + 1].y = n2.y; normals[i * 3 + 1].z = n2.z;
		normals[i * 3 + 2].x = n3.x; normals[i * 3 + 2].y = n3.y; normals[i * 3 + 2].z = n3.z;

		texcoords[i * 3 + 0].x = c1.x; texcoords[i * 3 + 0].y = c1.y;
		texcoords[i * 3 + 1].x = c2.x; texcoords[i * 3 + 1].y = c2.y;
		texcoords[i * 3 + 2].x = c3.x; texcoords[i * 3 + 2].y = c3.y;

		vec3 faceTangent = CaculateTangent(p1, p2, p3, c1, c2, c3);
		tangents[i * 3 + 0] = faceTangent;
		tangents[i * 3 + 1] = faceTangent;
		tangents[i * 3 + 2] = faceTangent;

		int mid = loader->mtlLoader->objMtls[loader->mtArr[i]];
		materialids[i * 3 + 0] = mid;
		materialids[i * 3 + 1] = mid;
		materialids[i * 3 + 2] = mid;
	}
}

void Model::initFacesWidthIndices() {
	vertexCount = loader->vCount;
	indexCount = loader->faceCount * 3;
	vertices = new vec4[indexCount];
	for(int i=0;i<vertexCount;i++) {
		vertices[i].x=loader->vArr[i][0];
		vertices[i].y=loader->vArr[i][1];
		vertices[i].z=loader->vArr[i][2];
		vertices[i].w=1.0;
	}

	normals = new vec3[indexCount];
	tangents = new vec3[indexCount];
	texcoords = new vec2[indexCount];
	materialids = new int[indexCount];
	indices = (int*)malloc(indexCount*sizeof(int));

	std::vector<bool> statlst; statlst.clear();
	std::vector<int> startlst; startlst.clear();
	std::vector<int> countlst; countlst.clear();
	int laststat = -1;

	std::map<int, bool> texcoordMap; texcoordMap.clear();

	int dupIndex = vertexCount;
	for (int i=0;i<loader->faceCount;i++) {
		int index1=loader->fvArr[i][0]-1;
		int index2=loader->fvArr[i][1]-1;
		int index3=loader->fvArr[i][2]-1;

		vec3 n1(loader->vnArr[loader->fnArr[i][0]-1][0],
				loader->vnArr[loader->fnArr[i][0]-1][1],
				loader->vnArr[loader->fnArr[i][0]-1][2]);
		vec3 n2(loader->vnArr[loader->fnArr[i][1]-1][0],
				loader->vnArr[loader->fnArr[i][1]-1][1],
				loader->vnArr[loader->fnArr[i][1]-1][2]);
		vec3 n3(loader->vnArr[loader->fnArr[i][2]-1][0],
				loader->vnArr[loader->fnArr[i][2]-1][1],
				loader->vnArr[loader->fnArr[i][2]-1][2]);

		vec2 c1(loader->vtArr[loader->ftArr[i][0]-1][0],
				loader->vtArr[loader->ftArr[i][0]-1][1]);
		vec2 c2(loader->vtArr[loader->ftArr[i][1]-1][0],
				loader->vtArr[loader->ftArr[i][1]-1][1]);
		vec2 c3(loader->vtArr[loader->ftArr[i][2]-1][0],
				loader->vtArr[loader->ftArr[i][2]-1][1]);
		
		// Duplicate vertex if texcoord not the same
		if (texcoordMap.find(index1) != texcoordMap.end() && texcoords[index1] != c1) {
			int newIndex = dupIndex++;
			vertices[newIndex] = vertices[index1];
			index1 = newIndex;
		}
		if (texcoordMap.find(index2) != texcoordMap.end() && texcoords[index2] != c2) {
			int newIndex = dupIndex++;
			vertices[newIndex] = vertices[index2];
			index2 = newIndex;
		}
		if (texcoordMap.find(index3) != texcoordMap.end() && texcoords[index3] != c3) {
			int newIndex = dupIndex++;
			vertices[newIndex] = vertices[index3];
			index3 = newIndex;
		}
		
		normals[index1] = n1; normals[index2] = n2; normals[index3] = n3;
		texcoords[index1] = c1; texcoords[index2] = c2; texcoords[index3] = c3;
		texcoordMap[index1] = true; texcoordMap[index2] = true; texcoordMap[index3] = true;

		vec3 faceTangent = CaculateTangent(vertices[index1], vertices[index2], vertices[index3], texcoords[index1], texcoords[index2], texcoords[index3]);
		tangents[index1] = faceTangent;
		tangents[index2] = faceTangent;
		tangents[index3] = faceTangent;

		indices[i * 3 + 0] = index1;
		indices[i * 3 + 1] = index2;
		indices[i * 3 + 2] = index3;

		int mid = loader->mtlLoader->objMtls[loader->mtArr[i]];
		materialids[index1] = mid;
		materialids[index2] = mid;
		materialids[index3] = mid;

		int curstat = 0;
		Material* mat = MaterialManager::materials->find(mid);
		if (mat && mat->singleFace) curstat = 1;
		else curstat = 0;
		if (laststat == curstat)
			countlst[countlst.size() - 1] += 3;
		else {
			startlst.push_back(i * 3);
			countlst.push_back(3);
			statlst.push_back(curstat);
		}
		laststat = curstat;
	}

	for (uint i = 0; i < statlst.size(); i++) {
		bool stat = statlst[i];
		if (!stat) normalFaces.push_back(new FaceBuf(startlst[i], countlst[i]));
		else singleFaces.push_back(new FaceBuf(startlst[i], countlst[i]));
	}
	vertexCount = dupIndex;
}
