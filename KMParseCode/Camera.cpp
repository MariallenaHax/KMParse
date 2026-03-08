#include "KMParseCode/Camera.h"
#include "KMParseCode/Input.h"

#include <iostream>
#include <algorithm>
#include <glfw3.h>

SceneCamera::SceneCamera() : mNearPlane(100.0f), mFarPlane(1000000.0f), mFovy(glm::radians(60.f)),
mCenter(ZERO), mEye(ZERO), mPitch(0.f), mYaw(glm::half_pi<float>()), mUp(UNIT_Y), mRight(UNIT_X), mForward(UNIT_Z),
mAspectRatio(16.f / 9.f), mMoveSpeed(1000.f), mMouseSensitivity(0.25f), mIsOrtho(false), mWinWidth(1280), mWinHeight(720), mOrthoZoom(1.0f)
{
	mCenter = mEye - mForward;
	mOrthoZoom = 0.008f;
}

void SceneCamera::Update(float deltaTime) {
	if (Input::GetKey(GLFW_KEY_LEFT_CONTROL) || Input::GetKey(GLFW_KEY_RIGHT_CONTROL)) return;
	glm::vec3 moveDir = glm::zero<glm::vec3>();

	if (Input::GetKey(GLFW_KEY_W))
		!mIsOrtho ? moveDir -= mForward : moveDir += mUp;
	if (Input::GetKey(GLFW_KEY_S))
		!mIsOrtho ? moveDir += mForward : moveDir -= mUp;
	if (Input::GetKey(GLFW_KEY_D))
		moveDir -= mRight;
	if (Input::GetKey(GLFW_KEY_A))
		moveDir += mRight;

	if (Input::GetKey(GLFW_KEY_Q))
		!mIsOrtho ? moveDir -= mUp : moveDir += mForward;
	if (Input::GetKey(GLFW_KEY_E))
		!mIsOrtho ? moveDir += mUp : moveDir -= mForward;

	mMoveSpeed += Input::GetMouseScrollDelta() * 100 * deltaTime;
	mMoveSpeed = std::clamp(mMoveSpeed, 100.f, 50000.f);
	float actualMoveSpeed = Input::GetKey(GLFW_KEY_LEFT_SHIFT) ? mMoveSpeed * 50.f : mMoveSpeed * 10.f;

	if (Input::GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT))
		Rotate(deltaTime, Input::GetMouseDelta());

	if (glm::length(moveDir) != 0.f)
		moveDir = glm::normalize(moveDir);

	mEye += moveDir * (actualMoveSpeed * deltaTime);
	mCenter = mEye - mForward;

	float delta = std::clamp((float)Input::GetMouseScrollDelta(), -0.1f, 0.1f);
	mOrthoZoom += delta * 0.01f;
	mOrthoZoom = std::clamp(mOrthoZoom, 0.001f, 0.1f);
}

void SceneCamera::Rotate(float deltaTime, glm::vec2 mouseDelta) {
	if (mouseDelta.x == 0.f && mouseDelta.y == 0.f)
		return;

	mPitch += mouseDelta.y * deltaTime * mMouseSensitivity;
	mYaw += mouseDelta.x * deltaTime * mMouseSensitivity;

	mPitch = std::clamp(mPitch, LOOK_UP_MIN, LOOK_UP_MAX);

	mForward.x = cos(mYaw) * cos(mPitch);
	mForward.y = sin(mPitch);
	mForward.z = sin(mYaw) * cos(mPitch);

	mForward = glm::normalize(mForward);

	mRight = glm::normalize(glm::cross(mForward, UNIT_Y));
	mUp = glm::normalize(glm::cross(mRight, mForward));

	mCenter = mEye - mForward;
}