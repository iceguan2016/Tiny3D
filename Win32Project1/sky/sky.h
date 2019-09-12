/*
 * sky.h
 *
 *  Created on: 2017-9-19
 *      Author: a
 */

#ifndef SKY_H_
#define SKY_H_

#include "../mesh/sphere.h"
#include "../node/staticNode.h"
#include "../render/render.h"
#include "../camera/camera.h"

class Scene;

class Sky {
private:
	Sphere* mesh;
	RenderState* state;
public:
	StaticNode* skyNode;

	Sky(Scene* scene);
	~Sky();

	void draw(Render* render,Shader* shader,Camera* camera);
};


#endif /* SKY_H_ */
