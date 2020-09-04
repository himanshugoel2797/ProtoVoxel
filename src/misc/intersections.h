#pragma once
#include "glm/glm.hpp"

namespace ProtoVoxel::Misc {
	class Intersections {
	public:
		static inline bool Ray_AABB(const glm::vec3& o, const glm::vec3& dir, const glm::vec3& aabb_min, const glm::vec3& aabb_max, float &t_near, float &t_far);
	};
}

inline bool ProtoVoxel::Misc::Intersections::Ray_AABB(const glm::vec3& o, const glm::vec3& dir, const glm::vec3& aabb_min, const glm::vec3& aabb_max, float &t_near, float &t_far)
{
	glm::vec3 invR = 1.0f / dir;
	glm::vec3 tmin = invR * (aabb_min - o);
	glm::vec3 tmax = invR * (aabb_max - o);
	glm::vec3 t1_ = glm::min(tmin, tmax);
	glm::vec3 t2_ = glm::max(tmin, tmax);

	t_near = glm::max(t1_.x, glm::max(t1_.y, t1_.z));
	t_far = glm::min(t2_.x, glm::min(t2_.y, t2_.z));
	return t_near <= t_far;
}