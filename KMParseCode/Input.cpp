#include "KMParseCode/Input.h"
#include "KMParseCode/Engine.h"
#include "KMParseCode/RenderContext.h"

namespace Input {
	// Anonymous namespace within UInput allows us to have private-scope variables that are only accessible from this TU.
	// This allows us to hide direct access to the input state variables and only allow access via the Get functions.
	namespace {
		constexpr uint32_t KEY_MAX = 512;
		constexpr uint32_t MOUSE_BUTTON_MAX = 3;
		GLFWwindow* mWindow = nullptr;

		glm::vec2 mMousePosition;
		glm::vec2 mMouseDelta;
		int32_t mMouseScrollDelta;

		bool mKeysDown[KEY_MAX];
		bool mPrevKeysDown[KEY_MAX];

		bool mMouseButtonsDown[MOUSE_BUTTON_MAX];
		bool mPrevMouseButtonsDown[MOUSE_BUTTON_MAX];
		glm::vec2 mPrevMousePosition;

		void SetKeyboardState(uint32_t key, bool pressed) {
			mKeysDown[key] = pressed;
		}

		void SetMouseState(uint32_t button, bool pressed) {
			mMouseButtonsDown[button] = pressed;
		}

void SetMousePosition(uint32_t x, uint32_t y) {
    if (mWindow == nullptr) return;

    int winWidth, winHeight;
    int fbWidth, fbHeight;
    
    glfwGetWindowSize(mWindow, &winWidth, &winHeight);

    glfwGetFramebufferSize(mWindow, &fbWidth, &fbHeight);

    x = std::clamp<int>(x, 0, winWidth);
    y = std::clamp<int>(y, 0, winHeight);

    float scaleX = (float)fbWidth / (float)winWidth;
    float scaleY = (float)fbHeight / (float)winHeight;

    mMousePosition = glm::vec2((float)x * scaleX, (float)y * scaleY);
}


		void SetMouseScrollDelta(uint32_t delta) {
			mMouseScrollDelta = delta;
		}
	}
}

void Input::SetWindow(GLFWwindow* window) {
	mWindow = window;
}


bool Input::GetKey(uint32_t key) {
	return mKeysDown[key];
}

bool Input::GetKeyDown(uint32_t key) {
	return mKeysDown[key] && !mPrevKeysDown[key];
}

bool Input::GetKeyUp(uint32_t key) {
	return mPrevKeysDown[key] && !mKeysDown[key];
}

bool Input::GetMouseButton(uint32_t button) {
	return mMouseButtonsDown[button];
}

bool Input::GetMouseButtonDown(uint32_t button) {
	return mMouseButtonsDown[button] && !mPrevMouseButtonsDown[button];
}

bool Input::GetMouseButtonUp(uint32_t button) {
	return mPrevMouseButtonsDown[button] && !mMouseButtonsDown[button];
}

glm::vec2 Input::GetMousePosition() {
	return mMousePosition;
}

glm::vec2 Input::GetMouseDelta() {
	return mMouseDelta;
}

int32_t Input::GetMouseScrollDelta() {
	return mMouseScrollDelta;
}

void Input::UpdateInputState() {
	for (int i = 0; i < KEY_MAX; i++)
		mPrevKeysDown[i] = mKeysDown[i];
	for (int i = 0; i < MOUSE_BUTTON_MAX; i++)
		mPrevMouseButtonsDown[i] = mMouseButtonsDown[i];

	mMouseDelta = mMousePosition - mPrevMousePosition;
	mPrevMousePosition = mMousePosition;
	mMouseScrollDelta = 0;
}

void Input::GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key >= KEY_MAX)
		return;
	gMain->mContext->HandleKey(key);
	if (action == GLFW_PRESS)
		SetKeyboardState(key, true);
	else if (action == GLFW_RELEASE)
		SetKeyboardState(key, false);
}
void Input::GLFWCharCallback(GLFWwindow* window, unsigned int codepoint) {
    gMain->mContext->HandleChar((char)codepoint);
}
void Input::ForceMousePosition(float x, float y)
{
	mMousePosition = glm::vec2(x, y);
	mPrevMousePosition = mMousePosition;
}
void Input::ResetMouseDelta()
{
	mMouseDelta = glm::vec2(0, 0);
	mPrevMousePosition = mMousePosition;
}

void Input::GLFWMousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
	SetMousePosition(xpos, ypos);
}

void Input::GLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button >= MOUSE_BUTTON_MAX)
		return;

	if (action == GLFW_PRESS)
		SetMouseState(button, true);
	else if (action == GLFW_RELEASE)
		SetMouseState(button, false);
}

void Input::GLFWMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	gMain->mContext->listScroll -= (float)yoffset * 20.0f;
	if (gMain->mContext->listScroll < 0.0f)
		gMain->mContext->listScroll = 0.0f;
	SetMouseScrollDelta(yoffset);
}