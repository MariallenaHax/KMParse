#pragma once
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#endif
#include "Input.h"
#include <string>
#include "../resource.h"
class Tarsa {
public:
	Tarsa();
	float fixedDt;
	bool parseBool(const std::string& s);
	bool LoadConfig(const std::string& path);
	bool SaveConfig(const std::string& path);
	bool InitConfig(const std::string& path);
	int Start();
	int mWidth;
	int mHeight;
	bool mShowFogsTemp;
	bool mShowPostEffectsTemp;
	bool mShowAnimationsTemp;
	bool mKCLViewTemp;
	bool mViewTemp;
	std::string mAutoAddPathTemp;
	#ifdef _WIN32
	HMENU hFile;
	HMENU hPreview;
	HFONT hFont;
	HWND hwnd;
	HWND configWnd;
	HWND chkFogs;
	HWND chkPostEffects;
	HWND chkAnimations;
	HWND chkKCLView;
	HWND chkView;
	HWND editAutoAddPathLabel;
	HWND editAutoAddPath;
	HWND btnSave;
	HWND btnCancel;
	HINSTANCE hInst;
	#endif
	bool mShowFogs;
	bool mShowPostEffects;
	bool mShowAnimations;
	bool mKCLView;
	bool mView;
	std::string mAutoAddPath;
	bool Render(float deltaTime);
	struct GLFWwindow* mWindow;
	class RenderContext* mContext;
	float accumulator = 0.0f;
	bool mMouseActive = false;
	float lastTime;
	bool dirty = false;
};
extern Tarsa* gMain;