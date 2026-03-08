#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GLFWwindow;

constexpr glm::vec3 ZERO = glm::vec3(0.0f, 0.0f, 0.0f);
constexpr glm::vec3 UNIT_X = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 UNIT_Y = glm::vec3(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 UNIT_Z = glm::vec3(0.0f, 0.0f, 1.0f);

constexpr float LOOK_UP_MIN = -glm::half_pi<float>() + glm::epsilon<float>();
constexpr float LOOK_UP_MAX = glm::half_pi<float>() - glm::epsilon<float>();

class SceneCamera {
	glm::vec3 mEye;
	glm::vec3 mCenter;

	bool mIsOrtho;
	float mPitch;
	float mYaw;
	glm::vec3 mForward;
	glm::vec3 mRight;
	glm::vec3 mUp;

	float mNearPlane;
	float mFarPlane;
	float mFovy;
	float mAspectRatio;

	float mMoveSpeed;
	float mMouseSensitivity;
	int mWinWidth, mWinHeight;
	float mOrthoZoom;

	void Rotate(float deltaTime, glm::vec2 mouseDelta);

public:
	SceneCamera();
	~SceneCamera() {}

	void Update(float deltaTime);

	bool GetIsOrtho() { return mIsOrtho; }
	void SetOrtho(bool ortho) { mIsOrtho = ortho; }
	void SetWindowSize(int w, int h) { mWinWidth = w; mWinHeight = h; }
	void SetForward(glm::vec3 forward) { mForward = forward; }
	void SetUp(glm::vec3 up) { mUp = up; }
	void SetPosition(glm::vec3 eye) { mEye = eye;}
	glm::vec3 GetPosition() { return mEye; }
	glm::vec3 GetForward() { return mForward; }
	glm::mat4 GetViewMatrix() { return glm::lookAt(mEye, mCenter, mUp); }
	glm::mat4 GetProjectionMatrix() { if (!mIsOrtho) { return glm::perspective(mFovy, mAspectRatio, mNearPlane, mFarPlane); } else { return glm::ortho(-((float)mWinWidth * 0.5f / mOrthoZoom), (float)mWinWidth * 0.5f / mOrthoZoom, -((float)mWinHeight * 0.5f / mOrthoZoom), (float)mWinHeight * 0.5f / mOrthoZoom, mNearPlane, mFarPlane); } }
};