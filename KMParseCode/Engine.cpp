#include "KMParseCode/Engine.h"
#include "KMParseCode/RenderContext.h"
#include "glfw3.h"
#include "glfw3native.h"
#include <unordered_set>
#include <filesystem>

Tarsa* gMain = nullptr;
void OnWindowRefresh(GLFWwindow* window) {
	gMain->lastTime = glfwGetTime();
}

Tarsa::Tarsa() {
	mWindow = nullptr;
	mContext = nullptr;
}
#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
#import <AppKit/AppKit.h>
GLFWimage LoadIcon() {
	GLFWimage img = {};
    return img;
}
#elif defined(_WIN32)
WNDPROC gWndProc = nullptr;
GLFWimage LoadIcon() {
		GLFWimage img = {};
	HINSTANCE hInst = GetModuleHandle(NULL);
	HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APP_ICON));
	if (!hIcon) return img;

	ICONINFO iconInfo;
	GetIconInfo(hIcon, &iconInfo);

	BITMAP bmp;
	GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);

	img.width = bmp.bmWidth;
	img.height = bmp.bmHeight;
	img.pixels = new unsigned char[img.width * img.height * 4];
	HDC hdc = GetDC(NULL);
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bmp.bmWidth;
	bmi.bmiHeader.biHeight = -bmp.bmHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	GetDIBits(hdc, iconInfo.hbmColor, 0, bmp.bmHeight, img.pixels, &bmi, DIB_RGB_COLORS);
	ReleaseDC(NULL, hdc);

	for (int i = 0; i < img.width * img.height; i++) {
		unsigned char b = img.pixels[i * 4 + 0];
		unsigned char g = img.pixels[i * 4 + 1];
		unsigned char r = img.pixels[i * 4 + 2];
		unsigned char a = img.pixels[i * 4 + 3];
		img.pixels[i * 4 + 0] = r;
		img.pixels[i * 4 + 1] = g;
		img.pixels[i * 4 + 2] = b;
		img.pixels[i * 4 + 3] = a;
	}

	DeleteObject(iconInfo.hbmColor);
	DeleteObject(iconInfo.hbmMask);

	return img;
}
#endif
#if defined(_WIN32)
INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hDlg, 1001, gMain->mShowFogs ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, 1002, gMain->mShowPostEffects ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, 1003, gMain->mShowAnimations ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, 1004, gMain->mKCLView ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, 1005, gMain->mView ? BST_CHECKED : BST_UNCHECKED);

		SetDlgItemTextA(hDlg, 1006, gMain->mAutoAddPath.c_str());
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			gMain->mShowFogs = IsDlgButtonChecked(hDlg, 1001);
			gMain->mShowPostEffects = IsDlgButtonChecked(hDlg, 1002);
			gMain->mShowAnimations = IsDlgButtonChecked(hDlg, 1003);
			gMain->mKCLView = IsDlgButtonChecked(hDlg, 1004);
			gMain->mView = IsDlgButtonChecked(hDlg, 1005);

			char buf[512];
			GetDlgItemTextA(hDlg, 1006, buf, sizeof(buf));
			gMain->mAutoAddPath = buf;
			wchar_t buffer[MAX_PATH];
			GetModuleFileNameW(NULL, buffer, MAX_PATH);

			std::filesystem::path exePath(buffer);
			gMain->SaveConfig(exePath.parent_path().string() + "/options.txt");
			EndDialog(hDlg, IDOK);
			return TRUE;
		}

		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITMENUPOPUP:
	{
		bool loaded = gMain->mContext->loaded;

		EnableMenuItem(gMain->hFile, 2,
			loaded ? MF_ENABLED : MF_GRAYED);

		EnableMenuItem(gMain->hFile, 3,
			loaded ? MF_ENABLED : MF_GRAYED);

		EnableMenuItem(gMain->hFile, 4,
			loaded ? MF_ENABLED : MF_GRAYED);

		EnableMenuItem(gMain->hPreview, 6,
			loaded ? MF_ENABLED : MF_GRAYED);

		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case 1:
		{
			if (gMain->dirty)
			{
				int r = MessageBoxA(
					hwnd,
					"Do you want to save before closing?",
					"Unsaved Changes",
					MB_YESNOCANCEL | MB_ICONWARNING
				);

				if (r == IDCANCEL)
				{
					return 0;
				}

				if (r == IDYES)
				{
					std::filesystem::path p = gMain->mContext->mPath;
					std::string ext = p.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					if (ext == ".szs")
						gMain->mContext->SaveSZS(gMain->mContext->mPath);
					else if (ext == ".kmp")
						gMain->mContext->SaveKMP(gMain->mContext->mPath);

					gMain->dirty = false;
				}
			}
			EnableWindow(gMain->hwnd, FALSE);
			std::string path = gMain->mContext->OpenFileDialog();
			if (path != "")
				gMain->mContext->LoadFile(path);
			EnableWindow(gMain->hwnd, TRUE);
			return 0;
		}

		case 2:
		{
			if (gMain->mContext->mPath.empty())
				return 0;

			std::filesystem::path p = gMain->mContext->mPath;
			std::string ext = p.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			if (ext == ".szs") {
				gMain->mContext->SaveSZS(gMain->mContext->mPath);
			}
			else if (ext == ".kmp") {
				gMain->mContext->SaveKMP(gMain->mContext->mPath);
			}
			glfwSetWindowTitle(
				gMain->mWindow,
				("KMParse: Editing " + p.filename().string()).c_str()
			);
			gMain->dirty = false;
			return 0;
		}

		case 3:
		{
			std::filesystem::path p = std::filesystem::path(gMain->mContext->mPath);
			std::string ext = p.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			std::string savePath;

			if (ext == ".szs") {
				EnableWindow(gMain->hwnd, FALSE);
				savePath = gMain->mContext->SaveFileDialogSZS();
				if (!savePath.empty()) {
					gMain->mContext->SaveSZS(savePath);
					gMain->mContext->mPath = savePath;
				}
				glfwSetWindowTitle(
					gMain->mWindow,
					("KMParse: Editing " + std::filesystem::path(savePath).filename().string()).c_str()
				);
				gMain->dirty = false;
				EnableWindow(gMain->hwnd, TRUE);
			}
			else if (ext == ".kmp") {
				EnableWindow(gMain->hwnd, FALSE);
				savePath = gMain->mContext->SaveFileDialogKMP();
				if (!savePath.empty()) {
					gMain->mContext->SaveKMP(savePath);
					gMain->mContext->mPath = savePath;
				}
				glfwSetWindowTitle(
					gMain->mWindow,
					("KMParse: Editing " + std::filesystem::path(savePath).filename().string()).c_str()
				);
				gMain->dirty = false;
				EnableWindow(gMain->hwnd, TRUE);
			}
			return 0;
		}

		case 4:
			if (gMain->mContext->loaded)
			{
				if (gMain->dirty)
				{
					int r = MessageBoxA(
						hwnd,
						"Do you want to save before closing?",
						"Unsaved Changes",
						MB_YESNOCANCEL | MB_ICONWARNING
					);

					if (r == IDCANCEL)
					{
						return 0;
					}

					if (r == IDYES)
					{
						std::filesystem::path p = gMain->mContext->mPath;
						std::string ext = p.extension().string();
						std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

						if (ext == ".szs")
							gMain->mContext->SaveSZS(gMain->mContext->mPath);
						else if (ext == ".kmp")
							gMain->mContext->SaveKMP(gMain->mContext->mPath);
					}
					gMain->mContext->Reset();
					gMain->mContext->mPath = "";
					gMain->dirty = false;
				}
				else
				{
					gMain->mContext->Reset();
					gMain->mContext->mPath = "";
				}
			}
			return 0;
		case 5:
			gMain->mContext->loadCourseBrres();
			return 0;
		case 6:
			if(gMain->mAutoAddPath != "")
			gMain->mContext->errorCheck();
			return 0;
		case 7:
			DialogBoxParamA(
				GetModuleHandle(nullptr),
				MAKEINTRESOURCEA(IDD_KMPARSE_SETTINGS),
				hwnd,
				SettingsDlgProc,
				0
			);
			return 0;

		case 8:
			MessageBox(hwnd, TEXT("KMParse: Author, Marianne8559.\nBased on Astral-C's StarForge.\nRenderer based on NoClip.\nKMP Loader based on Lorenzi's KMP Editor.\n\nCoded with Copilot(GPT-5.1)."), TEXT("About"), MB_OK);

			return 0;
		}
	}

	return CallWindowProc(gWndProc, hwnd, msg, wParam, lParam);
}
#elif defined(__APPLE__)
@interface KMParseMenuHandler : NSObject <NSApplicationDelegate>
@end
@implementation KMParseMenuHandler


