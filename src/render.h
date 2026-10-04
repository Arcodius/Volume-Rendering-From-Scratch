#pragma once

#include "camera.h"
#include "image.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <glm/glm.hpp>

namespace volume_render_detail {

inline bool intersectVolume(const Ray& ray, float& nearDistance, float& farDistance) {
	nearDistance = -INFINITY;
	farDistance = INFINITY;

	for (int axis = 0; axis < 3; ++axis) {
		if (std::abs(ray.direction[axis]) <= 1e-8f) {
			if (ray.origin[axis] < -1.0f || ray.origin[axis] > 1.0f) {
				return false;
			}
			continue;
		}

		float first = (-1.0f - ray.origin[axis]) / ray.direction[axis];
		float second = (1.0f - ray.origin[axis]) / ray.direction[axis];
		if (first > second) {
			std::swap(first, second);
		}
		nearDistance = std::max(nearDistance, first);
		farDistance = std::min(farDistance, second);
		if (nearDistance > farDistance) {
			return false;
		}
	}

	return farDistance >= std::max(nearDistance, 0.0f);
}

inline float sampleDensity(const VolumeGrid3D& volume, const glm::vec3& position) {
	const float scale = static_cast<float>(volume.resolution - 1) * 0.5f;
	const glm::vec3 gridPosition = glm::clamp((position + 1.0f) * scale, 0.0f,
											  static_cast<float>(volume.resolution - 1));
	const glm::ivec3 lower = glm::ivec3(glm::floor(gridPosition));
	const glm::ivec3 upper = glm::min(lower + 1, glm::ivec3(volume.resolution - 1));
	const glm::vec3 fraction = gridPosition - glm::vec3(lower);

	const float x00 = glm::mix(volume.get(lower.x, lower.y, lower.z), volume.get(upper.x, lower.y, lower.z), fraction.x);
	const float x10 = glm::mix(volume.get(lower.x, upper.y, lower.z), volume.get(upper.x, upper.y, lower.z), fraction.x);
	const float x01 = glm::mix(volume.get(lower.x, lower.y, upper.z), volume.get(upper.x, lower.y, upper.z), fraction.x);
	const float x11 = glm::mix(volume.get(lower.x, upper.y, upper.z), volume.get(upper.x, upper.y, upper.z), fraction.x);
	return glm::mix(glm::mix(x00, x10, fraction.y), glm::mix(x01, x11, fraction.y), fraction.z);
}

} // namespace volume_render_detail

inline Image renderVolume(const VolumeGrid3D& volume, const Camera& camera, int width, int height) {
	if (volume.resolution < 2 || volume.data.size() < static_cast<std::size_t>(volume.resolution) *
												   volume.resolution * volume.resolution) {
		throw std::invalid_argument("Volume data does not match its resolution");
	}

	Image image(width, height);
	const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	const float stepSize = 1.0f / static_cast<float>(volume.resolution);
	constexpr float extinction = 8.0f;
	constexpr float background = 24.0f;
	constexpr float cloudBrightness = 245.0f;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const float screenX = (2.0f * (static_cast<float>(x) + 0.5f) / width) - 1.0f;
			const float screenY = 1.0f - (2.0f * (static_cast<float>(y) + 0.5f) / height);
			const Ray ray = camera.generateRay(screenX, screenY, aspectRatio);

			float nearDistance = 0.0f;
			float farDistance = 0.0f;
			float transmittance = 1.0f;
			if (volume_render_detail::intersectVolume(ray, nearDistance, farDistance)) {
				const float startDistance = std::max(nearDistance, 0.0f);
				for (float distance = startDistance; distance < farDistance; distance += stepSize) {
					const float density = glm::clamp(
						volume_render_detail::sampleDensity(volume, ray.origin + ray.direction * distance), 0.0f, 1.0f);
					transmittance *= std::exp(-density * extinction * stepSize);
					if (transmittance < 0.01f) {
						break;
					}
				}
			}

			const float brightness = background * transmittance + cloudBrightness * (1.0f - transmittance);
			image(x, y) = static_cast<Image::Pixel>(std::lround(brightness));
		}
	}

	return image;
}

