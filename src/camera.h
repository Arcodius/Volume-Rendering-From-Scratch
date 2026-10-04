#pragma once

#include <cmath>
#include <stdexcept>

#include <glm/glm.hpp>

struct Ray {
	glm::vec3 origin;
	glm::vec3 direction;
};

class Camera {
public:
	Camera(const glm::vec3& position, const glm::vec3& target,
		   const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f),
		   float verticalFovDegrees = 45.0f)
		: position_(position), verticalFovDegrees_(verticalFovDegrees) {
		if (verticalFovDegrees <= 0.0f || verticalFovDegrees >= 179.0f) {
			throw std::invalid_argument("Camera field of view must be between 0 and 179 degrees");
		}

		const glm::vec3 viewDirection = target - position;
		if (glm::length(viewDirection) <= 1e-6f) {
			throw std::invalid_argument("Camera position and target must be different");
		}
		if (glm::length(glm::cross(viewDirection, up)) <= 1e-6f) {
			throw std::invalid_argument("Camera up direction must not be parallel to its view direction");
		}

		forward_ = glm::normalize(viewDirection);
		right_ = glm::normalize(glm::cross(forward_, up));
		up_ = glm::normalize(glm::cross(right_, forward_));
	}

	Ray generateRay(float screenX, float screenY, float aspectRatio) const {
		const float halfHeight = std::tan(glm::radians(verticalFovDegrees_) * 0.5f);
		const glm::vec3 direction = glm::normalize(
			forward_ + right_ * (screenX * halfHeight * aspectRatio) + up_ * (screenY * halfHeight));
		return {position_, direction};
	}

private:
	glm::vec3 position_;
	glm::vec3 forward_;
	glm::vec3 right_;
	glm::vec3 up_;
	float verticalFovDegrees_;

};