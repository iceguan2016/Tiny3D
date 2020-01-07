#include "renderQueue.h"
#include "../node/staticNode.h"
#include "../node/animationNode.h"
#include "../node/instanceNode.h"
#include "../assets/assetManager.h"
#include "../scene/scene.h"
#include <string.h>
#include <stdlib.h>
using namespace std;

RenderQueue::RenderQueue(int type, float midDis, float lowDis) {
	queueType = type;
	queue = new Queue(1);
	instanceQueue.clear();
	multiInstance = NULL;
	batchData = NULL;
	midDistSqr = powf(midDis, 2);
	lowDistSqr = powf(lowDis, 2);
	dual = true;
	shadowLevel = 0;
	firstFlush = true;
}

RenderQueue::~RenderQueue() {
	delete queue;

	if (batchData) delete batchData;
	if (multiInstance) delete multiInstance;

	map<Mesh*, InstanceData*>::iterator itIns;
	for (itIns = instanceQueue.begin(); itIns != instanceQueue.end(); ++itIns)
		delete itIns->second;
	instanceQueue.clear();
}

void RenderQueue::setDual(bool dual) {
	this->dual = dual;
}

void RenderQueue::push(Node* node) {
	queue->push(node);
}

void RenderQueue::flush() {
	queue->flush();
	
	map<Mesh*, InstanceData*>::iterator itData = instanceQueue.begin();
	while (itData != instanceQueue.end()) {
		itData->second->resetInstance();
		++itData;
	}
	
	if (batchData)
		batchData->resetBatch();
}

void RenderQueue::deleteInstance(InstanceData* data) {
	if (data->instance)
		delete data->instance;
	data->instance = NULL;
}

void RenderQueue::pushDatasToInstance(Scene* scene, InstanceData* data, bool copy) {
	if (!data->instance) {
		data->instance = new Instance(data);
		data->instance->initInstanceBuffers(data->object, data->insMesh->vertexCount, data->insMesh->indexCount, scene->queryMeshCount(data->insMesh), copy);
	}
	data->instance->setRenderData(data);
}

void RenderQueue::pushDatasToBatch(BatchData* data, int pass) {
	if (data->objectCount <= 0) return;
	if (!data->batch) {
		data->batch = new Batch(); 
		data->batch->initBatchBuffers(MAX_VERTEX_COUNT, MAX_INDEX_COUNT);
		data->batch->setDynamic(true);
	}
	data->batch->setRenderData(pass, data);
}

void RenderQueue::draw(Scene* scene, Camera* camera, Render* render, RenderState* state) {
	for (int it = 0; it < queue->size; it++) {
		Node* node = queue->get(it);
		if (!node->needUpdateNode) {
			if (node->needCreateDrawcall) 
				node->prepareDrawcall();
			else if (node->needUpdateDrawcall) {
				node->updateRenderData();
				node->updateDrawcall();
			}
		}

		if (node->type == TYPE_STATIC) 
			render->draw(camera, node->drawcall, state);
		else if (node->type == TYPE_ANIMATE) {
			node->drawcall->uModelMatrix = node->uTransformMatrix->entries;

			if (!node->drawcall->uNormalMatrix)
				node->drawcall->uNormalMatrix = (float*)malloc(9 * sizeof(float));
			memcpy(node->drawcall->uNormalMatrix, node->uNormalMatrix->entries, 3 * sizeof(float));
			memcpy(node->drawcall->uNormalMatrix + 3, node->uNormalMatrix->entries + 4, 3 * sizeof(float));
			memcpy(node->drawcall->uNormalMatrix + 6, node->uNormalMatrix->entries + 8, 3 * sizeof(float));

			render->draw(camera, node->drawcall, state);
		} else if (node->type == TYPE_TERRAIN) {
			if (state->pass == COLOR_PASS) {
				static Shader* terrainShader = render->findShader("terrain");
				Shader* shader = state->shader;
				state->shader = terrainShader;
				render->draw(camera, node->drawcall, state);
				state->shader = shader;
			}
		}
	}

	map<Mesh*, InstanceData*>::iterator itData = instanceQueue.begin();
	while (itData != instanceQueue.end()) {
		InstanceData* data = itData->second;
		pushDatasToInstance(scene, data, false);
		Instance* instance = data->instance;
		if (instance && instance->isBillboard) {
			if (!instance->drawcall) instance->createDrawcall();
			if (data->count > 0 && (instance->modelTransform || instance->billboards)) {
				instance->drawcall->updateInstances(instance, state->pass);
				render->draw(camera, instance->drawcall, state);
			}
		} else if (instance && !instance->isBillboard) {
			if (!multiInstance) multiInstance = new MultiInstance();
			if (!multiInstance->inited()) multiInstance->add(instance);
		}
		++itData;
		//if (instance && Instance::instanceTable[instance->instanceMesh] == 0)
		//	deleteInstance(data);
	}

	if (multiInstance) {
		if (!multiInstance->inited()) {
			multiInstance->initBuffers();
			multiInstance->createDrawcall();
		}
		multiInstance->drawcall->update(render, state);
		render->draw(camera, multiInstance->drawcall, state);
	}
	
	if (batchData) {
		pushDatasToBatch(batchData, state->pass);
		Batch* batch = batchData->batch;
		if (batch) {
			if (!batch->drawcall) batch->createDrawcall();
			if (batch->objectCount > 0) {
				batch->drawcall->updateBuffers(state->pass);
				batch->drawcall->updateMatrices();
				render->draw(camera, batch->drawcall, state);
			}
		}
	}
}

