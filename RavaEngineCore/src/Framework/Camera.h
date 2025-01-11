#pragma once

namespace Rava {
class Camera {
   public:
	Camera();
	~Camera() = default;

	void SetOrthographicProjection(float left, float right, float top, float bottom, float zNear, float zFar);
	void SetPerspectiveProjection(float fovy, float aspect, float zNear, float zFar);
	void SetViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.f, 1.f, 0.f});
	void SetViewYXZ(glm::vec3 position, glm::vec3 rotation);
	void SetTargetViewYXZ(glm::vec3 targetPosition, glm::vec3 targetRotation);
	void UpdateView(float deltaTime);
	float EaseInOut(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }

	const glm::mat4& GetProjection() const { return m_projectionMatrix; }
	const glm::mat4& GetView() const { return m_viewMatrix; }
	const glm::mat4& GetInverseView() const { return m_inverseViewMatrix; }
	const glm::vec3 GetPosition() const { return glm::vec3(m_viewMatrix[3]); }

   private:
	glm::mat4 m_projectionMatrix{1.f};
	glm::mat4 m_viewMatrix{1.f};
	glm::mat4 m_inverseViewMatrix{1.f};

	glm::vec3 m_position{0.0f};         // Current position
	glm::vec3 m_rotation{0.0f};         // Current rotation
	glm::vec3 m_targetPosition{0.0f};   // Target position
	glm::vec3 m_targetRotation{0.0f};   // Target rotation
	float m_positionSmoothness = .000001f;  // Higher value = faster movement
	float m_rotationSmoothness = .000001f;  // Higher value = faster rotation
	bool m_initialized         = false;  

	float m_aspect = 1280.0f / 720.0f;
};
}  // namespace Rava