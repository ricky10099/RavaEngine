#pragma once

namespace Rava {
class Camera {
   public:
	enum class ProjectionType {
		Perspective  = 0,
		Orthographic = 1
	};

   public:
	Camera();
	~Camera() = default;

	void RecalculateProjection();
	void UpdateView();
	void MoveCamera(glm::vec3 position, glm::vec3 rotation);

	void SetProjectionType(ProjectionType type);
	void SetPerspectiveProjection(float fovy, float aspect, float zNear, float zFar);
	void SetOrthographicProjection(float left, float right, float top, float bottom, float zNear, float zFar);
	void SetOrthographicProjection3D(float left, float right, float top, float bottom, float zNear, float zFar);
	void SetViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.f, 1.f, 0.f});
	void SetViewYXZ(glm::vec3 position, glm::vec3 rotation);
	void SetTargetViewYXZ(glm::vec3 targetPosition, glm::vec3 targetRotation);
	void SetPerspectiveVerticalFOV(float fov) { m_perspectiveFOV = fov; }
	void SetPerspectiveNearClip(float zNear) { m_perspectiveFOV = zNear; }
	void SetPerspectiveFarClip(float zFar) { m_perspectiveFOV = zFar; }
	void SetOrthographicSize(float size) { m_orthographicSize = size; }
	void SetOrthographicNearClip(float zNear) { m_orthographicNear = zNear; }
	void SetOrthographicFarClip(float zFar) { m_orthographicFar = zFar; }
	void SetSmoothTranslate(bool isSmooth) { m_isSmoothTranslate = isSmooth; }

	float EaseInOut(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }
	ProjectionType GetProjectionType() const { return m_projectionType; }
	float GetPerspectiveVerticalFOV() const { return m_perspectiveFOV; }
	float GetPerspectiveNearClip() const { return m_perspectiveNear; }
	float GetPerspectiveFarClip() const { return m_perspectiveFar; }
	float GetOrthographicSize() const { return m_orthographicSize; }
	float GetOrthographicNearClip() const { return m_orthographicNear; }
	float GetOrthographicFarClip() const { return m_orthographicFar; }
	bool GetIsSmoothTranslate() const { return m_isSmoothTranslate; }
	const glm::mat4& GetProjection() const { return m_projectionMatrix; }
	const glm::mat4& GetView() const { return m_viewMatrix; }
	const glm::mat4& GetInverseView() const { return m_inverseViewMatrix; }
	const glm::vec3 GetPosition() const { return glm::vec3(m_viewMatrix[3]); }

   private:
	ProjectionType m_projectionType = ProjectionType::Perspective;
	glm::mat4 m_projectionMatrix{1.f};
	glm::mat4 m_viewMatrix{1.f};
	glm::mat4 m_inverseViewMatrix{1.f};

	float m_aspect = 16.0f / 9.0f;

	float m_perspectiveFOV  = glm::radians(45.0f);
	float m_perspectiveNear = 0.01f;
	float m_perspectiveFar  = 1000.0f;

	float m_orthographicSize = 10.0f;
	float m_orthographicNear = -1.0f;
	float m_orthographicFar  = 1.0f;

	float m_zNear      = 0.01f;
	float m_zFar       = 100.0f;
	float m_zoomFactor = 1.0f;

	bool m_isSmoothTranslate = true;
	glm::vec3 m_position{0.0f};         // Current position
	glm::vec3 m_rotation{0.0f};         // Current rotation
	glm::vec3 m_targetPosition{0.0f};   // Target position
	glm::vec3 m_targetRotation{0.0f};   // Target rotation
	float m_positionSmoothness = 3.0f;  // Higher value = faster movement
	float m_rotationSmoothness = 3.0f;  // Higher value = faster rotation
	bool m_initialized         = false;

   private:
};
}  // namespace Rava