- (void)menuOpen {
	    if (gMain->dirty) {
        NSAlert* alert = [[NSAlert alloc] init];
        alert.messageText = @"Unsaved Changes";
        alert.informativeText = @"Do you want to save before closing?";
        [alert addButtonWithTitle:@"Save"];
        [alert addButtonWithTitle:@"Don't Save"];
        [alert addButtonWithTitle:@"Cancel"];

        NSModalResponse response = [alert runModal];

        if (response == NSAlertThirdButtonReturn) return;
        
        if (response == NSAlertFirstButtonReturn) {
            [self menuSave];
        }
    }
    std::string path = gMain->mContext->OpenFileDialog();
    if (!path.empty())
        gMain->mContext->LoadFile(path);
}

- (void)menuSave {
    if (!gMain->mContext->mPath.empty()) {
				std::filesystem::path p = gMain->mContext->mPath;
				std::string ext = p.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

				if (ext == ".szs")
					gMain->mContext->SaveSZS(gMain->mContext->mPath);
				else if (ext == ".kmp")
					gMain->mContext->SaveKMP(gMain->mContext->mPath);

			glfwSetWindowTitle(
				gMain->mWindow,
				("KMParse: Editing " + p.filename().string()).c_str()
			);
        gMain->dirty = false;
    }
}

