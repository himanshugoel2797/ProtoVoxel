#pragma once
#include "glm/glm.hpp"

class Frustum
{
public:
	Frustum() {}

	// m = ProjectionMatrix * ViewMatrix 
	Frustum(glm::mat4 m);

	// http://iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm
	bool IsBoxVisible(const glm::vec3& minp, const glm::vec3& maxp) const;

private:
	int Frustum::CalcPlaneBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& n, const float d) const;

	glm::vec3   near_n;
	float   near_p;
	glm::vec3   lft_n;
	float      lft_p;
	glm::vec3   rgt_n;
	float      rgt_p;
	glm::vec3   top_n;
	float      top_p;
	glm::vec3   btm_n;
	float      btm_p;
};

inline Frustum::Frustum(glm::mat4 m)
{
	auto iVP = glm::inverse(m);
	auto ntl = iVP * glm::vec4(-1, 1, 1, 1);
	auto ntr = iVP * glm::vec4(1, 1, 1, 1);
	auto nbl = iVP * glm::vec4(-1, -1, 1, 1);
	auto nbr = iVP * glm::vec4(1, -1, 1, 1);

	auto ftl = iVP * glm::vec4(-1, 1, 0.01f, 1);
	auto ftr = iVP * glm::vec4(1, 1, 0.01f, 1);
	auto fbl = iVP * glm::vec4(-1, -1, 0.01f, 1);
	auto fbr = iVP * glm::vec4(1, -1, 0.01f, 1);

	ntl /= ntl.w;
	ntr /= ntr.w;
	nbl /= nbl.w;
	nbr /= nbr.w;

	ftl /= ftl.w;
	ftr /= ftr.w;
	fbl /= fbl.w;
	fbr /= fbr.w;

	//Compute planes
	auto a = glm::vec3(nbl.x, nbl.y, nbl.z);
	auto b = glm::vec3(nbr.x, nbr.y, nbr.z);
	auto c = glm::vec3(ntl.x, ntl.y, ntl.z);
	{
		auto ab = b - a;
		auto ac = c - a;

		auto cr = glm::cross(ab, ac);
		near_n = glm::normalize(cr);
		near_p = -glm::dot(near_n, a);
	}

	a = glm::vec3(nbl.x, nbl.y, nbl.z);
	b = glm::vec3(ntl.x, ntl.y, ntl.z);
	c = glm::vec3(fbl.x, fbl.y, fbl.z);
	{
		auto ab = b - a;
		auto ac = c - a;

		auto cr = glm::cross(ab, ac);
		lft_n = glm::normalize(cr);
		lft_p = -glm::dot(lft_n, a);
	}

	a = glm::vec3(ntr.x, ntr.y, ntr.z);
	b = glm::vec3(nbr.x, nbr.y, nbr.z);
	c = glm::vec3(ftr.x, ftr.y, ftr.z);
	{
		auto ab = b - a;
		auto ac = c - a;

		auto cr = glm::cross(ab, ac);
		rgt_n = glm::normalize(cr);
		rgt_p = -glm::dot(rgt_n, a);
	}

	a = glm::vec3(ntl.x, ntl.y, ntl.z);
	b = glm::vec3(ntr.x, ntr.y, ntr.z);
	c = glm::vec3(ftl.x, ftl.y, ftl.z);
	{
		auto ab = b - a;
		auto ac = c - a;

		auto cr = glm::cross(ab, ac);
		top_n = glm::normalize(cr);
		top_p = -glm::dot(top_n, a);
	}

	a = glm::vec3(nbr.x, nbr.y, nbr.z);
	b = glm::vec3(nbl.x, nbl.y, nbl.z);
	c = glm::vec3(fbr.x, fbr.y, fbr.z);
	{
		auto ab = b - a;
		auto ac = c - a;

		auto cr = glm::cross(ab, ac);
		btm_n = glm::normalize(cr);
		btm_p = -glm::dot(btm_n, a);
	}
}

inline int Frustum::CalcPlaneBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& n, const float d_) const {
	glm::vec3 a(min.x, min.y, min.z);
	glm::vec3 b(max.x, min.y, min.z);
	glm::vec3 c(min.x, max.y, min.z);
	glm::vec3 d(max.x, max.y, min.z);
	glm::vec3 e(min.x, min.y, max.z);
	glm::vec3 f(max.x, min.y, max.z);
	glm::vec3 g(min.x, max.y, max.z);
	glm::vec3 h(max.x, max.y, max.z);

	int o = 0;
	o += (glm::dot(a, n) + d_ >= 0.0f) ? 1 : 0;
	o += (glm::dot(b, n) + d_ >= 0.0f) ? 1 : 0;
	o += (glm::dot(c, n) + d_ >= 0.0f) ? 1 : 0;
	o += (glm::dot(d, n) + d_ >= 0.0f) ? 1 : 0;
	o += (glm::dot(e, n) + d_ >= 0.0f) ? 1 : 0;
	o += (glm::dot(f, n) + d_ >= 0.0f) ? 1 : 0;
	o += (glm::dot(g, n) + d_ >= 0.0f) ? 1 : 0;
	o += (glm::dot(h, n) + d_ >= 0.0f) ? 1 : 0;
	if (o == 8) return 0;

	return 1;
}

// http://iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm
inline bool Frustum::IsBoxVisible(const glm::vec3& min, const glm::vec3& max) const
{
	if (CalcPlaneBox(min, max, lft_n, lft_p) == 0)
		return false;
	if (CalcPlaneBox(min, max, rgt_n, rgt_p) == 0)
		return false;
	if (CalcPlaneBox(min, max, top_n, top_p) == 0)
		return false;
	if (CalcPlaneBox(min, max, btm_n, btm_p) == 0)
		return false;
	if (CalcPlaneBox(min, max, near_n, near_p) == 0)
		return false;
	return true;
}