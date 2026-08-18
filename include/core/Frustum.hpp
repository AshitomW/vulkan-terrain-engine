#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct Frustum {
    glm::vec4 planes[6];

    static Frustum fromViewProj(const glm::mat4& viewProj) {
        Frustum f;
        f.planes[0] = glm::vec4(viewProj[0][3] + viewProj[0][0], viewProj[1][3] + viewProj[1][0],
                                viewProj[2][3] + viewProj[2][0], viewProj[3][3] + viewProj[3][0]);
        f.planes[1] = glm::vec4(viewProj[0][3] - viewProj[0][0], viewProj[1][3] - viewProj[1][0],
                                viewProj[2][3] - viewProj[2][0], viewProj[3][3] - viewProj[3][0]);
        f.planes[2] = glm::vec4(viewProj[0][3] - viewProj[0][1], viewProj[1][3] - viewProj[1][1],
                                viewProj[2][3] - viewProj[2][1], viewProj[3][3] - viewProj[3][1]);
        f.planes[3] = glm::vec4(viewProj[0][3] + viewProj[0][1], viewProj[1][3] + viewProj[1][1],
                                viewProj[2][3] + viewProj[2][1], viewProj[3][3] + viewProj[3][1]);
        f.planes[4] = glm::vec4(viewProj[0][2], viewProj[1][2], viewProj[2][2], viewProj[3][2]);
        f.planes[5] = glm::vec4(viewProj[0][3] - viewProj[0][2], viewProj[1][3] - viewProj[1][2],
                                viewProj[2][3] - viewProj[2][2], viewProj[3][3] - viewProj[3][2]);
        for (auto& plane : f.planes) {
            float len = glm::length(glm::vec3(plane));
            if (len > 0.0f) plane /= len;
        }
        return f;
    }

    bool intersectsAABB(const glm::vec3& min, const glm::vec3& max) const {
        for (const auto& plane : planes) {
            glm::vec3 n(plane);
            glm::vec3 pVertex{
                n.x >= 0.0f ? max.x : min.x,
                n.y >= 0.0f ? max.y : min.y,
                n.z >= 0.0f ? max.z : min.z
            };
            if (glm::dot(n, pVertex) + plane.w < 0.0f) {
                return false;
            }
        }
        return true;
    }
};