- (void)menuSaveAs {
	std::string savePath;
					std::filesystem::path p = gMain->mContext->mPath;
				std::string ext = p.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
			if (ext == ".szs") {
				savePath = gMain->mContext->SaveFileDialogSZS();
				if (!savePath.empty()) {
					gMain->mContext->SaveSZS(savePath);
					gMain->mContext->mPath = savePath;
				}
				glfwSetWindowTitle(
					gMain->mWindow,
					("KMParse: Editing " + std::filesystem::path(savePath).filename().string()).c_str()
				);
				gMain->dirty = false;
			}
			else if (ext == ".kmp") {
				savePath = gMain->mContext->SaveFileDialogKMP();
				if (!savePath.empty()) {
					gMain->mContext->SaveKMP(savePath);
					gMain->mContext->mPath = savePath;
				}
				glfwSetWindowTitle(
					gMain->mWindow,
					("KMParse: Editing " + std::filesystem::path(savePath).filename().string()).c_str()
				);
				gMain->dirty = false;
			}
}

- (void)menuCloseFile {
    if (gMain->dirty) {
        NSAlert* alert = [[NSAlert alloc] init];
        alert.messageText = @"Unsaved Changes";
        alert.informativeText = @"Do you want to save before closing?";
        [alert addButtonWithTitle:@"Save"];
        [alert addButtonWithTitle:@"Don't Save"];
        [alert addButtonWithTitle:@"Cancel"];

        NSModalResponse response = [alert runModal];

        if (response == NSAlertThirdButtonReturn) return;
        
        if (response == NSAlertFirstButtonReturn) {
            [self menuSave];
        }
    }
    
    gMain->mContext->Reset();
    gMain->mContext->mPath = "";
    gMain->dirty = false;
    glfwSetWindowTitle(gMain->mWindow, "KMParse");
}

- (void)toggleSetting:(NSMenuItem*)sender flag:(bool*)flag {
    *flag = !(*flag);
    sender.state = (*flag ? NSControlStateValueOn : NSControlStateValueOff);
    gMain->SaveConfig("options.txt");
}

- (void)menuToggleFogs:(NSMenuItem*)sender { [self toggleSetting:sender flag:&gMain->mShowFogs]; }
- (void)menuTogglePostEffects:(NSMenuItem*)sender { [self toggleSetting:sender flag:&gMain->mShowPostEffects]; }
- (void)menuToggleAnimations:(NSMenuItem*)sender { [self toggleSetting:sender flag:&gMain->mShowAnimations]; }
- (void)menuToggleKCL:(NSMenuItem*)sender { [self toggleSetting:sender flag:&gMain->mKCLView]; }
- (void)menuToggleOrtho:(NSMenuItem*)sender { [self toggleSetting:sender flag:&gMain->mView]; }

