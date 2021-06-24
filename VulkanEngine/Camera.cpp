#include "pch.h"
#include "Camera.h"

Camera::Camera()
{
	position = glm::vec3(0.0f, 0.0f, 0.0f);
	front = glm::vec3(0.0f, 0.0f, -1.0f);
	up = glm::vec3(0.0f);
	right = glm::vec3(0.0f);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

	yaw = 0;
	pitch = 0;
	movementSpeed = 0;
	turnSpeed = 0;
}

Camera::Camera(glm::vec3 initialPos, glm::vec3 _worldUp, 
	const float initialYaw, const float initialPitch, const float initialMoveSpeed, const float initialTurnSpeed)
{
	position	= initialPos;
	worldUp		= _worldUp;
	yaw			= initialYaw;
	pitch		= initialPitch;
	front		= glm::vec3(0.0f, 0.0f, -1.0f);

	movementSpeed = initialMoveSpeed;
	turnSpeed	= initialTurnSpeed;

	update();
}

void Camera::keyControl(bool* keys, const float deltaTime)
{
	const float velocity = movementSpeed * deltaTime;

	if (keys[GLFW_KEY_W])
	{
		position += front * velocity;
	}

	if (keys[GLFW_KEY_A])
	{
		position -= right * velocity;
	}

	if (keys[GLFW_KEY_S])
	{
		position -= front * velocity;
	}

	if (keys[GLFW_KEY_D])
	{
		position += right * velocity;
	}
}

void Camera::mouseControl(float xChange, float yChange)
{
	xChange *= turnSpeed;
	yChange *= turnSpeed;

	yaw += xChange;
	pitch += yChange;

	if (pitch > 89.0f)
	{
		pitch = 89.0f;
	}

	if (pitch < -89.0f)
	{
		pitch = -89.0f;
	}

	update();
}

glm::mat4 Camera::calculateViewMatrix()
{
	return glm::lookAt(position, position + front, up);	// Sommando la posizione della camera (che può variare) al vettore front che semplicemente indica z-, otterremo il target sempre aggiornato
}

glm::vec3 Camera::getCameraPosition()
{
	return position;
}

glm::vec3 Camera::getCameraDirection()
{
	return glm::normalize(front);
}

void Camera::update()
{
	front.x = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	front.y = sinf(glm::radians(pitch));
	front.z = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	front	= glm::normalize(front);

	right	= glm::normalize(glm::cross(front, worldUp));
	up		= glm::normalize(glm::cross(right, front));
}

Camera::~Camera()
{

}