void RenderQueue::animate(long startTime, long currentTime) {
	for (int it = 0; it < queue->size; it++) {
		Node* node = queue->get(it);
		if (node->type == TYPE_ANIMATE) {
			AnimationNode* animateNode = (AnimationNode*)node;
			animateNode->animate(0, startTime, currentTime);
		}
	}
}

Mesh* RenderQueue::queryLodMesh(Object* object, const vec3& eye) {
	Mesh* mesh = object->mesh;
	float e2oDis = (eye - object->bounding->position).GetSquaredLength();
	if (e2oDis > lowDistSqr) 
		mesh = object->meshLow;
	else if (e2oDis > midDistSqr) 
		mesh = object->meshMid;
	
	return mesh;
}

void PushNodeToQueue(RenderQueue* queue, Scene* scene, Node* node, Camera* camera, Camera* mainCamera) {
	if (queue->firstFlush) {
		if (queue->queueType == QUEUE_STATIC_SN || queue->queueType == QUEUE_STATIC_SM || 
			queue->queueType == QUEUE_STATIC_SF || queue->queueType == QUEUE_STATIC) {
			for (uint i = 0; i < scene->meshes.size(); ++i) {
				Mesh* mesh = scene->meshes[i]->mesh;
				Object* object = scene->meshes[i]->object;
				InstanceData* insData = new InstanceData(mesh, object, scene->queryMeshCount(mesh));
				queue->instanceQueue.insert(pair<Mesh*, InstanceData*>(mesh, insData));
			}
		}
		queue->firstFlush = false;
	}

	if (node->checkInCamera(camera)) {
		for (unsigned int i = 0; i<node->children.size(); ++i) {
			Node* child = node->children[i];
			if (child->objects.size() <= 0)
				PushNodeToQueue(queue, scene, child, camera, mainCamera);
			else {
				if (child->shadowLevel < queue->shadowLevel) continue;

				if (child->checkInCamera(camera)) {
					if (child->type != TYPE_INSTANCE && child->type != TYPE_STATIC)
						queue->push(child);
					else if (child->type == TYPE_STATIC) {
						if (!((StaticNode*)child)->isDynamicBatch())
							queue->push(child);
						else {
							child->needCreateDrawcall = false;
							child->needUpdateDrawcall = false;
							for (uint j = 0; j < child->objects.size(); ++j) {
								Object* object = child->objects[j];
								if (queue->shadowLevel > 0 && !object->genShadow) continue;
								if (object->checkInCamera(camera)) {
									Mesh* mesh = queue->queryLodMesh(object, mainCamera->position);
									if (!mesh) continue;
									if (!queue->batchData)
										queue->batchData = new BatchData();
									queue->batchData->addObject(object, mesh);
								}
							}
						}
					} else if (child->type == TYPE_INSTANCE) {
						child->needCreateDrawcall = false;
						child->needUpdateDrawcall = false;
						
						for (uint j = 0; j < child->objects.size(); ++j) {
							Object* object = child->objects[j];
							if (queue->shadowLevel > 0 && !object->genShadow) continue;
							if (object->checkInCamera(camera)) {
								Mesh* mesh = queue->queryLodMesh(object, mainCamera->position);
								if (!mesh) continue;
								InstanceData* insData = queue->instanceQueue[mesh];
								insData->addInstance(object);
							}
						}
					}
				}
			}
		}
	}
}