- (BOOL)validateMenuItem : (NSMenuItem*)menuItem
{
	bool loaded = gMain->mContext->loaded;

	SEL action = menuItem.action;

	if (action == @selector(menuSave) ||
		action == @selector(menuSaveAs) ||
		action == @selector(menuCloseFile) ||
		action == @selector(menuErrorCheck))
	{
		return loaded;
	}

	return YES;
}

- (void)menuEditPath:(NSMenuItem*)sender {
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"auto_add Path Settings";
    [alert addButtonWithTitle:@"Apply"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField *input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 300, 24)];
    [input setStringValue:[NSString stringWithUTF8String:gMain->mAutoAddPath.c_str()]];
    alert.accessoryView = input;

    if ([alert runModal] == NSAlertFirstButtonReturn) {
        gMain->mAutoAddPath = [[input stringValue] UTF8String];
        gMain->SaveConfig("options.txt");
    }
}

- (void)menuAbout {
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"About KMParse";
    alert.informativeText = @"Author: Marianne8559.\nBased on Astral-C's StarForge.\nRenderer based on NoClip.\nKMP Loader based on Lorenzi's KMP Editor.\n\nCoded with Copilot(GPT-5.1).";
    [alert runModal];
}

- (void)menuErrorCheck {
	if (gMain->mAutoAddPath != "")
		gMain->mContext->errorCheck(); 
}

