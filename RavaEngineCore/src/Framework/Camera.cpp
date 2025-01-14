#include "ravapch.h"

#include "Framework/Camera.h"
#include "Framework/Timestep.h"

namespace Rava {
Camera::Camera() {
	SetPerspectiveProjection(glm::radians(50.f), m_aspect, 0.1f, 100.f);
}

void Camera::RecalculateProjection() {
	if (m_projectionType == ProjectionType::Perspective) {
		SetPerspectiveProjection(m_perspectiveFOV, m_aspect, m_perspectiveNear, m_perspectiveFar);
	} else {
		float orthoLeft   = -m_orthographicSize * m_aspect * 0.5f;
		float orthoRight  = m_orthographicSize * m_aspect * 0.5f;
		float orthoTop    = m_orthographicSize * 0.5f;
		float orthoBottom = -m_orthographicSize * 0.5f;

		SetOrthographicProjection3D(orthoLeft, orthoRight, orthoTop, orthoBottom, m_orthographicNear, m_orthographicFar);
	}
}

void Camera::UpdateView() {
	if (!m_initialized) {
		return;
	}
	m_position = glm::mix(m_position, m_targetPosition, m_positionSmoothness * Timestep::Count());
	m_rotation = glm::mix(m_rotation, m_targetRotation, m_rotationSmoothness * Timestep::Count());

	SetViewYXZ(m_position, m_rotation);
}

void Camera::MoveCamera(glm::vec3 position, glm::vec3 rotation) {
	if (m_isSmoothTranslate) {
		SetTargetViewYXZ(position, rotation);
		UpdateView();
	} else {
		SetViewYXZ(position, rotation);
	}
}

void Rava::Camera::SetProjectionType(ProjectionType type) {
	m_projectionType = type;
	RecalculateProjection();
}

void Camera::SetPerspectiveProjection(float fovy, float aspect, float zNear, float zFar) {
	m_perspectiveFOV  = fovy;
	m_aspect          = aspect;
	m_perspectiveNear = zNear;
	m_perspectiveFar  = zFar;

	ENGINE_ASSERT(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
	m_projectionMatrix = glm::perspective(m_perspectiveFOV, m_aspect, m_perspectiveNear, m_perspectiveFar);
}

void Camera::SetOrthographicProjection(float left, float right, float top, float bottom, float zNear, float zFar) {
	m_projectionMatrix       = glm::mat4{1.0f};
	m_projectionMatrix[0][0] = 2.f / (right - left);
	m_projectionMatrix[1][1] = 2.f / (bottom - top);
	m_projectionMatrix[2][2] = 1.f / (zFar - zNear);
	m_projectionMatrix[3][0] = -(right + left) / (right - left);
	m_projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
	m_projectionMatrix[3][2] = -zNear / (zFar - zNear);
}

void Camera::SetOrthographicProjection3D(float left, float right, float top, float bottom, float zNear, float zFar) {
	m_projectionMatrix = glm::ortho(-left, -right, bottom, top, zNear, zFar);
}

void Camera::SetViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
	m_viewMatrix        = glm::lookAt(position, target, up);
	m_inverseViewMatrix = glm::inverse(m_viewMatrix);
}

void Camera::SetViewYXZ(glm::vec3 position, glm::vec3 rotation) {
	const float c3 = glm::cos(rotation.z);
	const float s3 = glm::sin(rotation.z);
	const float c2 = glm::cos(rotation.x);
	const float s2 = glm::sin(rotation.x);
	const float c1 = glm::cos(rotation.y);
	const float s1 = glm::sin(rotation.y);
	const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
	const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
	const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
	m_viewMatrix       = glm::mat4{1.f};
	m_viewMatrix[0][0] = u.x;
	m_viewMatrix[1][0] = u.y;
	m_viewMatrix[2][0] = u.z;
	m_viewMatrix[0][1] = v.x;
	m_viewMatrix[1][1] = v.y;
	m_viewMatrix[2][1] = v.z;
	m_viewMatrix[0][2] = w.x;
	m_viewMatrix[1][2] = w.y;
	m_viewMatrix[2][2] = w.z;
	m_viewMatrix[3][0] = -glm::dot(u, position);
	m_viewMatrix[3][1] = -glm::dot(v, position);
	m_viewMatrix[3][2] = -glm::dot(w, position);

	m_inverseViewMatrix       = glm::mat4{1.f};
	m_inverseViewMatrix[0][0] = u.x;
	m_inverseViewMatrix[0][1] = u.y;
	m_inverseViewMatrix[0][2] = u.z;
	m_inverseViewMatrix[1][0] = v.x;
	m_inverseViewMatrix[1][1] = v.y;
	m_inverseViewMatrix[1][2] = v.z;
	m_inverseViewMatrix[2][0] = w.x;
	m_inverseViewMatrix[2][1] = w.y;
	m_inverseViewMatrix[2][2] = w.z;
	m_inverseViewMatrix[3][0] = position.x;
	m_inverseViewMatrix[3][1] = position.y;
	m_inverseViewMatrix[3][2] = position.z;
}

void Camera::SetTargetViewYXZ(glm::vec3 targetPosition, glm::vec3 targetRotation) {
	if (!m_initialized) {
		m_position    = targetPosition;
		m_rotation    = targetRotation;
		m_initialized = true;
	}

	m_targetPosition = targetPosition;
	m_targetRotation = targetRotation;
}
}  // namespace Rava
