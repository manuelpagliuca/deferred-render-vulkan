#pragma once

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

struct CameraData {

};

class Camera
{
public:
	// Constructors
	Camera();
	Camera(glm::vec3 initialPos, glm::vec3 _worldUp, const float initialYaw, 
		const float initialPitch, const float initialMoveSpeed, const float initialTurnSpeed);

	// Methods
	void keyControl(bool* keys, const float deltaTime);
	void mouseControl(float xChange, float yChange);

	glm::vec3 getCameraPosition();
	glm::vec3 getCameraDirection();
	glm::mat4 calculateViewMatrix();

	// Destructor
	~Camera();
private:
	glm::vec3 position;
	glm::vec3 front;	// Inverse direction of the camera
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;

	float yaw, pitch;
	float movementSpeed, turnSpeed;

	void update();
};