@end
void createCocoaMenu() {
    KMParseMenuHandler* handler = [[KMParseMenuHandler alloc] init];

    NSMenu* menubar = [[NSMenu alloc] init];
    [NSApp setMainMenu:menubar];

    NSMenuItem* appMenuItem = [[NSMenuItem alloc] initWithTitle:@"App" action:nil keyEquivalent:@""];
    [menubar addItem:appMenuItem];
    NSMenu* appMenu = [[NSMenu alloc] init];
    [appMenu addItemWithTitle:@"About" action:@selector(menuAbout) keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenuItem setSubmenu:appMenu];

    NSMenuItem* fileMenuItem = [[NSMenuItem alloc] initWithTitle:@"File" action:nil keyEquivalent:@""];
    [menubar addItem:fileMenuItem];

    NSMenu* fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
    [fileMenu addItemWithTitle:@"Open" action:@selector(menuOpen) keyEquivalent:@"o"];
    [fileMenu addItemWithTitle:@"Save" action:@selector(menuSave) keyEquivalent:@"s"];
    [fileMenu addItemWithTitle:@"Save As" action:@selector(menuSaveAs) keyEquivalent:@"S"];
    [fileMenu addItem:[NSMenuItem separatorItem]];
    [fileMenu addItemWithTitle:@"Close File" action:@selector(menuCloseFile) keyEquivalent:@"w"];
    [fileMenuItem setSubmenu:fileMenu];

    NSMenuItem* courseMenuItem = [[NSMenuItem alloc] initWithTitle:@"Course" action:nil keyEquivalent:@""];
    [menubar addItem:courseMenuItem];

    NSMenu* courseMenu = [[NSMenu alloc] initWithTitle:@"Course"];
	NSMenuItem* errorCheckItem =
		[courseMenu addItemWithTitle:@"Error Check"
		action:@selector(menuErrorCheck)
		keyEquivalent:@"e"];

		[courseMenuItem setSubmenu:courseMenu];

NSMenuItem* settingsMenuItem = [[NSMenuItem alloc] initWithTitle:@"Settings" action:nil keyEquivalent:@""];
[menubar addItem:settingsMenuItem];

NSMenu* settingsMenu = [[NSMenu alloc] initWithTitle:@"Settings"];

struct { NSString* title; SEL sel; bool val; } items[] = {
    {@"Show Fogs", @selector(menuToggleFogs:), gMain->mShowFogs},
    {@"Show Posteffects", @selector(menuTogglePostEffects:), gMain->mShowPostEffects},
    {@"Show Animations", @selector(menuToggleAnimations:), gMain->mShowAnimations},
    {@"Show Course Collisions", @selector(menuToggleKCL:), gMain->mKCLView},
    {@"Orthographic Projection", @selector(menuToggleOrtho:), gMain->mView}
};

for (int i = 0; i < 5; i++) {
    NSMenuItem* item = [settingsMenu addItemWithTitle:items[i].title 
                                               action:items[i].sel 
                                        keyEquivalent:@""];
    [item setState:(items[i].val ? NSControlStateValueOn : NSControlStateValueOff)];
    [item setTarget:handler];
}

[settingsMenu addItem:[NSMenuItem separatorItem]];

NSMenuItem* pathItem = [settingsMenu addItemWithTitle:@"Edit auto_add Path..." 
                                               action:@selector(menuEditPath:) 
                                        keyEquivalent:@""];
[pathItem setTarget:handler];

[settingsMenuItem setSubmenu:settingsMenu];

    for (NSMenuItem* mainItem in [menubar itemArray]) {
        if ([mainItem hasSubmenu]) {
            for (NSMenuItem* subItem in [[mainItem submenu] itemArray]) {
                [subItem setTarget:handler];
            }
        }
    }

    [NSApp setDelegate:handler];
}
#endif
bool Tarsa::parseBool(const std::string& s)
{
	std::string v = s;

	auto trim = [&](std::string& str) {
		str.erase(0, str.find_first_not_of(" \t\r\n"));
		str.erase(str.find_last_not_of(" \t\r\n") + 1);
	};
	trim(v);

	std::transform(v.begin(), v.end(), v.begin(), ::tolower);

	if (v == "true" || v == "1" || v == "on" || v == "yes")
		return true;
	if (v == "false" || v == "0" || v == "off" || v == "no")
		return false;

	return false;
}
void GLFWCursorEnterCallback(GLFWwindow* window, int entered) {
	gMain->mMouseActive = (entered == GLFW_TRUE);
}
bool Tarsa::LoadConfig(const std::string& path)
{
	std::unordered_set<std::string> loadedKeys;

	std::ifstream file(path);
	if (!file.is_open()) {
		InitConfig(path);
		return true;
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#')
			continue;

		auto pos = line.find('=');
		if (pos == std::string::npos)
			continue;

		std::string key = line.substr(0, pos);
		std::string value = line.substr(pos + 1);

		auto trim = [&](std::string& s) {
			s.erase(0, s.find_first_not_of(" \t"));
			s.erase(s.find_last_not_of(" \t") + 1);
		};
		trim(key);
		trim(value);

		if (!value.empty() && value.front() == '"' && value.back() == '"')
			value = value.substr(1, value.size() - 2);
		if (value.find_first_not_of('"') == std::string::npos)
			value = "";

		loadedKeys.insert(key);

		if (key == "windowWidth")      mWidth = std::stoi(value);
		if (key == "windowHeight")     mHeight = std::stoi(value);
		if (key == "showFogs")         mShowFogs = parseBool(value);
		if (key == "showPostEffects")  mShowPostEffects = parseBool(value);
		if (key == "showAnimations")  mShowAnimations = parseBool(value);
		if (key == "kclView")  mKCLView = parseBool(value);
		if (key == "view")  mView = parseBool(value);
		if (key == "autoAddPath")      mAutoAddPath = value;
	}

	bool needRewrite = false;

	auto ensureBool = [&](const std::string& key, bool& var, bool def) {
		if (!loadedKeys.count(key)) {
			var = def;
			needRewrite = true;
		}
	};

	auto ensureInt = [&](const std::string& key, int& var, int def) {
		if (!loadedKeys.count(key)) {
			var = def;
			needRewrite = true;
		}
	};

	auto ensureString = [&](const std::string& key, std::string& var, const std::string& def) {
		if (!loadedKeys.count(key)) {
			var = def;
			needRewrite = true;
		}
	};

	ensureInt("windowWidth", mWidth, 1280);
	ensureInt("windowHeight", mHeight, 720);
	ensureBool("showFogs", mShowFogs, false);
	ensureBool("showPostEffects", mShowPostEffects, true);
	ensureBool("showAnimations", mShowAnimations, true);
	ensureBool("kclView", mKCLView, false);
	ensureBool("view", mKCLView, false);
	ensureString("autoAddPath", mAutoAddPath, "");

	if (needRewrite)
		SaveConfig(path);

	return true;
}
bool Tarsa::SaveConfig(const std::string& path)
{
	std::ofstream file(path);
	if (!file.is_open())
		return false;

	file << "windowWidth = " << mWidth << "\n";
	file << "windowHeight = " << mHeight << "\n";
	file << "showFogs = " << (mShowFogs ? "true" : "false") << "\n";
	file << "showPostEffects = " << (mShowPostEffects ? "true" : "false") << "\n";
	file << "showAnimations = " << (mShowAnimations ? "true" : "false") << "\n";
	file << "kclView = " << (mKCLView ? "true" : "false") << "\n";
	file << "view = " << (mView ? "true" : "false") << "\n";
	file << "autoAddPath = \"" << mAutoAddPath << "\"\n";

	return true;
}
bool Tarsa::InitConfig(const std::string& path)
{
	mWidth = 1280;
	mHeight = 720;
	mShowFogs = false;
	mShowPostEffects = true;
	mShowAnimations = true;
	mKCLView = false;
	mView = false;
	mAutoAddPath = "";
	SaveConfig(path);

	return true;
}
void DropCallback(GLFWwindow* window, int count, const char** paths)
{
	#ifdef _WIN32
	if (gMain->dirty)
		{
			int r = MessageBoxA(
				gMain->hwnd,
				"Do you want to save before closing?",
				"Unsaved Changes",
				MB_YESNOCANCEL | MB_ICONWARNING
			);

			if (r == IDCANCEL)
			{
				return;
			}

			if (r == IDYES)
			{
				std::filesystem::path p = gMain->mContext->mPath;
				std::string ext = p.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

				if (ext == ".szs")
					gMain->mContext->SaveSZS(gMain->mContext->mPath);
				else if (ext == ".kmp")
					gMain->mContext->SaveKMP(gMain->mContext->mPath);

				gMain->dirty = false;
			}
			gMain->mContext->Reset();
			gMain->mContext->mPath = "";
		}
		#elif defined(__APPLE__)
	if (gMain->dirty) {
        NSAlert* alert = [[NSAlert alloc] init];
        alert.messageText = @"Unsaved Changes";
        alert.informativeText = @"Do you want to save before closing?";
        [alert addButtonWithTitle:@"Save"];
        [alert addButtonWithTitle:@"Don't Save"];
        [alert addButtonWithTitle:@"Cancel"];

        NSModalResponse response = [alert runModal];

        if (response == NSAlertThirdButtonReturn) return;
        
        if (response == NSAlertFirstButtonReturn) {
                if (!gMain->mContext->mPath.empty()) {
				std::filesystem::path p = gMain->mContext->mPath;
				std::string ext = p.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

				if (ext == ".szs")
					gMain->mContext->SaveSZS(gMain->mContext->mPath);
				else if (ext == ".kmp")
					gMain->mContext->SaveKMP(gMain->mContext->mPath);

        gMain->dirty = false;
    }
        }
	}
		#endif
		gMain->dirty = false;
	for (int i = 0; i < count; i++)
	{
		gMain->mContext->LoadFile(paths[i]);
		gMain->mContext->mPath = paths[i];
	}
}
int Tarsa::Start()
{
    gMain = this;

#ifdef __APPLE__
		std::string configPath = std::string(getenv("HOME")) + "/Library/Application Support/KMParse/options.txt";
        LoadConfig(configPath);
#else
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::filesystem::path exePath(buffer);
    LoadConfig(exePath.parent_path().string() + "/options.txt");
#endif
	if (!glfwInit()) {
		return 0;
	}
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
	if (mWidth < 640)mWidth = 640;
	if (mHeight < 360)mHeight = 360;
	mWindow = glfwCreateWindow(mWidth, mHeight, "KMParse", nullptr, nullptr);
	glfwSetWindowSizeLimits(mWindow, 640, 360, 7680, 4320);
	if (mWindow == nullptr) {
		glfwTerminate();
		return 0;
	}
	#if defined(_WIN32)
	hwnd = glfwGetWin32Window(mWindow);

	HMENU hMenu = CreateMenu();
	hFile = CreatePopupMenu();
	hPreview = CreatePopupMenu();
	HMENU hOther = CreatePopupMenu();

	AppendMenu(hFile, MF_STRING, 1, TEXT("Open"));
	AppendMenu(hFile, MF_STRING, 2, TEXT("Save"));
	AppendMenu(hFile, MF_STRING, 3, TEXT("Save As"));
	AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
	AppendMenu(hFile, MF_STRING, 4, TEXT("Close File"));

	AppendMenu(hPreview, MF_STRING, 6, TEXT("Error Check"));

	AppendMenu(hOther, MF_STRING, 7, TEXT("Settings"));
	AppendMenu(hOther, MF_STRING, 8, TEXT("About"));

	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFile, TEXT("File"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hPreview, TEXT("Course"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hOther, TEXT("Other"));
	SetMenu(hwnd, hMenu);
	DrawMenuBar(hwnd);
	gWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
	#elif defined(__APPLE__)
	createCocoaMenu();
	#endif
	Input::SetWindow(mWindow);
	#if defined(_WIN32)
	RECT rect;
	GetClientRect(hwnd, &rect);
	int cw = rect.right - rect.left;
	int ch = rect.bottom - rect.top;
	glfwSetWindowSize(mWindow, cw, ch);
	#endif
	glfwSetCursorEnterCallback(mWindow, GLFWCursorEnterCallback);
	glfwSetKeyCallback(mWindow, Input::GLFWKeyCallback);
	glfwSetCharCallback(mWindow, Input::GLFWCharCallback);
	glfwSetCursorPosCallback(mWindow, Input::GLFWMousePositionCallback);
	glfwSetMouseButtonCallback(mWindow, Input::GLFWMouseButtonCallback);
	glfwSetScrollCallback(mWindow, Input::GLFWMouseScrollCallback);
	glfwMakeContextCurrent(mWindow);
	glfwSetInputMode(mWindow, GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSwapInterval(1);
	glfwSetWindowRefreshCallback(mWindow, OnWindowRefresh);
	glfwSetDropCallback(mWindow, DropCallback);
	glfwSetWindowFocusCallback(mWindow, [](GLFWwindow* w, int focused) {
		if (focused) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
	});
glfwSetWindowCloseCallback(mWindow, [](GLFWwindow* window) {

#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);

    if (gMain->dirty)
    {
        int r = MessageBoxA(
            hwnd,
            "Do you want to save before closing?",
            "Unsaved Changes",
            MB_YESNOCANCEL | MB_ICONWARNING
        );

        if (r == IDCANCEL) {
            glfwSetWindowShouldClose(window, GLFW_FALSE);
            return;
        }

        if (r == IDYES) {
            std::filesystem::path p = gMain->mContext->mPath;
            std::string ext = p.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".szs")
                gMain->mContext->SaveSZS(gMain->mContext->mPath);
            else if (ext == ".kmp")
                gMain->mContext->SaveKMP(gMain->mContext->mPath);

            gMain->dirty = false;
        }
    }

#else

    if (gMain->dirty)
    {
        @autoreleasepool {
            NSAlert* alert = [[NSAlert alloc] init];
            alert.messageText = @"Unsaved Changes";
            alert.informativeText = @"Do you want to save before closing?";
            [alert addButtonWithTitle:@"Save"];
            [alert addButtonWithTitle:@"Don't Save"];
            [alert addButtonWithTitle:@"Cancel"];

            NSModalResponse response = [alert runModal];

            if (response == NSAlertThirdButtonReturn) {
                glfwSetWindowShouldClose(window, GLFW_FALSE);
                return;
            }

            if (response == NSAlertFirstButtonReturn) {
                std::filesystem::path p = gMain->mContext->mPath;
                std::string ext = p.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (ext == ".szs")
                    gMain->mContext->SaveSZS(gMain->mContext->mPath);
                else if (ext == ".kmp")
                    gMain->mContext->SaveKMP(gMain->mContext->mPath);

                gMain->dirty = false;
            }
        }
    }

#endif

    glfwSetWindowShouldClose(window, GLFW_TRUE);
});


	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		return -1;
	}
	glFrontFace(GL_CW);

	mContext = new RenderContext();
	glEnable(GL_MULTISAMPLE);
	fixedDt = 1.0f / 60.0f;
		#if defined(_WIN32)
	GLFWimage images[1];
	images[0] = LoadIcon();
	glfwSetWindowIcon(mWindow, 1, images);
	#endif
	while (!glfwWindowShouldClose(mWindow)) {
		glfwPollEvents();

		float currentTime = glfwGetTime();
		float frameTime = currentTime - lastTime;
		lastTime = currentTime;
		accumulator += frameTime;
		while (accumulator >= fixedDt) {
			bool ctrl = (Input::GetKey(GLFW_KEY_LEFT_CONTROL) || Input::GetKey(GLFW_KEY_RIGHT_CONTROL)) || (Input::GetKey(GLFW_KEY_LEFT_SUPER) || Input::GetKey(GLFW_KEY_RIGHT_SUPER));
			bool shift = Input::GetKey(GLFW_KEY_LEFT_SHIFT) || Input::GetKey(GLFW_KEY_RIGHT_SHIFT);

			if (ctrl && Input::GetKeyDown(GLFW_KEY_Z) && !shift)
			{
				mContext->undo();
			}
#ifdef _WIN32
			if (ctrl && Input::GetKeyDown(GLFW_KEY_S) && !gMain->mContext->mPath.empty()&& !shift)
			{
				std::filesystem::path p = gMain->mContext->mPath;
					std::string ext = p.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					if (ext == ".szs")
						gMain->mContext->SaveSZS(gMain->mContext->mPath);
					else if (ext == ".kmp")
						gMain->mContext->SaveKMP(gMain->mContext->mPath);

					gMain->dirty = false;
			}

			if (ctrl && Input::GetKeyDown(GLFW_KEY_S) && !gMain->mContext->mPath.empty()&& shift)
			{
			std::filesystem::path p = std::filesystem::path(gMain->mContext->mPath);
			std::string ext = p.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			std::string savePath;

			if (ext == ".szs") {
				EnableWindow(gMain->hwnd, FALSE);
				savePath = gMain->mContext->SaveFileDialogSZS();
				if (!savePath.empty()) {
					gMain->mContext->SaveSZS(savePath);
					gMain->mContext->mPath = savePath;
				}
				glfwSetWindowTitle(
					gMain->mWindow,
					("KMParse: Editing " + std::filesystem::path(savePath).filename().string()).c_str()
				);
				gMain->dirty = false;
				EnableWindow(gMain->hwnd, TRUE);
			}
			else if (ext == ".kmp") {
				EnableWindow(gMain->hwnd, FALSE);
				savePath = gMain->mContext->SaveFileDialogKMP();
				if (!savePath.empty()) {
					gMain->mContext->SaveKMP(savePath);
					gMain->mContext->mPath = savePath;
				}
				glfwSetWindowTitle(
					gMain->mWindow,
					("KMParse: Editing " + std::filesystem::path(savePath).filename().string()).c_str()
				);
				gMain->dirty = false;
				EnableWindow(gMain->hwnd, TRUE);
			}
			}
			#endif
			

			if (ctrl && Input::GetKeyDown(GLFW_KEY_Z) && shift)
			{
				mContext->redo();
			}
			if (Input::GetKeyDown(GLFW_KEY_F1))
				mShowFogs ? mShowFogs = false: mShowFogs = true;
			if (Input::GetKeyDown(GLFW_KEY_F2))
				mShowPostEffects ? mShowPostEffects = false : mShowPostEffects = true;
			if (Input::GetKeyDown(GLFW_KEY_F3))
				mShowAnimations ? mShowAnimations = false : mShowAnimations = true;
			if (Input::GetKeyDown(GLFW_KEY_F4))
				mKCLView ? mKCLView = false : mKCLView = true;
			if (Input::GetKeyDown(GLFW_KEY_F5))
				mView ? mView = false : mView = true;

			mContext->Update(fixedDt);
			Input::UpdateInputState();
			accumulator -= fixedDt;
		}
		float alpha = accumulator / fixedDt;
		Render(alpha);
	}
	delete mContext;
	mContext = nullptr;

#ifdef _WIN32
    GetWindowRect(hwnd, &rect);
    mWidth  = rect.right  - rect.left;
    mHeight = rect.bottom - rect.top;
#else
    int w, h;
    glfwGetWindowSize(mWindow, &w, &h);
    mWidth = w;
    mHeight = h;
#endif

glfwMakeContextCurrent(nullptr);

if (mWindow) {
    glfwDestroyWindow(mWindow);
    mWindow = nullptr;
}

glfwTerminate();

#ifdef __APPLE__
@autoreleasepool {
	std::string configPath = std::string(getenv("HOME")) + "/Library/Application Support/KMParse/options.txt";
    SaveConfig(configPath);
}
#else
SaveConfig(exePath.parent_path().string() + "/options.txt");
#endif

}
bool Tarsa::Render(float deltaTime)
{
	if (!mContext || !mWindow || glfwWindowShouldClose(mWindow))
		return false;
	glfwMakeContextCurrent(mWindow);
	mContext->Render(mWindow, deltaTime);
	glfwSwapBuffers(mWindow);
	return true;
}