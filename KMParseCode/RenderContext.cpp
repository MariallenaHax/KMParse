#include "KMParseCode/RenderContext.h"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#endif
#if defined(__APPLE__)
#import <Appkit/Appkit.h>
#import <Foundation/Foundation.h>
#endif
#include "glad/glad.h"
#include "nw4r/brres.h"
#include "gctoolsplusplus/Bti.hpp"
#include "KMParseCode/Engine.h"
#include "../src/discord/discord.h"

constexpr discord::ClientId ClientID = 1476903249612378162;
discord::Core * core_raw{};
auto result = discord::Core::Create(ClientID, DiscordCreateFlags_Default, &core_raw);
std::unique_ptr<discord::Core> core(core_raw);
std::atomic<bool> running = true;
int RenderContext::UIItem::nextId = 0;
std::vector<std::string> RenderContext::cameraList = { "None" };
std::vector<std::string> RenderContext::cannonList = {};
std::vector<std::string> RenderContext::routeList = { "None" };
std::vector<std::string> RenderContext::fogList = { "None" };
void RenderContext::SetActivity(const std::string& details, const std::string& state)
{
	discord::Activity activity{};
	activity.SetState(state.c_str());
	activity.SetDetails(details.c_str());
	activity.SetType(discord::ActivityType::Playing);
	core->ActivityManager().UpdateActivity(activity, nullptr);
}
RenderContext::~RenderContext() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glDeleteFramebuffers(1, &mFbo);
	glDeleteRenderbuffers(1, &mRbo);
	glDeleteTextures(1, &mColorTex);
	if(loaded)brresRenderer.clearinstance();
	mRbo = 0;
	mFbo = 0;
	mColorTex = 0;
	kclRenderer.Reset();
	brresRenderer.clearinstance();
	for (auto& kv : shaderCache)
		glDeleteProgram(kv.second);

	shaderCache.clear();
}
void RenderContext::InitFbo(int width, int height)
{
	glGenFramebuffers(1, &mFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

	glGenTextures(1, &mColorTex);
	glBindTexture(GL_TEXTURE_2D, mColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, mColorTex, 0);

	glGenTextures(1, &brresRenderer.zbufferTex);
	glBindTexture(GL_TEXTURE_2D, brresRenderer.zbufferTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
		width, height, 0,
		GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, brresRenderer.zbufferTex, 0);

	{
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, bufs);
	}
	posteffect2.InitFbo(width, height);
}
void RenderContext::SaveSZS(std::string path)
{
	auto newKmpFile = archive->Get<Archive::File>("course.kmp");
	if(!newKmpFile)
		newKmpFile = archive->Get<Archive::File>("./course.kmp");
	uint32_t newSize = kmpLoader.CalculateKMPSize(loadedKMP);
	std::vector<uint8_t> buffer(newSize);

	bStream::CMemoryStream stream(buffer.data(), buffer.size(),
		bStream::Endianess::Big, bStream::OpenMode::Out);

	kmpLoader.saveKMP(loadedKMP, &stream);
	newKmpFile->SetData(buffer.data(), buffer.size());

	archive->SaveToFile(path, Compression::Format::YAZ0);
}
void RenderContext::SaveKMP(std::string path)
{
	bStream::CFileStream stream(path, bStream::Endianess::Big, bStream::OpenMode::Out);
	kmpLoader.saveKMP(loadedKMP, &stream);
}
std::string RenderContext::OpenFileDialog()
{
	#ifdef __APPLE__
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        panel.allowedFileTypes = @[@"szs", @"kmp"];

        if ([panel runModal] == NSModalResponseOK) {
            NSString* nsPath = panel.URL.path;
            std::string path([nsPath UTF8String]);
            mPath = path;
            return path;
        }
        mPath = "";
        return "";
    }
#else
	OPENFILENAMEW ofn;
	wchar_t fileName[MAX_PATH] = L"";

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = gMain->hwnd;
	ofn.lpstrFilter =
		L"MKWii Course Archive / MKWii Course Settings (*.szs;*.kmp)\0*.szs;*.kmp\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	if (GetOpenFileNameW(&ofn))
	{
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, fileName, -1, NULL, 0, NULL, NULL);
		std::string path(size_needed - 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, fileName, -1, &path[0], size_needed, NULL, NULL);
		mPath = path;
		return path;
	}
	mPath = "";
	return "";
	#endif
}

std::string RenderContext::SaveFileDialogSZS()
{
#ifdef __APPLE__
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.allowedFileTypes = @[@"szs"];
        panel.canCreateDirectories = YES;

        if ([panel runModal] == NSModalResponseOK) {
            NSString* nsPath = panel.URL.path;
            return std::string([nsPath UTF8String]);
        }
        return "";
    }
#else

	OPENFILENAMEW ofn{};
	wchar_t fileName[MAX_PATH] = L"";

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = gMain->hwnd;
	ofn.lpstrFilter =
		L"MKWii Course Archive (*.szs)\0*.szs\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	if (GetSaveFileNameW(&ofn))
	{
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, fileName, -1, NULL, 0, NULL, NULL);
		std::string path(size_needed - 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, fileName, -1, path.data(), size_needed, NULL, NULL);
		return path;
	}

	return "";
#endif
}
std::string RenderContext::SaveFileDialogKMP()
{
#ifdef __APPLE__
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.allowedFileTypes = @[@"kmp"];
        panel.canCreateDirectories = YES;

        if ([panel runModal] == NSModalResponseOK) {
            NSString* nsPath = panel.URL.path;
            return std::string([nsPath UTF8String]);
        }
        return "";
    }
#else

	OPENFILENAMEW ofn{};
	wchar_t fileName[MAX_PATH] = L"";

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = gMain->hwnd;
	ofn.lpstrFilter =
		L"MKWii Course Settings (*.kmp)\0*.kmp\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	if (GetSaveFileNameW(&ofn))
	{
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, fileName, -1, NULL, 0, NULL, NULL);
		std::string path(size_needed - 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, fileName, -1, path.data(), size_needed, NULL, NULL);
		return path;
	}

	return "";
#endif
}
bool RenderContext::LoadKMP(std::filesystem::path path)
{
	if (!std::filesystem::exists(path))
		return false;

	std::string name = path.extension().string();

	name.erase(std::remove_if(name.begin(), name.end(),
		[](unsigned char c) { return std::isspace(c); }),
		name.end());

	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	if (name != ".kmp")
		return false;

	bStream::CFileStream stream(path.string(), bStream::Endianess::Big, bStream::OpenMode::In);
	loadedKMP = kmpLoader.parseKMP(&stream);
		return true;
}
std::u16string RenderContext::utf8ToUtf16(const std::string& utf8)
{
	std::u16string out;
	out.reserve(utf8.size());

	uint32_t codepoint = 0;
	int bytes = 0;

	for (unsigned char c : utf8)
	{
		if (c <= 0x7F) {
			// ASCII
			out.push_back(c);
		}
		else if ((c >> 5) == 0x6) {
			// 2 bytes
			codepoint = c & 0x1F;
			bytes = 1;
		}
		else if ((c >> 4) == 0xE) {
			// 3 bytes
			codepoint = c & 0x0F;
			bytes = 2;
		}
		else if ((c >> 3) == 0x1E) {
			// 4 bytes
			codepoint = c & 0x07;
			bytes = 3;
		}
		else if ((c >> 6) == 0x2) {
			// continuation
			codepoint = (codepoint << 6) | (c & 0x3F);
			if (--bytes == 0) {
				if (codepoint <= 0xFFFF) {
					out.push_back((char16_t)codepoint);
				}
				else {
					// surrogate pair
					codepoint -= 0x10000;
					out.push_back((char16_t)(0xD800 + (codepoint >> 10)));
					out.push_back((char16_t)(0xDC00 + (codepoint & 0x3FF)));
				}
			}
		}
	}

	return out;
}
void RenderContext::DrawTextBase(
	float x, float y,
	const std::string& text,
	const glm::vec4& color,
	float scaleX, float scaleY,
	const std::function<float(float)>& ratioFunc)
{
	if (!mUIFont) return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::u16string u16 = utf8ToUtf16(text);

	ddraw.vertices.reserve(u16.size() * 6);

	writer.setFont(mUIFont);

	float ascent = (mUIFont->rfnt.ascent - 6.0f) * scaleY;

	writer.cursor = glm::vec3(x, y + ascent, 0.0f);

	writer.scale = glm::vec3(scaleX, scaleY, scaleX);
	writer.color0 = Color(color.r, color.g, color.b, color.a);
	writer.color1 = writer.color0;

	writer.drawString(uiProj, ddraw, u16, ratioFunc);

	mUIFont->materialHelper->bind(uiProj, writer.color0, writer.color1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mUIFont->gfxTextures[0]);
	ddraw.flush();
	glDisable(GL_BLEND);
}
void RenderContext::DrawTextCenter(float x, float y, float w, float h,
	const std::string& text, const glm::vec4& color)
{
	if (!mUIFont) return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::u16string u16 = utf8ToUtf16(text);

	lyt::CharWriter measure;
	measure.setFont(mUIFont);
	measure.scale = glm::vec3(1.0f);
	measure.cursor = glm::vec3(0, 0, 0);

	glm::vec4 rect;
	measure.calcRect(rect, u16);
	float textWidth = rect.z - rect.x;

	float ascent = mUIFont->rfnt.ascent;
	float height = mUIFont->rfnt.height;

	float centerX = x + w * 0.5f;
	float centerY = y + h * 0.5f;

	float drawX = centerX - textWidth * 0.5f;

	float baselineY = centerY + (height * 0.5f) - ascent;

	ddraw.vertices.reserve(u16.size() * 6);

	lyt::CharWriter writer;
	writer.setFont(mUIFont);
	writer.cursor = glm::vec3(drawX, baselineY, 0.0f);

	writer.colorT = Color(color.r, color.g, color.b, color.a);
	writer.colorB = writer.colorT;

	float squeezeStartX = drawX + textWidth - (40.0f * (mUIWidth / 512.0f));

	auto ratioFunc = [&](float cursorX) {
		float t = (cursorX - squeezeStartX) / (80.0f * (mUIWidth / 512.0f));
		t = std::clamp(t, 0.0f, 1.0f);
		return std::lerp(1.0f, 0.75f, t);
	};

	writer.drawString(uiProj, ddraw, u16, ratioFunc);

	mUIFont->materialHelper->bind(uiProj, writer.color0, writer.color1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mUIFont->gfxTextures[0]);
	ddraw.flush();
	glDisable(GL_BLEND);
}

void RenderContext::Reset()
{
	loaded = false;
	enptItem = nullptr;
	potiItem = nullptr;
	areaItem = nullptr;
	gobjItem = nullptr;
	cnptItem = nullptr;
	ckptItem = nullptr;
	cameItem = nullptr;
	itptItem = nullptr;
	jgptItem = nullptr;
	msptItem = nullptr;
	primaryIndex = -1;
	dragIndices.clear();
    lapLabelItem = lapValueItem = nullptr; 
	speedLabelItem = speedValueItem = nullptr; 
	flareLabelItem = flareValueItem = nullptr; 
	flareColorLabelItem = flareColorValueItem = nullptr; 
	poleLabelItem = poleValueItem = nullptr; 
	startZoneLabelItem = startZoneValueItem = nullptr;
	startPositionItem = courseInfoItem = nullptr;
	mainPanel.groups.clear();
	mainPanel.rows.clear();
	mainPanel.open = false;
	ownedItems.clear();
	mainPanel = UIPanel{};
	rows.clear();
	loadedKMP.area.clear();
	loadedKMP.came.clear();
	loadedKMP.ckph.clear();
	loadedKMP.ckpt.clear();
	loadedKMP.cnpt.clear();
	loadedKMP.enph.clear();
	loadedKMP.enpt.clear();
	loadedKMP.gobj.clear();
	loadedKMP.itph.clear();
	loadedKMP.itpt.clear();
	loadedKMP.jgpt.clear();
	loadedKMP.ktpt.clear();
	loadedKMP.mspt.clear();
	loadedKMP.poti.clear();
	loadedKMP.stgi.dist = 0;
	loadedKMP.stgi.flareColor[0] = 0;
	loadedKMP.stgi.flareColor[1] = 0;
	loadedKMP.stgi.flareColor[2] = 0;
	loadedKMP.stgi.flareColor[3] = 0;
	loadedKMP.stgi.flare = 0;
	loadedKMP.stgi.lap = 0;
	loadedKMP.stgi.pole = 0;
	loadedKMP.stgi.speedMod = 0;
	loadedKMP.stgi.unk1 = 0;
	loadedKMP.stgi.unk2 = 0;
	initKMP = loadedKMP;
	kclRenderer.Reset();
	kclRenderer.mKCLs.clear();
	kclRenderer.mTris.clear();
	kclRenderer.verts.clear();
	kclRenderer.mPrisms.clear();
	kclRenderer.mNormals.clear();
	kclRenderer.mPositions.clear();
	kclRenderer.opaqueTris.clear();
	kclRenderer.transparentTris.clear();
	posteffect2.currentThresholdColor.r = 0;
	posteffect2.currentThresholdColor.g = 0;
	posteffect2.currentThresholdColor.b = 0;
	posteffect2.currentThresholdColor.a = 0;
	bblmRes.thresholdColor.r = 0;
	bblmRes.thresholdColor.g = 0;
	bblmRes.thresholdColor.b = 0;
	bblmRes.thresholdColor.a = 0;
	bblmRes.thresholdAmount = 0;
	bblmRes.compositeColor.r = 0;
	bblmRes.compositeColor.g = 0;
	bblmRes.compositeColor.b = 0;
	bblmRes.compositeColor.a = 0;
	bblmRes.compositeBlendMode = 0;
	bblmRes.bokehColorScale1 = 0;
	bblmRes.bokehColorScale0 = 0;
	bblmRes.blurFlags = 0;
	bblmRes.blur1Radius = 0;
	bblmRes.blur1NumPasses = 0;
	bblmRes.blur1Intensity = 0;
	bblmRes.blur0Radius = 0;
	bblmRes.blur0Intensity = 0;
	bdofRes.indTexTransTScroll = 0;
	bdofRes.indTexTransSScroll = 0;
	bdofRes.indTexScaleT = 0;
	bdofRes.indTexScaleS = 0;
	bdofRes.indTexIndScaleT = 0;
	bdofRes.indTexIndScaleS = 0;
	bdofRes.focusRange = 0;
	bdofRes.focusCenter = 0;
	bdofRes.flags = 0;
	bdofRes.drawMode = 0;
	bdofRes.depthCurveType = 0;
	bdofRes.blurRadius = 0;
	bdofRes.blurDrawAmount = 0;
	bdofRes.blurAlpha[0] = 0;
	bdofRes.blurAlpha[1] = 0;
	bfgRes.entries.clear();
	posteffect2.blur0Program = 0;
	posteffect2.blur1Program = 0;
	posteffect2.bdofProgram = 0;
	posteffect2.bdofProgram2 = 0;
	brresRenderer2.clearinstance();
	brresRenderer.clearinstance();
	undoStack.clear();
	redoStack.clear();
	currentState = {};
	archive = nullptr;
	SetActivity("Does nothing", "MKWii KMP Editor");
}
void RenderContext::LoadFile(std::string path)
{
	if (loaded) Reset();
	posteffect2.currentThresholdColor.r = 0;
	posteffect2.currentThresholdColor.g = 0;
	posteffect2.currentThresholdColor.b = 0;
	posteffect2.currentThresholdColor.a = 0;
	bblmRes.thresholdColor.r = 0;
	bblmRes.thresholdColor.g = 0;
	bblmRes.thresholdColor.b = 0;
	bblmRes.thresholdColor.a = 0;
	bblmRes.thresholdAmount = 0;
	bblmRes.compositeColor.r = 0;
	bblmRes.compositeColor.g = 0;
	bblmRes.compositeColor.b = 0;
	bblmRes.compositeColor.a = 0;
	bblmRes.compositeBlendMode = 0;
	bblmRes.bokehColorScale1 = 0;
	bblmRes.bokehColorScale0 = 0;
	bblmRes.blurFlags = 0;
	bblmRes.blur1Radius = 0;
	bblmRes.blur1NumPasses = 0;
	bblmRes.blur1Intensity = 0;
	bblmRes.blur0Radius = 0;
	bblmRes.blur0Intensity = 0;
	bdofRes.indTexTransTScroll = 0;
	bdofRes.indTexTransSScroll = 0;
	bdofRes.indTexScaleT = 0;
	bdofRes.indTexScaleS = 0;
	bdofRes.indTexIndScaleT = 0;
	bdofRes.indTexIndScaleS = 0;
	bdofRes.focusRange = 0;
	bdofRes.focusCenter = 0;
	bdofRes.flags = 0;
	bdofRes.drawMode = 0;
	bdofRes.depthCurveType = 0;
	bdofRes.blurRadius = 0;
	bdofRes.blurDrawAmount = 0;
	bdofRes.blurAlpha[0] = 0;
	bdofRes.blurAlpha[1] = 0;
	bfgRes.entries.clear();
	posteffect2.blur0Program = 0;
	posteffect2.blur1Program = 0;
	posteffect2.bdofProgram = 0;
	posteffect2.bdofProgram2 = 0;
	std::filesystem::path modelPath = std::filesystem::path(path);
	archive = Archive::U8::Create();
	bStream::CFileStream modelArchive(modelPath.string(), bStream::Endianess::Big, bStream::OpenMode::In);
	if (!archive->Load(&modelArchive))
	{
		archive = nullptr;
		if (!LoadKMP(modelPath))
		{
			mPath = "";
			printf("This is NOT Course file or this file is corrupt: %s\n", path.c_str());
			return;
		}
		std::filesystem::path parent = modelPath.parent_path();
		SetActivity("Editing a kmp", "MKWii KMP Editor");
		{
			std::filesystem::path p = parent / "course_model.brres";
			if (std::filesystem::exists(p))
			{
				bStream::CFileStream stream(p.string(), bStream::Endianess::Big, bStream::OpenMode::In);
				brresRenderer.Loader(&stream, 0, { 0,0,0 }, { 0,0,0 }, { 1,1,1 });
			}
		}

		{
			std::filesystem::path p = parent / "vrcorn_model.brres";
			if (std::filesystem::exists(p))
			{
				bStream::CFileStream stream(p.string(), bStream::Endianess::Big, bStream::OpenMode::In);
				brresRenderer.Loader(&stream, 0, { 0,0,0 }, { 0,0,0 }, { 1,1,1 });
			}
		}
		brresRenderer.LoadAllAnimations();
		{
			std::filesystem::path p = parent / "course.kcl";
			if (std::filesystem::exists(p))
			{
				bStream::CFileStream stream(p.string(), bStream::Endianess::Big, bStream::OpenMode::In);
				kclRenderer.Load(&stream);
			}
		}

		std::filesystem::path pe = parent / "posteffect";

		auto loadPE = [&](const char* name, auto func) {
			std::filesystem::path p = pe / name;
			if (std::filesystem::exists(p))
			{
				bStream::CFileStream stream(p.string(), bStream::Endianess::Big, bStream::OpenMode::In);
				func(&stream);
			}
		};

		loadPE("posteffect.blight", [&](auto s) {
			auto res = posteffect1.ParseBLIGHT(s);
			blight::EggLightManager manager(res);
			mLights = manager.lightSet;
		});

		loadPE("posteffect.bblm", [&](auto s) {
			bblmRes = posteffect2.ParseBBLM(s);
			posteffect2.LoadBBLM(bblmRes);
		});

		loadPE("posteffect.bdof", [&](auto s) {
			bdofRes = posteffect2.ParseBDOF(s);
		});

		loadPE("posteffect.bti", [&](auto s) {
			Bti bti;
			if (bti.Load(s)) {
				glGenTextures(1, &posteffect2.mViewTex);
				glBindTexture(GL_TEXTURE_2D, posteffect2.mViewTex);

				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
					bti.mWidth, bti.mHeight, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, bti.GetData());

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
					bti.mMinFilterType == 0 ? GL_LINEAR : GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
					bti.mMagFilterType == 0 ? GL_LINEAR : GL_NEAREST);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			posteffect2.LoadBDOF(bdofRes);
		});

		glfwSetWindowTitle(
			gMain->mWindow,
			("KMParse: Editing " + modelPath.filename().string()).c_str()
		);
		glm::vec3 globalMin(+FLT_MAX);
		glm::vec3 globalMax(-FLT_MAX);

		for (const auto& shape : brresRenderer.allLoadedModels[0]->shapes) {
			globalMin = glm::min(globalMin, shape.runtime->min);
			globalMax = glm::max(globalMax, shape.runtime->max);
		}

		glm::vec3 modelCenter = (globalMin + globalMax) * 0.5f;

		mCamera.SetPosition(modelCenter);
		undoStack.clear();
		redoStack.clear();
		currentState = {};

		elapsedTime = 0;
		loaded = true;
		BuildPanels();
		return;
	}
	auto modelFile = archive->Get<Archive::File>("course_model.brres");
	auto modelFile2 = archive->Get<Archive::File>("vrcorn_model.brres");
	auto kclFile = archive->Get<Archive::File>("course.kcl");
	auto kmpFile = archive->Get<Archive::File>("course.kmp");
	auto blightFile = archive->Get<Archive::File>("posteffect/posteffect.blight");
	auto bblmFile1 = archive->Get<Archive::File>("posteffect/posteffect.bblm");
	auto bdofFile = archive->Get<Archive::File>("posteffect/posteffect.bdof");
	auto btiFile = archive->Get<Archive::File>("posteffect/posteffect.bti");
	auto bfgFile = archive->Get<Archive::File>("posteffect/posteffect.bfg");
	if (!modelFile) {
		modelFile = archive->Get<Archive::File>("./course_model.brres");
		modelFile2 = archive->Get<Archive::File>("./vrcorn_model.brres");
		kclFile = archive->Get<Archive::File>("./course.kcl");
		kmpFile = archive->Get<Archive::File>("./course.kmp");
		blightFile = archive->Get<Archive::File>("./posteffect/posteffect.blight");
		bblmFile1 = archive->Get<Archive::File>("./posteffect/posteffect.bblm");
		bdofFile = archive->Get<Archive::File>("./posteffect/posteffect.bdof");
		btiFile = archive->Get<Archive::File>("./posteffect/posteffect.bti");
		bfgFile = archive->Get<Archive::File>("./posteffect/posteffect.bfg");
		if (!modelFile)
		{
			archive = nullptr;
			mPath = "";
			printf("Cannot find \"course_model.brres\" in %s\n", path.c_str());
			return;
		}
	}
	if (!modelFile2) {
		archive = nullptr;
		mPath = "";
		printf("Cannot find \"vrcorn_model.brres\" in %s\n",path.c_str());
		return;
	}
	if (!kclFile) {
		archive = nullptr;
		mPath = "";
		printf("Cannot find \"course.kcl\" in %s\n", path.c_str());
		return;
	}
	if (!kmpFile) {
		archive = nullptr;
		mPath = "";
		printf("Cannot find \"course.kmp\" in %s\n", path.c_str());
		return;
	}
	bStream::CMemoryStream kmpStream(kmpFile->GetData(), kmpFile->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
	loadedKMP = kmpLoader.parseKMP(&kmpStream);
	bStream::CMemoryStream modelStream(modelFile->GetData(), modelFile->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
	SetActivity("Editing " + modelPath.filename().string(),"MKWii KMP Editor");
	brresRenderer.Loader(&modelStream, 0, { 0,0,0 }, { 0,0,0 }, { 1,1,1 });

	bStream::CMemoryStream modelStream2(modelFile2->GetData(), modelFile2->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);

	brresRenderer.Loader(&modelStream2, 0, { 0,0,0 }, { 0,0,0 }, { 1,1,1 });

	brresRenderer.LoadAllAnimations();

	bStream::CMemoryStream kclStream(kclFile->GetData(), kclFile->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);

	kclRenderer.Load(&kclStream);
	glfwSetWindowTitle(
		gMain->mWindow,
		("KMParse: Editing " + modelPath.filename().string()).c_str()
	);
	if (blightFile) {
		bStream::CMemoryStream postEffectStream(
			blightFile->GetData(),
			blightFile->GetSize(),
			bStream::Endianess::Big,
			bStream::OpenMode::In
		);

		auto res = posteffect1.ParseBLIGHT(&postEffectStream);
		blight::EggLightManager manager(res);
		mLights = manager.lightSet;

	}
	if (bblmFile1) {
		bStream::CMemoryStream bblmStream(
			bblmFile1->GetData(),
			bblmFile1->GetSize(),
			bStream::Endianess::Big,
			bStream::OpenMode::In
		);

		bblmRes = posteffect2.ParseBBLM(&bblmStream);
		posteffect2.LoadBBLM(bblmRes);

	}
	if (bdofFile) {
		bStream::CMemoryStream bdofStream(
			bdofFile->GetData(),
			bdofFile->GetSize(),
			bStream::Endianess::Big,
			bStream::OpenMode::In
		);

		bdofRes = posteffect2.ParseBDOF(&bdofStream);

		if (btiFile) {
			bStream::CMemoryStream btiStream(
				btiFile->GetData(),
				btiFile->GetSize(),
				bStream::Endianess::Big,
				bStream::OpenMode::In
			);

			Bti bti;
			if (bti.Load(&btiStream)) {
				glGenTextures(1, &posteffect2.mViewTex);
				glBindTexture(GL_TEXTURE_2D, posteffect2.mViewTex);

				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
					bti.mWidth, bti.mHeight, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, bti.GetData());

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
					bti.mMinFilterType == 0 ? GL_LINEAR : GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
					bti.mMagFilterType == 0 ? GL_LINEAR : GL_NEAREST);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
		}

		posteffect2.LoadBDOF(bdofRes);

	}
	if (bfgFile) {
		bStream::CMemoryStream bfgStream(
			bfgFile->GetData(),
			bfgFile->GetSize(),
			bStream::Endianess::Big,
			bStream::OpenMode::In
		);

		bfgRes = bfg.ParseBFG(&bfgStream, bfgFile->GetSize());
	}
	glm::vec3 globalMin(+FLT_MAX);
	glm::vec3 globalMax(-FLT_MAX);

	for (const auto& shape : brresRenderer.allLoadedModels[0]->shapes) {
		globalMin = glm::min(globalMin, shape.runtime->min);
		globalMax = glm::max(globalMax, shape.runtime->max);
	}

	glm::vec3 modelCenter = (globalMin + globalMax) * 0.5f;

	mCamera.SetPosition(modelCenter);
	elapsedTime = 0;
	undoStack.clear();
	redoStack.clear();
	currentState = {};
	loaded = true;
	BuildPanels();
}
void RenderContext::SetLights(bres::LightSet& lights) {
	brresRenderer.SetLights(lights);
}
void RenderContext::SetLights2(bres::LightSet& lights) {
	brresRenderer2.SetLights(lights);
}
RenderContext::RenderContext()
{
	glGenVertexArrays(1, &uiVAO);
	glGenBuffers(1, &uiVBO);

	glBindVertexArray(uiVAO);
	glBindBuffer(GL_ARRAY_BUFFER, uiVBO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(float),
		(void*)0
	);

	glBindVertexArray(0);

	uProjLoc = glGetUniformLocation(uiProgram, "u_proj");
	uColorLoc = glGetUniformLocation(uiProgram, "u_color");

#ifdef _WIN32
    HRSRC res = FindResource(nullptr, MAKEINTRESOURCE(IDR_FONT), RT_RCDATA);
    HGLOBAL mem = LoadResource(nullptr, res);
    DWORD size = SizeofResource(nullptr, res);
    void* data = LockResource(mem);

    bStream::CMemoryStream fontStream(
        (uint8_t*)data,
        size,
        bStream::Endianess::Little,
        bStream::OpenMode::In
    );
#elif defined(__APPLE__)
    NSBundle* bundle = [NSBundle mainBundle];
    NSString* path = [bundle pathForResource:@"font" ofType:@"brfnt"];
    
    if (path == nil) {
        return;
    }

    std::string cPath = [path UTF8String];

    bStream::CFileStream fontStream(
		cPath,
        bStream::Endianess::Little,
        bStream::OpenMode::In
    );
#endif

	lyt::RFNT rfnt = lyt::parseBRFNT(&fontStream, "Font");

	mUIFont = new lyt::ResFont(rfnt);

	modelPoint = ModelBuilder()
		.addSphere(-150, -150, -150, 150, 150, 150, 8)
		.calculateNormals()
		.makeModel();

	modelPointSelection = ModelBuilder()
		.addSphere(-250, -250, 250, 250, 250, -250, 8)
		.calculateNormals()
		.makeModel();

	modelPath = ModelBuilder()
		.addCylinder(-150, -150, 0, 150, 150, 1000, 8, glm::vec3(1, 0, 0))
		.calculateNormals()
		.makeModel();

	modelArrow = ModelBuilder()
		.addCone(-250, -250, 1000, 250, 250, 1300, 8, glm::vec3(1, 0, 0))
		.calculateNormals()
		.makeModel();

	modelArrowUp = ModelBuilder()
		.addCone(-150, -150, 600, 150, 150, 1500, 8, glm::normalize(glm::vec3(0, 0.01, 1)))
		.calculateNormals()
		.makeModel();

	modelSizeCircle = ModelBuilder()
		.addSphere(-1, -1, -1, 1, 1, 1, 8)
		.calculateNormals()
		.makeModel();

	startZoneWideModel = ModelBuilder()
		.addQuad({ 0, -1000, 20 }, { 0, 1000, 20 }, { -5300, 1000, 20 }, { -5300, -1000, 20 }, glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1))
		.addQuad({ -5300, -1000, 20 }, { -5300, 1000, 20 }, { 0, 1000, 20 }, { 0, -1000, 20 },glm::vec4(1, 1, 1, 1),glm::vec4(1, 1, 1, 1),glm::vec4(1, 1, 1, 1),glm::vec4(1, 1, 1, 1))
		.calculateNormals()
		.makeModel();

	startZoneNarrowModel = ModelBuilder()
		.addQuad({ 0, -1000, 20 }, { 0, 1000, 20 }, { -4800, 1000, 20 }, { -4800, -1000, 20 }, glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1))
		.addQuad({ -4800, -1000, 20 }, { -4800, 1000, 20 }, { 0, 1000, 20 }, { 0, -1000, 20 }, glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1))
		.calculateNormals()
		.makeModel();

	modelPlayerPos = ModelBuilder()
		.addSphere(-60, -60, -60, 60, 60, 60, 8)
		.calculateNormals()
		.makeModel();

	modelPanel = ModelBuilder()
		.addQuad({ 0, 0, 1 }, { 1, 0, 1 }, { 1, 0, -1 }, { 0, 0, -1 }, glm::vec4(1, 0.5, 0.5, 1), glm::vec4(1, 0.5, 0.5, 1), glm::vec4(1, 0.5, 0.5, 1), glm::vec4(1, 0.5, 0.5, 1))
		.addQuad({ 0, 0, -1 }, { 1, 0, -1 }, { 1, 0, 1 }, { 0, 0, 1 }, glm::vec4(1, 0.5, 0.5, 1), glm::vec4(1, 0.5, 0.5, 1), glm::vec4(1, 0.5, 0.5, 1), glm::vec4(1, 0.5, 0.5, 1))
		.calculateNormals()
		.makeModel();

	modelPanelCK = ModelBuilder()
		.addQuad(glm::vec3(0, 0, 1), glm::vec3(1, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 0, 0), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1))
		.addQuad(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 0, 1), glm::vec3(0, 0, 1), glm::vec4(1, 1, 1, 0.5), glm::vec4(1, 1, 1, 0.5), glm::vec4(1, 1, 1, 0.5), glm::vec4(1, 1, 1, 0.5))
		.calculateNormals()
		.makeModel();

	modelPanelWithoutBacksize = ModelBuilder()
		.addQuad(glm::vec3(0, 0, 1), glm::vec3(1, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 0, 0), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1))
		.calculateNormals()
		.makeModel();

	modelAreaBox = ModelBuilder()
		.addCube(5000, 5000, -10000, -5000, -5000, 0,1)
		.calculateNormals()
		.makeModel();

	modelAreaCylinder = ModelBuilder()
		.addCylinder(5000, 5000, -10000, -5000, -5000, 0, 24, glm::vec3(0, 0, 0))
		.calculateNormals()
		.makeModel();

	SetActivity("Does nothing", "MKWii KMP Editor");
}
static const uint8_t behavior2Mask[] = {
	0x00,
	0x80,
	0x40,
	0x20,
	0x10,
};
std::vector<std::string> loadType = {
	"Unload object",   // 0
	"Load object"      // 1
};
std::vector<std::string> applyTo = {
	"Only activating player",  // 0
	"Everyone"                 // 1
};
std::vector<std::string> lapCondition = {
	"No lap check",   // 0
	"Lap 1",
	"Lap 2",
	"Lap 3",
	"Lap 4",
	"Lap 5",
	"Lap 6",
	"Lap 7"
};
void RenderContext::BuildPanels()
{
	ownedItems.clear();
	mainPanel = UIPanel{};
	UIPanelBuilder b(*this, mainPanel);
	initKMP = loadedKMP;

	auto& g1 = b.addGroup("Course Info", false);

	courseInfoItem = g1.titleItem;

	glm::vec4 flareColorVec = {
	loadedKMP.stgi.flareColor[0] / 255.0f,
	loadedKMP.stgi.flareColor[1] / 255.0f,
	loadedKMP.stgi.flareColor[2] / 255.0f,
	loadedKMP.stgi.flareColor[3] / 255.0f
	};

	auto& lapRow = b.addNumber(g1, "Lap Count", loadedKMP.stgi.lap, 1, 9, 1,1, true, false,
		[&](uint8_t v) { loadedKMP.stgi.lap = v; });

	lapLabelItem = lapRow.label;
	lapValueItem = lapRow.value;

auto& speedRow = b.addFloat(g1, "Speed Mod.", loadedKMP.stgi.speedMod,
           0.0f, 999.0f, 0.0f, 4, true, false,
           [&](float v) { loadedKMP.stgi.speedMod = v; });

speedLabelItem = speedRow.label;
speedValueItem = speedRow.value;

auto& flareRow = b.addCheckbox(g1, "   Enable Lensflare", loadedKMP.stgi.flare, true, false,
	[this](bool v)
{
	loadedKMP.stgi.flare = v;
	std::string pathing = std::filesystem::path(mPath).filename().string();

	glfwSetWindowTitle(
		gMain->mWindow,
		("KMParse: Editing " + pathing + " (Unsaved)").c_str()
	);
});

flareLabelItem = flareRow.label;
flareValueItem = flareRow.value;

auto& flareColorRow = b.addColor(g1, "Lensflare Color", flareColorVec,
		[&](glm::vec4 c) {
		loadedKMP.stgi.flareColor[0] = (uint8_t)(c.r * 255.0f);
		loadedKMP.stgi.flareColor[1] = (uint8_t)(c.g * 255.0f);
		loadedKMP.stgi.flareColor[2] = (uint8_t)(c.b * 255.0f);
		loadedKMP.stgi.flareColor[3] = (uint8_t)(c.a * 255.0f);
	});

	flareColorLabelItem = flareColorRow.label;
	flareColorValueItem = flareColorRow.value;

	auto& g2 = b.addGroup("Starting Points", false);

	startPositionItem = g2.titleItem;

	std::vector<std::string> poles = { "Left", "Right" };
	auto& poleRow = b.addList(g2, "Pole Position", poles, loadedKMP.stgi.pole, true, false,
		[&](uint8_t i) { loadedKMP.stgi.pole = i; });

	poleLabelItem = poleRow.label;
	poleValueItem = poleRow.value;

	std::vector<std::string> startZone = { "Normal", "Narrow" };
	auto& startRow = b.addList(g2, "Start Zone", startZone, loadedKMP.stgi.dist, true, false,
		[&](uint8_t i) { loadedKMP.stgi.dist = i; });

	startZoneLabelItem = startRow.label;
	startZoneValueItem = startRow.value;

	PointUI uiKTPT;

	uiKTPT.posX = b.addFloat(g2, "X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.ktpt, primaryIndex, [&](auto& p) { p.pos.x = v; }); }).value;

	uiKTPT.posY = b.addFloat(g2, "Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.ktpt, primaryIndex, [&](auto& p) { p.pos.y = v; }); }).value;

	uiKTPT.posZ = b.addFloat(g2, "Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.ktpt, primaryIndex, [&](auto& p) { p.pos.z = v; }); }).value;

	uiKTPT.rotX = b.addFloat(g2, "Rot.X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.ktpt, primaryIndex, [&](auto& p) { p.rot.x = v; }); }).value;

	uiKTPT.rotY = b.addFloat(g2, "Rot.Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.ktpt, primaryIndex, [&](auto& p) { p.rot.y = v; }); }).value;

	uiKTPT.rotZ = b.addFloat(g2, "Rot.Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.ktpt, primaryIndex, [&](auto& p) { p.rot.z = v; }); }).value;

	auto& g3 = b.addGroup("Enemy Paths", false);
	enptItem = g3.titleItem;
	PointUI uiENPT;

	uiENPT.posX = b.addFloat(g3, "X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.enpt, primaryIndex, [&](auto& p) { p.pos.x = v; }); }).value;

	uiENPT.posY = b.addFloat(g3, "Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.enpt, primaryIndex, [&](auto& p) { p.pos.y = v; }); }).value;

	uiENPT.posZ = b.addFloat(g3, "Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.enpt, primaryIndex, [&](auto& p) { p.pos.z = v; }); }).value;

	uiENPT.deviation = b.addFloat(g3, "Deviation", 0, 0, 100, 1, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.enpt, primaryIndex, [&](auto& p) { p.deviation = v; }); }).value;

	uiENPT.setting1 = b.addList(g3, "Behavior 1", { "None", "Requires Mushroom", "Use Muhsroom", "Force Wheelie", "End Wheelie" }, 0, false, false,
		[&](int v) { applyToSelected(loadedKMP.enpt, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiENPT.setting2 = b.addList(g3, "Drift", { "None", "End Drift", "Can't Drift", "Force Drift" }, 0, false, false,
		[&](int v) { applyToSelected(loadedKMP.enpt, primaryIndex, [&](auto& p) { p.s2 = v; }); }).value;

	uiENPT.setting3 = b.addList(g3, "Behavior 2", { "None", "Can't Use Mushroom", "(!) Play Won Animations", "(!) Force Throwing Items", "(!) Hide Characters" }, 0, false, false,
		[&](int v) { applyToSelected(loadedKMP.enpt, primaryIndex, [&](auto& p) { p.s3 = behavior2Mask[v]; }); }).value;

	auto& g4 = b.addGroup("Item Paths", false);
	itptItem = g4.titleItem;
	PointUI uiITPT;

	uiITPT.posX = b.addFloat(g4, "X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.itpt, primaryIndex, [&](auto& p) { p.pos.x = v; }); }).value;

	uiITPT.posY = b.addFloat(g4, "Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.itpt, primaryIndex, [&](auto& p) { p.pos.y = v; }); }).value;

	uiITPT.posZ = b.addFloat(g4, "Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.itpt, primaryIndex, [&](auto& p) { p.pos.z = v; }); }).value;

	uiITPT.deviation = b.addFloat(g4, "Deviation", 0, 0, 100, 1, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.itpt, primaryIndex, [&](auto& p) { p.deviation = v; }); }).value;

	uiITPT.setting1 = b.addList(g4, "B.Bill Gravity", { "None", "Use Gravity", "Disregard Gravity" }, 0, false, false,
		[&](int v) { applyToSelected(loadedKMP.itpt, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiITPT.setting2 = b.addList(g4, "Item Behavior", { "None", "B.Bill Can't Stop Here", "Low-priority Route", "Mixed" }, 0, false, false,
		[&](int v) { applyToSelected(loadedKMP.itpt, primaryIndex, [&](auto& p) { p.s2 = v; }); }).value;

	PointUI uiCKPT;
	auto& gCKPT = b.addGroup("Checkpoints", false);
	ckptItem = gCKPT.titleItem;

	uiCKPT.editingY = b.addFloat(gCKPT, "Editing Y", 0, -131071, 131071, 1003, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.ckpt, primaryIndex, [&](auto& p) { editingY = v; }); }).value;

	uiCKPT.posX1 = b.addFloat(gCKPT, "X1", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.ckpt, primaryIndex, [&](auto& p) { p.x1 = v; }); }).value;

	uiCKPT.posZ1 = b.addFloat(gCKPT, "Z1", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.ckpt, primaryIndex, [&](auto& p) { p.z1 = v; }); }).value;

	uiCKPT.posX2 = b.addFloat(gCKPT, "X2", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.ckpt, primaryIndex, [&](auto& p) { p.x2 = v; }); }).value;

	uiCKPT.posZ2 = b.addFloat(gCKPT, "Z2", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.ckpt, primaryIndex, [&](auto& p) { p.z2 = v; }); }).value;

	uiCKPT.typeCK = b.addNumber(gCKPT, "Type", 0, 0, 255, 255, 6, false, false,
		[&](int v) { applyToSelected(loadedKMP.ckpt, primaryIndex, [&](auto& p) { p.type = v; }); }).value;

	uiCKPT.respawn = b.addNumber(gCKPT, "Type", 0, 0, 255, 255, 6, false, false,
		[&](int v) { applyToSelected(loadedKMP.ckpt, primaryIndex, [&](auto& p) { p.respawn = v; }); }).value;

	PointUI uiJGPT;

	auto& gJGPT = b.addGroup("Respawn Points", false);
	jgptItem = gJGPT.titleItem;

	uiJGPT.posX = b.addFloat(gJGPT, "X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.jgpt, primaryIndex, [&](auto& p) { p.pos.x = v; }); }).value;

	uiJGPT.posY = b.addFloat(gJGPT, "Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.jgpt, primaryIndex, [&](auto& p) { p.pos.y = v; }); }).value;

	uiJGPT.posZ = b.addFloat(gJGPT, "Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.jgpt, primaryIndex, [&](auto& p) { p.pos.z = v; }); }).value;

	uiJGPT.rotX = b.addFloat(gJGPT, "Rot.X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.jgpt, primaryIndex, [&](auto& p) { p.rot.x = v; }); }).value;

	uiJGPT.rotY = b.addFloat(gJGPT, "Rot.Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.jgpt, primaryIndex, [&](auto& p) { p.rot.y = v; }); }).value;

	uiJGPT.rotZ = b.addFloat(gJGPT, "Rot.Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.jgpt, primaryIndex, [&](auto& p) { p.rot.z = v; }); }).value;

	uiJGPT.id = b.addNumber(gJGPT, "ID", 0, 0, 65535, 0, 6, false, false,
		[&](int v) { applyToSelected(loadedKMP.jgpt, primaryIndex, [&](auto& p) { p.id = v; }); }).value;

	uiJGPT.soundTrig = b.addNumber(gJGPT, "Sound Trig.", 0, 0, 65535, 0, 6, false, false,
		[&](int v) { applyToSelected(loadedKMP.jgpt, primaryIndex, [&](auto& p) { p.sound = v; }); }).value;
	cameraList.clear();
	cannonList.clear();
	routeList.clear();
	fogList.clear();
	cRoute = -1;
	cameraList.push_back("None");
	routeList.push_back("None");
	for (int i = 0; i < loadedKMP.came.size(); i++)
	{
		cameraList.push_back("Camera " + std::to_string(i));
	}
	for (int i = 0; i < loadedKMP.cnpt.size(); i++)
	{
		cannonList.push_back("Cannon " + std::to_string(i));
	}
	for (int i = 0; i < loadedKMP.poti.size(); i++)
	{
		routeList.push_back("Route " + std::to_string(i));
	}
	for (int i = 0; i < bfgRes.entries.size(); i++)
	{
		fogList.push_back("Entry " + std::to_string(i));
	}
	PointUI uiGOBJ;
	auto& gGOBJ = b.addGroup("Objects", false);
	gobjItem = gGOBJ.titleItem;

	PointUI uiPOTI;
	auto& gPOTI = b.addGroup("Routes", false);
	potiItem = gPOTI.titleItem;

	uiPOTI.routeIndex = b.addList(gPOTI, "Current", routeList, 0, true, false,
		[&](int v) { applyToSelected(loadedKMP.poti, primaryIndex, [&](auto& p) { cRoute = v; }); }).value;

	uiPOTI.button1 = b.addButton(gPOTI, "Add New Route", [this]() {

		kmp::KMP before = loadedKMP;

		kmp::POTI poti;
		poti.num = 1;
		poti.s1 = 0;
		poti.s2 = 0;

		kmp::POTI_Point pt;
		pt.pos = glm::vec3(0, 0, 0);
		pt.s1 = 0;
		pt.s2 = 0;
		poti.points.push_back(pt);

		loadedKMP.poti.push_back(poti);

		kmp::KMP after = loadedKMP;

		UIItemState beforeUI, afterUI;
		pushHistory(-1, before, after, beforeUI, afterUI);

		ctxPOTI.ui.routeIndex->listValues.push_back(
			"Route " + std::to_string(loadedKMP.poti.size() - 1)
		);
		ctxAREA.ui.routeIndex->listValues.push_back(
			"Route " + std::to_string(loadedKMP.poti.size() - 1)
		);
		ctxCAME.ui.route->listValues.push_back(
			"Route " + std::to_string(loadedKMP.poti.size() - 1)
		);

		ctxPOTI.ui.routeIndex->listIndex = loadedKMP.poti.size() - 1;
		ctxAREA.ui.routeIndex->listIndex = loadedKMP.poti.size() - 1;
		ctxCAME.ui.route->listIndex = loadedKMP.poti.size() - 1;
		gMain->dirty = true;
	}).value;


	uiPOTI.button2 = b.addButton(gPOTI, "Delete Current Route", [this]() {

		int idx = ctxPOTI.ui.routeIndex->listIndex;

		if (idx < 0 || idx >= loadedKMP.poti.size())
			return;

		kmp::KMP before = loadedKMP;

		loadedKMP.poti.erase(loadedKMP.poti.begin() + idx);

		kmp::KMP after = loadedKMP;

		UIItemState beforeUI, afterUI;
		pushHistory(-1, before, after, beforeUI, afterUI);


		ctxPOTI.ui.routeIndex->listValues.clear();
		ctxAREA.ui.routeIndex->listValues.clear();
		ctxCAME.ui.route->listValues.clear();
		for (int i = 0; i < loadedKMP.poti.size(); i++)
		{
			ctxPOTI.ui.routeIndex->listValues.push_back("Route " + std::to_string(i));
			ctxAREA.ui.routeIndex->listValues.push_back("Route " + std::to_string(i));
			ctxCAME.ui.route->listValues.push_back("Route " + std::to_string(i));
		}

		if (idx >= loadedKMP.poti.size())
		{
			ctxPOTI.ui.routeIndex->listIndex = loadedKMP.poti.size() - 1;
			ctxAREA.ui.routeIndex->listIndex = loadedKMP.poti.size() - 1;
			ctxCAME.ui.route->listIndex = loadedKMP.poti.size() - 1;
		}
		else
		{
			ctxPOTI.ui.routeIndex->listIndex = idx;
			ctxAREA.ui.routeIndex->listIndex = idx;
			ctxCAME.ui.route->listIndex = idx;
		}
		gMain->dirty = true;
	}).value;

	PointUI uiAREA;
	auto& gAREA = b.addGroup("Area", false);
	areaItem = gAREA.titleItem;

	uiAREA.type = b.addList(gAREA, "Type", { "Camera", "Env Effect", "Fog Effect", "Moving Water", "Force Recalc", "Minimap Control", "Bloom Effect", "Enable Boos", "Object Group", "Object Unload", "Fall Boundary", "(!)Low Gravity", "(!)Railriding", "(!)Conditional Objects", "(!)Air Ring", "(!)Teleport", "(!)Gravity", "(!)Wind" }, 0, false, false,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { SetAreaItem(ctxAREA.ui.type); p.type = v; }); }).value;

	uiAREA.shape = b.addList(gAREA, "Shape", { "Box", "Cylinder" }, 0, false, false,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.shape = v; }); }).value;

	uiAREA.posX = b.addFloat(gAREA, "X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.pos.x = v; }); }).value;

	uiAREA.posY = b.addFloat(gAREA, "Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.pos.y = v; }); }).value;

	uiAREA.posZ = b.addFloat(gAREA, "Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.pos.z = v; }); }).value;

	uiAREA.rotX = b.addFloat(gAREA, "Rot.X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.rot.x = v; }); }).value;

	uiAREA.rotY = b.addFloat(gAREA, "Rot.Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.rot.y = v; }); }).value;

	uiAREA.rotZ = b.addFloat(gAREA, "Rot.Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.rot.z = v; }); }).value;

	uiAREA.scaleX = b.addFloat(gAREA, "Scale.X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.scale.x = v; }); }).value;

	uiAREA.scaleY = b.addFloat(gAREA, "Scale.Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.scale.y = v; }); }).value;

	uiAREA.scaleZ = b.addFloat(gAREA, "Scale.Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.scale.z = v; }); }).value;

	uiAREA.priority = b.addNumber(gAREA, "Priority", 0, 0, 255, 1, 1, false, false,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.priority = v; }); }).value;
	uiAREA.cameraIndex = b.addList(gAREA, "Camera", cameraList, 0, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.cam = v; }); }).value;

	uiAREA.routeIndex = b.addList(gAREA, "Route", routeList, 0, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.route = v; }); }).value;

	uiAREA.setting11 = b.addCheckbox(gAREA, "   Enable Conditional Out of Bounds", false, false, true,
			[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { 
		std::string pathing = std::filesystem::path(mPath).filename().string();
		p.route = v;
		ctxAREA.ui.setting4->invisible = !v;
		ctxAREA.ui.coobStartCPIndex->invisible = !v;
		glfwSetWindowTitle(
			gMain->mWindow,
			("KMParse: Editing " + pathing + " (Unsaved)").c_str()
		);
	}); }).value;

	uiAREA.setting4 = b.addList(gAREA, "COOB Method", {"kHacker","Riidefi"}, 0, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { if (v == 0) p.route = 1; else if (v == 1) p.route = 0xFF; }); }).value;

	uiAREA.setting9 = b.addNumber(gAREA, "Acceleration", 0, 0, 65535, 65535, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiAREA.setting12 = b.addNumber(gAREA, "Route Speed", 0, 0, 65535, 65535, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s2 = v; }); }).value;

	auto coobSetting = b.addNumber(gAREA, "KCP Index", 0, 0, 65535, 65535, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s1 = v; }); });

	uiAREA.coobStartCPIndex = coobSetting.value;
	uiAREA.coobStartCPIndexName = coobSetting.label;

	uiAREA.coobEndCPIndex = b.addNumber(gAREA, "End Index", 0, 0, 65535, 65535, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s2 = v; }); }).value;

	uiAREA.setting10 = b.addNumber(gAREA, "Group ID", 0, 0, 65535, 65535, 3, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiAREA.enemyIndex = b.addNumber(gAREA, "Enemy Point", 0, 0, 255, 255, 3, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.enemy = v; }); }).value;

	uiAREA.setting6 = b.addNumber(gAREA, "BBLM File", 0, 0, 65535, 65535, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiAREA.setting5 = b.addNumber(gAREA, "Fade Time", 0, 0, 65535, 65535, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s2 = v; }); }).value;

	uiAREA.setting7 = b.addList(gAREA, "Object", { "EnvKareha", "EnvKarehaUp" }, 0, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiAREA.setting8 = b.addList(gAREA, "Fog Entry", fogList, 0, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiAREA.railRidingRotation = b.addCheckbox(gAREA, "   Rotation", false, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) {
		std::string pathing = std::filesystem::path(mPath).filename().string();
		p.pad = !v;
		glfwSetWindowTitle(
			gMain->mWindow,
			("KMParse: Editing " + pathing + " (Unsaved)").c_str()
		);
	}); }).value;

	uiAREA.cobjUnloadGroup = b.addNumber(gAREA, "Group ID", 0, 0, 65535, 0, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.pad = v; }); }).value;

	uiAREA.cobjUnloadKCLFlag = b.addNumber(gAREA, "KCL Flag", 0, 0, 65535, 0, 3, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiAREA.cobjUnloadFrame1 = b.addNumber(gAREA, "Load Frame", 0, 0, 65535, 0, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s2 = v; }); }).value;
	uint8_t flags = 1;

	uint8_t loadTypeIndex = flags & 0b00000011;      // Bit 0–1
	uint8_t applyToIndex = (flags >> 2) & 0b00000001;      // Bit 2
	uint8_t lapIndex = (flags >> 3) & 0b00000111;      // Bit 3–5
	uiAREA.cobjUnloadSetting = b.addList(
		gAREA, "Load Type", loadType, loadTypeIndex, false, true,
		[&](uint8_t v) {
		loadTypeIndex = v;
		applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p)
		{
			uint8_t flags = 0;
			flags |= (loadTypeIndex & 0b11);
			flags |= (applyToIndex & 0b1) << 2;
			flags |= (lapIndex & 0b111) << 3;

			p.route = flags;
		});
	}
	).value;

	uiAREA.cobjUnloadSetting2 = b.addList(
		gAREA, "Apply To", applyTo, applyToIndex, false, true,
		[&](uint8_t v) {
		applyToIndex = v;
		applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p)
		{
			uint8_t flags = 0;
			flags |= (loadTypeIndex & 0b11);
			flags |= (applyToIndex & 0b1) << 2;
			flags |= (lapIndex & 0b111) << 3;

			p.route = flags;
		});
	}
	).value;

	uiAREA.cobjUnloadSetting3 = b.addList(
		gAREA, "Lap Condition", lapCondition, lapIndex, false, true,
		[&](uint8_t v) {
		lapIndex = v;
		applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p)
		{
			uint8_t flags = 0;
			flags |= (loadTypeIndex & 0b11);
			flags |= (applyToIndex & 0b1) << 2;
			flags |= (lapIndex & 0b111) << 3;

			p.route = flags;
		});
	}
	).value;

	uiAREA.cobjUnloadFrame2 = b.addNumber(gAREA, "Loading Frame", 0, 0, 255, 0, 3, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.enemy = v; }); }).value;

	uiAREA.airRingBoostTime = b.addNumber(gAREA, "Boost Duration", 0, 0, 65535, 2, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiAREA.teleportTime = b.addNumber(gAREA, "Time to Teleport", 0, 0, 65535, 0, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiAREA.teleportAngle = b.addList(gAREA, "Cannon Point", cannonList, 0, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s2 = v; }); }).value;

	uiAREA.gravityType = b.addList(gAREA, "Gravity Type", {"Anti-Gravity","Configurable","Glider"}, 0, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.pad = v; }); }).value;

	uiAREA.antiGravity = b.addList(gAREA, "Direction", { "Normal","Flipped" }, 0, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiAREA.gravityScale = b.addNumber(gAREA, "Gravity Scale", 0, 0, 65535, 0, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s2 = v; }); }).value;

	uiAREA.windAngle = b.addNumber(gAREA, "Angle", 0, -360, 360, 0, 3, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s1 = v; }); }).value;

	uiAREA.windPower = b.addNumber(gAREA, "Power", 0, 0, 65535, 0, 5, false, true,
		[&](int v) { applyToSelected(loadedKMP.area, primaryIndex, [&](auto& p) { p.s2 = v; }); }).value;

	PointUI uiCAME;
	auto& gCAME = b.addGroup("Cameras", false);
	cameItem = gCAME.titleItem;
	uiCAME.setting8 = b.addList(gCAME, "Intro Start", cameraList, loadedKMP.openingCamera, true, false,
		[&](uint8_t i) { loadedKMP.openingCamera = i; }).value;

	uiCAME.type = b.addList(gCAME, "Type", { "Goal", "FixSearch", "PathSearch", "KartFollow", "KartPathFollow", "OP_FixMoveAt", "OP_PathMoveAt", "MiniGame", "MissionSuccess", "Unknown" }, 0, false, false,
		[&](int v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.type = v; }); }).value;

	uiCAME.nextCam = b.addList(gCAME, "Next Camera", cameraList, 0, false, false,
		[&](int v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.nextCam = v; }); }).value;

	uiCAME.route = b.addList(gCAME, "Route", routeList, 0, false, false,
		[&](int v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.route = v; }); }).value;

	uiCAME.posX = b.addFloat(gCAME, "X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.pos.x = v; }); }).value;

	uiCAME.posY = b.addFloat(gCAME, "Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.pos.y = v; }); }).value;

	uiCAME.posZ = b.addFloat(gCAME, "Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.pos.z = v; }); }).value;

	uiCAME.rotX = b.addFloat(gCAME, "Rot.X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.rot.x = v; }); }).value;

	uiCAME.rotY = b.addFloat(gCAME, "Rot.Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.rot.y = v; }); }).value;

	uiCAME.rotZ = b.addFloat(gCAME, "Rot.Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.rot.z = v; }); }).value;

	uiCAME.time = b.addFloat(gCAME, "Time", 0, 0, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.time = v; }); }).value;

	uiCAME.vCam = b.addNumber(gCAME, "Point Speed", 0, 0, 65535, 100, 6, false, false,
		[&](int v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.vCam = v; }); }).value;

	uiCAME.zoomStart = b.addFloat(gCAME, "Zoom Start", 0, -1e6, 1e6, 0, 3, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.zoomStart = v; }); }).value;

	uiCAME.zoomEnd = b.addFloat(gCAME, "Zoom End", 0, -1e6, 1e6, 0, 3, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.zoomEnd = v; }); }).value;

	uiCAME.viewStartX = b.addFloat(gCAME, "View Start X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.viewStart.x = v; }); }).value;

	uiCAME.viewStartY = b.addFloat(gCAME, "View Start Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.viewStart.y = v; }); }).value;

	uiCAME.viewStartZ = b.addFloat(gCAME, "View Start Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.viewStart.z = v; }); }).value;

	uiCAME.viewEndX = b.addFloat(gCAME, "View End X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.viewEnd.x = v; }); }).value;

	uiCAME.viewEndY = b.addFloat(gCAME, "View End Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.viewEnd.y = v; }); }).value;

	uiCAME.viewEndZ = b.addFloat(gCAME, "View End Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.viewEnd.z = v; }); }).value;

	uiCAME.shake = b.addNumber(gCAME, "Shake?", 0, 0, 65535, 0, 6, false, false,
		[&](int v) { applyToSelected(loadedKMP.came, primaryIndex, [&](auto& p) { p.shake = v; }); }).value;

	PointUI uiCNPT;

	auto& gCNPT = b.addGroup("Cannon Points", false);
	cnptItem = gCNPT.titleItem;

	uiCNPT.posX = b.addFloat(gCNPT, "Dest.X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.cnpt, primaryIndex, [&](auto& p) { p.pos.x = v; }); }).value;

	uiCNPT.posY = b.addFloat(gCNPT, "Dest.Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.cnpt, primaryIndex, [&](auto& p) { p.pos.y = v; }); }).value;

	uiCNPT.posZ = b.addFloat(gCNPT, "Dest.Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.cnpt, primaryIndex, [&](auto& p) { p.pos.z = v; }); }).value;

	uiCNPT.rotX = b.addFloat(gCNPT, "Rot.X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.cnpt, primaryIndex, [&](auto& p) { p.rot.x = v; }); }).value;

	uiCNPT.rotY = b.addFloat(gCNPT, "Rot.Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.cnpt, primaryIndex, [&](auto& p) { p.rot.y = v; }); }).value;

	uiCNPT.rotZ = b.addFloat(gCNPT, "Rot.Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.cnpt, primaryIndex, [&](auto& p) { p.rot.z = v; }); }).value;

	uiCNPT.id = b.addNumber(gCNPT, "ID", 0, 0, 65535, 0, 6, false, false,
		[&](int v) { applyToSelected(loadedKMP.cnpt, primaryIndex, [&](auto& p) { p.id = v; }); }).value;

	uiCNPT.setting1 = b.addList(gCNPT, "Behavior", { "Fast","Curved","Curved and Slow"}, 0, false, false,
		[&](int v) { applyToSelected(loadedKMP.cnpt, primaryIndex, [&](auto& p) { p.effect = v; }); }).value;


	PointUI uiMSPT;

	auto& gMSPT = b.addGroup("Finish Points", false);
	msptItem = gMSPT.titleItem;

	uiMSPT.posX = b.addFloat(gMSPT, "X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.mspt, primaryIndex, [&](auto& p) { p.pos.x = v; }); }).value;

	uiMSPT.posY = b.addFloat(gMSPT, "Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.mspt, primaryIndex, [&](auto& p) { p.pos.y = v; }); }).value;

	uiMSPT.posZ = b.addFloat(gMSPT, "Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.mspt, primaryIndex, [&](auto& p) { p.pos.z = v; }); }).value;

	uiMSPT.rotX = b.addFloat(gMSPT, "Rot.X", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.mspt, primaryIndex, [&](auto& p) { p.rot.x = v; }); }).value;

	uiMSPT.rotY = b.addFloat(gMSPT, "Rot.Y", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.mspt, primaryIndex, [&](auto& p) { p.rot.y = v; }); }).value;

	uiMSPT.rotZ = b.addFloat(gMSPT, "Rot.Z", 0, -131071, 131071, 0, 6, false, false,
		[&](float v) { applyToSelected(loadedKMP.mspt, primaryIndex, [&](auto& p) { p.rot.z = v; }); }).value;

	uiMSPT.id = b.addNumber(gMSPT, "ID", 0, 0, 65535, 0, 6, false, false,
		[&](int v) { applyToSelected(loadedKMP.mspt, primaryIndex, [&](auto& p) { p.id = v; }); }).value;

	ctxKTPT.ui = uiKTPT;
	ctxENPT.ui = uiENPT;
	ctxITPT.ui = uiITPT;
	ctxJGPT.ui = uiJGPT;
	ctxAREA.ui = uiAREA; 
	ctxCAME.ui = uiCAME;
	ctxCNPT.ui = uiCNPT;
	ctxPOTI.ui = uiPOTI;
	ctxCKPT.ui = uiCKPT;
	ctxGOBJ.ui = uiGOBJ;
	ctxMSPT.ui = uiMSPT;
}
void RenderContext::DrawUIBase(float x, float y, float w, float h, const glm::vec4& color, float z)
{
	float verts[12] = {
		x,     y,     z,
		x + w, y,     z,
		x + w, y + h, z,
		x,     y + h, z
	};

	glUseProgram(uiProgram);

	glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, glm::value_ptr(uiProj));
	glUniform4fv(uColorLoc, 1, glm::value_ptr(color));

	glBindVertexArray(uiVAO);
	glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
bool RenderContext::RayPlaneIntersection(
	glm::vec3 ro, glm::vec3 rd,
	glm::vec3 p0, glm::vec3 n,
	glm::vec3& out)
{
	float denom = glm::dot(n, rd);
	if (abs(denom) < 1e-6) return false;

	float t = glm::dot(p0 - ro, n) / denom;
	if (t < 0) return false;

	out = ro + rd * t;
	return true;
}
bool RenderContext::allEqualPosX() {
	float v = loadedKMP.ktpt[primaryIndex].pos.x;
	for (auto& p : loadedKMP.ktpt)
		if (p.selected && p.pos.x != v)
			return false;
	return true;
}
bool RenderContext::allEqualPosY() {
	float v = loadedKMP.ktpt[primaryIndex].pos.x;
	for (auto& p : loadedKMP.ktpt)
		if (p.selected && p.pos.x != v)
			return false;
	return true;
}
bool RenderContext::allEqualPosZ() {
	float v = loadedKMP.ktpt[primaryIndex].pos.x;
	for (auto& p : loadedKMP.ktpt)
		if (p.selected && p.pos.x != v)
			return false;
	return true;
}
bool RenderContext::allEqualRotX() {
	float v = loadedKMP.ktpt[primaryIndex].pos.x;
	for (auto& p : loadedKMP.ktpt)
		if (p.selected && p.pos.x != v)
			return false;
	return true;
}
bool RenderContext::allEqualRotY() {
	float v = loadedKMP.ktpt[primaryIndex].pos.x;
	for (auto& p : loadedKMP.ktpt)
		if (p.selected && p.pos.x != v)
			return false;
	return true;
}
bool RenderContext::allEqualRotZ() {
	float v = loadedKMP.ktpt[primaryIndex].pos.x;
	for (auto& p : loadedKMP.ktpt)
		if (p.selected && p.pos.x != v)
			return false;
	return true;
}
bool RenderContext::RayTriangleIntersection(
	const glm::vec3& orig,
	const glm::vec3& dir,
	const glm::vec3& v0,
	const glm::vec3& v1,
	const glm::vec3& v2,
	glm::vec3& outHit)
{
	const float EPSILON = 1e-6f;

	glm::vec3 edge1 = v1 - v0;
	glm::vec3 edge2 = v2 - v0;

	glm::vec3 h = glm::cross(dir, edge2);
	float a = glm::dot(edge1, h);

	if (fabs(a) < EPSILON)
		return false;

	float f = 1.0f / a;
	glm::vec3 s = orig - v0;
	float u = f * glm::dot(s, h);

	if (u < 0.0f || u > 1.0f)
		return false;

	glm::vec3 q = glm::cross(s, edge1);
	float v = f * glm::dot(dir, q);

	if (v < 0.0f || u + v > 1.0f)
		return false;

	float t = f * glm::dot(edge2, q);

	if (t > EPSILON) {
		outHit = orig + dir * t;
		return true;
	}

	return false;
}

bool RenderContext::RaycastKCL(
	const glm::vec3& origin,
	const glm::vec3& dir,
	glm::vec3& outHit)
{
	float minDist = FLT_MAX;
	bool hitFound = false;

	for (const auto& tri : kclRenderer.mKCLs) {

		glm::vec3 hit;
		if (RayTriangleIntersection(origin, dir, tri.v1, tri.v2, tri.v3, hit)) {

			float dist = glm::distance(origin, hit);
			if (dist < minDist) {
				minDist = dist;
				outHit = hit;
				hitFound = true;
			}
		}
	}

	return hitFound;
}
int maskToIndex(uint8_t mask) {
	switch (mask) {
	case 0x00: return 0;
	case 0x80: return 1;
	case 0x40: return 2;
	case 0x20: return 3;
	case 0x10: return 4;
	}
	return 0;
}
template <typename T>
void RenderContext::ApplyTextEditToPoint(SectionContext<T>& ctx)
{
	if (primaryIndex < 0) return;
	if (!editingItem) return;

	auto& p = (*ctx.data)[primaryIndex];
	UIItem* item = editingItem;

	try {
		if (!editingItem->draggingValue)
		{
			float v = std::stof(item->editBuffer);

			if (item == ctx.ui.posX) p.pos.x = v;
			if (item == ctx.ui.posY) p.pos.y = v;
			if (item == ctx.ui.posZ) p.pos.z = v;

			if constexpr (requires { p.rot; }) {
				if (item == ctx.ui.rotX) p.rot.x = v;
				if (item == ctx.ui.rotY) p.rot.y = v;
				if (item == ctx.ui.rotZ) p.rot.z = v;
			}
			if constexpr (requires { p.scale; }) {
				if (item == ctx.ui.scaleX) p.scale.x = v;
				if (item == ctx.ui.scaleY) p.scale.y = v;
				if (item == ctx.ui.scaleZ) p.scale.z = v;
			}
			if constexpr (requires { p.viewStart; }) {
				if (item == ctx.ui.viewStartX) p.viewStart.x = v;
				if (item == ctx.ui.viewStartY) p.viewStart.y = v;
				if (item == ctx.ui.viewStartZ) p.viewStart.z = v;
			}
			if constexpr (requires { p.viewEnd; }) {
				if (item == ctx.ui.viewEndX) p.viewEnd.x = v;
				if (item == ctx.ui.viewEndY) p.viewEnd.y = v;
				if (item == ctx.ui.viewEndZ) p.viewEnd.z = v;
			}
			if constexpr (requires { p.time; }) {
				if (item == ctx.ui.time) p.time = v;
			}
			if constexpr (requires { p.zoomStart; }) {
				if (item == ctx.ui.zoomStart) p.zoomStart = v;
			}
			if constexpr (requires { p.zoomEnd; }) {
				if (item == ctx.ui.zoomEnd) p.zoomEnd = v;
			}
			if constexpr (requires { p.shake; }) {
				if (item == ctx.ui.shake) p.shake= (int)v;
			}
			if constexpr (requires { p.nextCam; }) {
				if (item == ctx.ui.nextCam) p.nextCam = (int)v;
			}
			if constexpr (requires { p.id; }) {
				if (item == ctx.ui.id) p.id = (int)v;
			}
			if constexpr (requires { p.shape; }) {
				if (item == ctx.ui.shape) p.shape = (int)v;
			}
			if constexpr (requires { p.type; }) {
				if (item == ctx.ui.type) p.type = (int)v;
			}
			if constexpr (requires { p.deviation; }) {
				if (item == ctx.ui.deviation) p.deviation = v;
			}
			if constexpr (requires { p.setting1; }) {
				if (item == ctx.ui.setting1) p.s1 = (int)v;
			}
			if constexpr (requires { p.setting2; }) {
				if (item == ctx.ui.setting2) p.setting2 = (int)v;
			}
			if constexpr (requires { p.s1; }) {
				if (item == ctx.ui.setting1 || item == ctx.ui.setting6 || item == ctx.ui.setting7 || item == ctx.ui.setting8 || item == ctx.ui.setting9 || item == ctx.ui.setting10 || ctx.ui.coobStartCPIndex || ctx.ui.cobjUnloadKCLFlag || ctx.ui.windAngle || ctx.ui.antiGravity || ctx.ui.teleportTime || ctx.ui.airRingBoostTime) p.s1 = (int)v;
			}
			if constexpr (requires { p.s2; }) {
				if (item == ctx.ui.setting2 || item == ctx.ui.setting5 || ctx.ui.coobEndCPIndex || ctx.ui.cobjUnloadFrame1 || ctx.ui.windPower || ctx.ui.gravityScale || ctx.ui.teleportAngle) p.s2 = (int)v;
			}
			if constexpr (requires { p.pad; }) {
				if (item == ctx.ui.railRidingRotation || ctx.ui.cobjUnloadGroup || ctx.ui.gravityType) p.pad = (int)v;
			}
			if constexpr (requires { p.route; }) {
				if (item == ctx.ui.setting11 || ctx.ui.routeIndex || ctx.ui.route) p.route = (int)v;
			}
			if constexpr (requires { p.s3; }) {
				if (item == ctx.ui.setting3) p.s3 = maskToIndex(v);
			}
			if constexpr (requires { p.sound; }) {
				if (item == ctx.ui.soundTrig) p.sound = (int)v;
			}
			item->floatValue = v;
		}
		else
		{
			double gx, gy;
			glfwGetCursorPos(gMain->mWindow, &gx, &gy);

			HandleMouseWrap();

			glfwGetCursorPos(gMain->mWindow, &gx, &gy);

			float mouseY = (float)gy;

			float dy = mouseY - editingItem->dragStartY;
			float delta = dy * 0.04f;
			float v = editingItem->dragStartValue + delta;
			auto& p = (*ctx.data)[primaryIndex]; 

			if (item == ctx.ui.posX) p.pos.x = v;
			if (item == ctx.ui.posY) p.pos.y = v;
			if (item == ctx.ui.posZ) p.pos.z = v;

			if constexpr (requires { p.rot; }) {
				if (item == ctx.ui.rotX) p.rot.x = v;
				if (item == ctx.ui.rotY) p.rot.y = v;
				if (item == ctx.ui.rotZ) p.rot.z = v;
			}
			if constexpr (requires { p.scale; }) {
				if (item == ctx.ui.scaleX) p.scale.x = v;
				if (item == ctx.ui.scaleY) p.scale.y = v;
				if (item == ctx.ui.scaleZ) p.scale.z = v;
			}
			if constexpr (requires { p.viewStart; }) {
				if (item == ctx.ui.viewStartX) p.viewStart.x = v;
				if (item == ctx.ui.viewStartY) p.viewStart.y = v;
				if (item == ctx.ui.viewStartZ) p.viewStart.z = v;
			}
			if constexpr (requires { p.viewEnd; }) {
				if (item == ctx.ui.viewEndX) p.viewEnd.x = v;
				if (item == ctx.ui.viewEndY) p.viewEnd.y = v;
				if (item == ctx.ui.viewEndZ) p.viewEnd.z = v;
			}
			if constexpr (requires { p.time; }) {
				if (item == ctx.ui.time) p.time = v;
			}
			if constexpr (requires { p.zoomStart; }) {
				if (item == ctx.ui.zoomStart) p.zoomStart = v;
			}
			if constexpr (requires { p.zoomEnd; }) {
				if (item == ctx.ui.zoomEnd) p.zoomEnd = v;
			}
			if constexpr (requires { p.shake; }) {
				if (item == ctx.ui.shake) p.shake = (int)v;
			}
			if constexpr (requires { p.nextCam; }) {
				if (item == ctx.ui.nextCam) p.nextCam = (int)v;
			}
			if constexpr (requires { p.id; }) {
				if (item == ctx.ui.id) p.id = (int)v;
			}
			if constexpr (requires { p.shape; }) {
				if (item == ctx.ui.shape) p.shape = (int)v;
			}
			if constexpr (requires { p.type; }) {
				if (item == ctx.ui.type) p.type = (int)v;
			}
			if constexpr (requires { p.deviation; }) {
				if (item == ctx.ui.deviation) p.deviation = v;
			}
			if constexpr (requires { p.setting1; }) {
				if (item == ctx.ui.setting1) p.s1 = (int)v;
			}
			if constexpr (requires { p.setting2; }) {
				if (item == ctx.ui.setting2) p.setting2 = (int)v;
			}
			if constexpr (requires { p.s1; }) {
				if (item == ctx.ui.setting1 || item == ctx.ui.setting6 || item == ctx.ui.setting7 || item == ctx.ui.setting8 || item == ctx.ui.setting9 || item == ctx.ui.setting10 || ctx.ui.coobStartCPIndex || ctx.ui.cobjUnloadKCLFlag || ctx.ui.windAngle || ctx.ui.antiGravity || ctx.ui.teleportTime || ctx.ui.airRingBoostTime) p.s1 = (int)v;
			}
			if constexpr (requires { p.s2; }) {
				if (item == ctx.ui.setting2 || item == ctx.ui.setting5 || ctx.ui.coobEndCPIndex || ctx.ui.cobjUnloadFrame1 || ctx.ui.windPower || ctx.ui.gravityScale || ctx.ui.teleportAngle) p.s2 = (int)v;
			}
			if constexpr (requires { p.pad; }) {
				if (item == ctx.ui.railRidingRotation || ctx.ui.cobjUnloadGroup || ctx.ui.gravityType) p.pad = (int)v;
			}
			if constexpr (requires { p.s3; }) {
				if (item == ctx.ui.setting3) p.s3 = maskToIndex(v);
			}
			if constexpr (requires { p.route; }) {
				if (item == ctx.ui.setting11 || ctx.ui.routeIndex || ctx.ui.route) p.route = (int)v;
			}
			if constexpr (requires { p.sound; }) {
				if (item == ctx.ui.soundTrig) p.sound = (int)v;
			}
			editingItem->dragStartValue = v; 
			editingItem->dragStartY = mouseY;
		}
	}
	catch (...) {

	}
}


template <typename T>
void RenderContext::ProcessSectionEditing(
	SectionContext<T>& ctx,
	int& primaryIndex,
	int& anchorIndex,
	int& hoverIndex,
	std::vector<int>& dragIndices,
	std::vector<glm::vec3>& dragStartPositions,
	glm::vec3& dragStartPrimaryPos,
	glm::mat4 projection,
	glm::mat4 view,
	float vx, float vy, float vw, float vh,
	bool altDown)
{
	if (Input::GetMousePosition().x < mUIWidth) return;
	auto editable = MakeEditable(*ctx.data);

	hoverIndex = ComputeHoverIndex(
		editable, projection, view,
		vx, vy, vw, vh,
		Input::GetMousePosition(),
		25.f
	);

	if (altDown && Input::GetMousePosition().x > mUIWidth && Input::GetMouseButtonDown(0)) {

		glm::vec3 hit;
		if (RaycastOrPlane(hit)) {

			UIItemState beforeUI = ctx.ui.posX->toState();
			kmp::KMP beforeKMP = loadedKMP;

			primaryIndex = AddPointAtClick(
				*ctx.data,
				hit,
				hoverIndex,
				primaryIndex,
				ctx.createNew
			);
			anchorIndex = primaryIndex;

			auto editable2 = MakeEditable(*ctx.data);
			BeginDragSelection(primaryIndex, editable2, dragIndices, dragStartPositions, dragStartPrimaryPos);

			if (ctx.type == SectionType::CAME) {
				std::string name = "Camera " + std::to_string(primaryIndex);
				ctxCAME.ui.nextCam->listValues.push_back(name);
				ctxCAME.ui.setting8->listValues.push_back(name);
				ctxAREA.ui.cameraIndex->listValues.push_back(name);
			}
			if (ctx.type == SectionType::CNPT) {
				std::string name = "Cannon " + std::to_string(primaryIndex);
				ctxAREA.ui.teleportAngle->listValues.push_back(name);
			}
			UpdateUIFromSelection(ctx.ui, editable2, primaryIndex);

			UIItemState afterUI = ctx.ui.posX->toState();
			kmp::KMP afterKMP = loadedKMP;

			pushHistory(ctx.ui.posX->id, beforeKMP, afterKMP, beforeUI, afterUI);
		}

	}

	if (Input::GetKeyDown(GLFW_KEY_DELETE)) {

		UIItemState beforeUI = ctx.ui.posX->toState();
		kmp::KMP beforeKMP = loadedKMP;

		if (ctx.type == SectionType::CAME) {
			auto deleted = DeleteSelected(loadedKMP.came);

			for (int idx : deleted) {
				cameraList.erase(cameraList.begin() + idx);
				auto fixIndex = [&](int& v) {
					if (v < 0 || v >= (int)cameraList.size()) {
						v = 0;
					}
				};
				fixIndex(ctxCAME.ui.nextCam->listIndex);
				fixIndex(ctxCAME.ui.setting8->listIndex);
				fixIndex(ctxAREA.ui.cameraIndex->listIndex);
				ctxCAME.ui.nextCam->listValues.erase(ctxCAME.ui.nextCam->listValues.begin() + idx);
				ctxCAME.ui.setting8->listValues.erase(ctxCAME.ui.setting8->listValues.begin() + idx);
				ctxAREA.ui.cameraIndex->listValues.erase(ctxAREA.ui.cameraIndex->listValues.begin() + idx);
			}
		}
		else if (ctx.type == SectionType::CNPT) {
				auto deleted = DeleteSelected(loadedKMP.cnpt);

				for (int idx : deleted) {
					cannonList.erase(cannonList.begin() + idx);
					auto fixIndex = [&](int& v) {
						if (v < 0 || v >= (int)cannonList.size()) {
							v = 0;
						}
					};
					fixIndex(ctxAREA.ui.teleportAngle->listIndex);
					ctxAREA.ui.teleportAngle->listValues.erase(ctxAREA.ui.teleportAngle->listValues.begin() + idx);
				}
			} 
		else
		{
			DeleteSelected(*ctx.data);
		}

		primaryIndex = -1;
		anchorIndex = -1;

		auto editable2 = MakeEditable(*ctx.data);
		UpdateUIFromSelection(ctx.ui, editable2, primaryIndex);

		UIItemState afterUI = ctx.ui.posX->toState();
		kmp::KMP afterKMP = loadedKMP;

		pushHistory(ctx.ui.posX->id, beforeKMP, afterKMP, beforeUI, afterUI);
	}

	if ((Input::GetKey(GLFW_KEY_LEFT_CONTROL) || Input::GetKey(GLFW_KEY_RIGHT_CONTROL)) &&
		Input::GetKeyDown(GLFW_KEY_A)) {

		UIItemState beforeUI = ctx.ui.posX->toState();
		kmp::KMP beforeKMP = loadedKMP;

		SelectAll(*ctx.data);
		primaryIndex = ctx.data->size() - 1;
		anchorIndex = primaryIndex;

		UIItemState afterUI = ctx.ui.posX->toState();
		kmp::KMP afterKMP = loadedKMP;

		pushHistory(ctx.ui.posX->id, beforeKMP, afterKMP, beforeUI, afterUI);

	}

	if (Input::GetMouseButtonDown(0)) {

		clickedIndex = hoverIndex;

		wasSelectedBeforeDown.clear();
		wasSelectedBeforeDown.reserve(ctx.data->size());
		for (auto& p : *ctx.data)
			wasSelectedBeforeDown.push_back(p.selected);

		if (hoverIndex >= 0) {
			bool wasSelected = wasSelectedBeforeDown[hoverIndex];

			if (!wasSelected) {
				HandleClickSelection(
					editable,
					hoverIndex,
					primaryIndex,
					anchorIndex,
					(Input::GetKey(GLFW_KEY_LEFT_CONTROL) || Input::GetKey(GLFW_KEY_RIGHT_CONTROL))
				);
				auto editable2 = MakeEditable(*ctx.data);
				UpdateUIFromSelection(ctx.ui, editable2, primaryIndex);

			}
		}

		dragPending = true;
		dragStartMouse = Input::GetMousePosition();

		editable = MakeEditable(*ctx.data);
		if (primaryIndex >= 0)
			BeginDragSelection(primaryIndex, editable, dragIndices, dragStartPositions, dragStartPrimaryPos);
	}
	if (dragPending && Input::GetMouseButton(0)) {

		glm::vec2 now = Input::GetMousePosition();
		float dist = glm::distance(now, dragStartMouse);

		if (dist > dragThreshold) {
			dragBeforeUI = ctx.ui.posX->toState();
			dragBeforeKMP = loadedKMP;

			glm::vec3 rayOrigin = mCamera.GetPosition();
			glm::vec3 rayDir = ScreenToWorldRay(
				Input::GetMousePosition().x,
				Input::GetMousePosition().y,
				vx, vy, vw, vh,
				projection, view
			);

			glm::vec3 hit;
			if (RayPlaneIntersection(rayOrigin, rayDir, dragStartPrimaryPos, glm::vec3(0, 1, 0), hit)) {
				dragPlanePoint = hit;
			}

			dragPending = false;
			editingDrag = true;
		}
	}
	if (editingDrag && Input::GetMouseButton(0)) {
		glm::vec3 rayOrigin = mCamera.GetPosition();
		glm::vec3 rayDir = ScreenToWorldRay(
			Input::GetMousePosition().x,
			Input::GetMousePosition().y,
			vx, vy, vw, vh,
			projection, view
		);

		glm::vec3 hit;
		if (RaycastKCL(rayOrigin, rayDir, hit) ||
			RayPlaneIntersection(rayOrigin, rayDir, dragPlanePoint, glm::vec3(0, 1, 0), hit)) {

			glm::vec3 delta = hit - dragStartPrimaryPos;

			auto& vec = *ctx.data;
			for (size_t i = 0; i < dragIndices.size(); ++i) {
				int idx = dragIndices[i];
				vec[idx].pos = dragStartPositions[i] + delta;
			}

			auto editable2 = MakeEditable(*ctx.data);
			UpdateUIFromSelection(ctx.ui, editable2, primaryIndex);

			gMain->dirty = true;
			std::string pathing = std::filesystem::path(mPath).filename().string();

			glfwSetWindowTitle(
				gMain->mWindow,
				("KMParse: Editing " + pathing + " (Unsaved)").c_str()
			);
		}
	}
	if (Input::GetMouseButtonUp(0)) {

		if (!editingDrag && dragPending) {

			if (clickedIndex < 0) {
				for (auto& p : *ctx.data)
					p.selected = false;
				primaryIndex = -1;
				anchorIndex = -1;
			}
			else {
				if (wasSelectedBeforeDown[clickedIndex]) {
					(*ctx.data)[clickedIndex].selected = false;

					if (clickedIndex == primaryIndex)
						primaryIndex = -1;

					anchorIndex = primaryIndex;

					auto editable2 = MakeEditable(*ctx.data);
					UpdateUIFromSelection(ctx.ui, editable2, primaryIndex);
				}
			}
		}

		dragPending = false;

		if (editingDrag) {
			editingDrag = false;

			UIItemState afterUI = ctx.ui.posX->toState();
			kmp::KMP afterKMP = loadedKMP;

			pushHistory(ctx.ui.posX->id, dragBeforeKMP, afterKMP, dragBeforeUI, afterUI);
		}
	}
}
void RenderContext::UpdateUIFromSelection(
	const PointUI& ui,
	const std::vector<EditablePoint>& pts,
	int primaryIndex)
{
	if (primaryIndex < 0 || primaryIndex >= pts.size())
		return;

	glm::vec3 p = *pts[primaryIndex].pos;
	glm::vec3 r = pts[primaryIndex].rot ? *pts[primaryIndex].rot : glm::vec3(0);

	auto allEqual = [&](auto getter) {
		bool first = true;
		using V = decltype(getter(pts[primaryIndex]));
		V v{};

		for (auto& e : pts) {
			if (!*e.selected) continue;

			if (first) {
				v = getter(e);
				first = false;
			}
			else {
				if (getter(e) != v)
					return false;
			}
		}
		return !first;
	};


	auto setField = [&](UIItem* item, float v, bool eq) {
		if (!item) return;
		if (eq) {
			item->floatValue = v;
			item->editBuffer = std::format("{:.6f}", v);
		}
		else {
			item->editBuffer.clear();
		}
	};

	auto setField2 = [&](UIItem* item, int v, bool eq) {
		if (!item) return;
		if (eq) {
			item->numberValue = v;
			item->editBuffer = std::to_string(v);
		}
		else {
			item->editBuffer.clear();
		}
	};

	setField(ui.posX, p.x, allEqual([](auto& e) { return e.pos->x; }));
	setField(ui.posY, p.y, allEqual([](auto& e) { return e.pos->y; }));
	setField(ui.posZ, p.z, allEqual([](auto& e) { return e.pos->z; }));

	if (pts[primaryIndex].rot) {
		setField(ui.rotX, r.x, allEqual([](auto& e) { return e.rot ? e.rot->x : 0; }));
		setField(ui.rotY, r.y, allEqual([](auto& e) { return e.rot ? e.rot->y : 0; }));
		setField(ui.rotZ, r.z, allEqual([](auto& e) { return e.rot ? e.rot->z : 0; }));
	}

	if (pts[primaryIndex].scale) {
		glm::vec3 s = *pts[primaryIndex].scale;

		bool eqX = allEqual([](auto& e) { return e.scale->x; });
		bool eqY = allEqual([](auto& e) { return e.scale->y; });
		bool eqZ = allEqual([](auto& e) { return e.scale->z; });

		setField(ui.scaleX, s.x, eqX);
		setField(ui.scaleY, s.y, eqY);
		setField(ui.scaleZ, s.z, eqZ);
	}

	if (ui.id && pts[primaryIndex].id) {
		int v = static_cast<int>(*pts[primaryIndex].id);
		bool eq = allEqual([](auto& e) { return static_cast<int>(*e.id); });
		setField2(ui.id, v, eq);
	}

	if (ui.soundTrig && pts[primaryIndex].sound) {
		int v = *pts[primaryIndex].sound;
		bool eq = allEqual([](auto& e) { return *e.sound; });
		setField2(ui.soundTrig, v, eq);
	}

	if (ui.type && pts[primaryIndex].type) {
		int v = *pts[primaryIndex].type;
		bool eq = allEqual([](auto& e) { return *e.type; });
		if (eq)
		{
			ui.type->numberValue = v;
			ui.type->listIndex = v;
		}
		else ui.type->editBuffer.clear();
	}
	if (ui.shape && pts[primaryIndex].shape) {
		int v = *pts[primaryIndex].shape;
		bool eq = allEqual([](auto& e) { return *e.shape; });
		if (eq)
		{
			ui.shape->numberValue = v;
			ui.shape->listIndex = v;
		}
		else ui.shape->editBuffer.clear();
	}
	if (ui.cameraIndex && pts[primaryIndex].cameraIndex) {
		int v = *pts[primaryIndex].cameraIndex;
		bool eq = allEqual([](auto& e) { return *e.cameraIndex; });
		if (eq)
		{
			ui.cameraIndex->numberValue = v;
			ui.cameraIndex->listIndex = v;
		}
		else ui.cameraIndex->editBuffer.clear();
	}
	if (ui.setting3 && pts[primaryIndex].s3) {
		int v = *pts[primaryIndex].s3;
		bool eq = allEqual([](auto& e) { return *e.s3; });
		if (eq)
		{
			ui.setting3->numberValue = v;
			ui.setting3->listIndex = v;
		}
		else ui.setting3->editBuffer.clear();
	}
	if (ui.routeIndex && pts[primaryIndex].routeIndex) {
		int v = *pts[primaryIndex].routeIndex;
		bool eq = allEqual([](auto& e) { return *e.routeIndex; });
		if (eq)
		{
			ui.routeIndex->numberValue = v;
			ui.routeIndex->listIndex = v;

			uint8_t flags = loadedKMP.area[primaryIndex].route;

			uint8_t loadTypeIndex = flags & 0b00000011;
			uint8_t applyToIndex = (flags >> 2) & 0b00000001;
			uint8_t lapIndex = (flags >> 3) & 0b00000111;

			if (ui.cobjUnloadSetting)
			{
				ui.cobjUnloadSetting->listIndex = loadTypeIndex;
				ui.cobjUnloadSetting->numberValue = loadTypeIndex;
			}

			if (ui.cobjUnloadSetting2)
			{
				ui.cobjUnloadSetting2->listIndex = applyToIndex;
				ui.cobjUnloadSetting2->numberValue = applyToIndex;
			}

			if (ui.cobjUnloadSetting3)
			{
				ui.cobjUnloadSetting3->listIndex = lapIndex;
				ui.cobjUnloadSetting3->numberValue = lapIndex;
			}

			if (ui.type && pts[primaryIndex].type && *pts[primaryIndex].type == 10)
			{
				if (v == 1 || v == 0xFF)
				{
					ctxAREA.ui.setting11->checked = true;
					if (v == 1)
					{
						ctxAREA.ui.setting4->listIndex = 0;
						ctxAREA.ui.setting4->numberValue = 0;
					}
					else
					{
						ctxAREA.ui.setting4->listIndex = 1;
						ctxAREA.ui.setting4->numberValue = 1;
					}
				}
				else
				{
					ctxAREA.ui.setting4->checked = false;
				}
			}
		}
		else ui.routeIndex->editBuffer.clear();
	}
	if ((ui.enemyIndex || ui.cobjUnloadFrame2) && pts[primaryIndex].enemyIndex) {
		int v = *pts[primaryIndex].enemyIndex;
		bool eq = allEqual([](auto& e) { return *e.enemyIndex; });
		if (eq)
		{
			if (ui.enemyIndex) ui.enemyIndex->numberValue = ui.enemyIndex->listIndex = v;
			if (ui.cobjUnloadFrame2) ui.cobjUnloadFrame2->numberValue = ui.cobjUnloadFrame2->listIndex = v;
		}
		else
		{
			if (ui.enemyIndex) ui.enemyIndex->editBuffer.clear();
			if (ui.cobjUnloadFrame2) ui.cobjUnloadFrame2->editBuffer.clear();
		}
	}
	if ((ui.setting1 || ui.setting6 || ui.setting7 || ui.setting8 || ui.setting9 ||
		ui.setting10 || ui.coobStartCPIndex || ui.cobjUnloadKCLFlag || ui.windAngle || ui.airRingBoostTime || ui.teleportTime || ui.antiGravity) && pts[primaryIndex].s1)
	{
		int v = *pts[primaryIndex].s1;
		bool eq = allEqual([](auto& e) { return *e.s1; });

		if (eq)
		{
			if (ui.setting1) ui.setting1->numberValue = ui.setting1->listIndex = v;
			if (ui.setting6) ui.setting6->numberValue = ui.setting6->listIndex = v;
			if (ui.setting7) ui.setting7->numberValue = ui.setting7->listIndex = v;
			if (ui.setting8) ui.setting8->numberValue = ui.setting8->listIndex = v;
			if (ui.setting9) ui.setting9->numberValue = ui.setting9->listIndex = v;
			if (ui.setting10) ui.setting10->numberValue = ui.setting10->listIndex = v;
			if (ui.windAngle) ui.windAngle->numberValue = ui.windAngle->listIndex = v;
			if (ui.airRingBoostTime) ui.airRingBoostTime->numberValue = ui.airRingBoostTime->listIndex = v;
			if (ui.teleportTime) ui.teleportTime->numberValue = ui.teleportTime->listIndex = v;
			if (ui.antiGravity) ui.antiGravity->numberValue = ui.antiGravity->listIndex = v;
			if (ui.cobjUnloadKCLFlag) ui.cobjUnloadKCLFlag->numberValue = ui.cobjUnloadKCLFlag->listIndex = v;
			if (ui.coobStartCPIndex) ui.coobStartCPIndex->numberValue = ui.coobStartCPIndex->listIndex = v;
		}
		else {
			if (ui.setting1) ui.setting1->editBuffer.clear();
			if (ui.setting6) ui.setting6->editBuffer.clear();
			if (ui.setting7) ui.setting7->editBuffer.clear();
			if (ui.setting8) ui.setting8->editBuffer.clear();
			if (ui.setting9) ui.setting9->editBuffer.clear();
			if (ui.setting10) ui.setting10->editBuffer.clear();
			if (ui.windAngle) ui.windAngle->editBuffer.clear();
			if (ui.airRingBoostTime) ui.airRingBoostTime->editBuffer.clear();
			if (ui.teleportTime) ui.teleportTime->editBuffer.clear();
			if (ui.antiGravity) ui.antiGravity->editBuffer.clear();
			if (ui.cobjUnloadKCLFlag) ui.cobjUnloadKCLFlag->editBuffer.clear();
			if (ui.coobStartCPIndex) ui.coobStartCPIndex->editBuffer.clear();
		}
	}
	if ((ui.railRidingRotation || ui.cobjUnloadGroup || ui.gravityType) &&pts[primaryIndex].pad)
	{
		int v = *pts[primaryIndex].pad;
		bool eq = allEqual([](auto& e) { return *e.pad; });

		if (eq)
		{
			if (ui.cobjUnloadGroup) ui.cobjUnloadGroup->numberValue = ui.cobjUnloadGroup->listIndex = v;
			if (ui.gravityType) ui.gravityType->numberValue = ui.gravityType->listIndex = v;
			if (ui.railRidingRotation) ui.railRidingRotation->checked = !v;
		}
		else {
			if (ui.gravityType) ui.gravityType ->editBuffer.clear();
			if (ui.cobjUnloadGroup) ui.cobjUnloadGroup->editBuffer.clear();
		}
	}
	if ((ui.setting2 || ui.setting12  || ui.setting5 || ui.coobEndCPIndex || ui.cobjUnloadFrame1 || ui.windPower || ui.gravityScale || ui.teleportAngle)
		&& pts[primaryIndex].s2)
	{
		int v = *pts[primaryIndex].s2;
		bool eq = allEqual([](auto& e) { return *e.s2; });

		if (eq)
		{
			if (ui.setting2) ui.setting2->numberValue = ui.setting2->listIndex = v;
			if (ui.setting12) ui.setting12->numberValue = ui.setting12->listIndex = v;
			if (ui.setting5) ui.setting5->numberValue = ui.setting5->listIndex = v;
			if (ui.cobjUnloadFrame1) ui.cobjUnloadFrame1->numberValue = ui.cobjUnloadFrame1->listIndex = v;
			if (ui.windPower) ui.windPower->numberValue = ui.windPower->listIndex = v;
			if (ui.gravityScale) ui.gravityScale->numberValue = ui.gravityScale->listIndex = v;
			if (ui.teleportAngle) ui.teleportAngle->numberValue = ui.teleportAngle->listIndex = v;
			if (ui.coobEndCPIndex) ui.coobEndCPIndex->numberValue = ui.coobEndCPIndex->listIndex = v;
		}
		else {
			if (ui.setting2) ui.setting2->editBuffer.clear();
			if (ui.setting12) ui.setting12->editBuffer.clear();
			if (ui.setting5) ui.setting5->editBuffer.clear();
			if (ui.coobEndCPIndex) ui.coobEndCPIndex->editBuffer.clear();
			if (ui.cobjUnloadFrame1) ui.cobjUnloadFrame1->editBuffer.clear();
			if (ui.windPower) ui.windPower->editBuffer.clear();
			if (ui.gravityScale) ui.gravityScale->editBuffer.clear();
			if (ui.teleportAngle) ui.teleportAngle->editBuffer.clear();
		}
	}
	if (ui.zoomStart && pts[primaryIndex].zoomStart) {
		float v = *pts[primaryIndex].zoomStart;
		bool eq = allEqual([](auto& e) { return *e.zoomStart; });
		setField(ui.zoomStart, v, eq);
	}
	if (ui.zoomEnd && pts[primaryIndex].zoomEnd) {
		float v = *pts[primaryIndex].zoomEnd;
		bool eq = allEqual([](auto& e) { return *e.zoomEnd; });
		setField(ui.zoomEnd, v, eq);
	}
	if (ui.viewStartX && pts[primaryIndex].viewStart) {
		glm::vec3 v = *pts[primaryIndex].viewStart;

		bool eqX = allEqual([](auto& e) { return e.viewStart->x; });
		bool eqY = allEqual([](auto& e) { return e.viewStart->y; });
		bool eqZ = allEqual([](auto& e) { return e.viewStart->z; });

		setField(ui.viewStartX, v.x, eqX);
		setField(ui.viewStartY, v.y, eqY);
		setField(ui.viewStartZ, v.z, eqZ);
	}
	if (ui.viewEndX && pts[primaryIndex].viewEnd) {
		glm::vec3 v = *pts[primaryIndex].viewEnd;

		bool eqX = allEqual([](auto& e) { return e.viewEnd->x; });
		bool eqY = allEqual([](auto& e) { return e.viewEnd->y; });
		bool eqZ = allEqual([](auto& e) { return e.viewEnd->z; });

		setField(ui.viewEndX, v.x, eqX);
		setField(ui.viewEndY, v.y, eqY);
		setField(ui.viewEndZ, v.z, eqZ);
	}

	if (ui.type)
		SetAreaItem(ui.type);

}

void RenderContext::BeginDragSelection(
	int primaryIndex,
	const std::vector<EditablePoint>& pts,
	std::vector<int>& dragIndices,
	std::vector<glm::vec3>& dragStartPositions,
	glm::vec3& dragStartPrimaryPos)
{
	dragIndices.clear();
	dragStartPositions.clear();

	for (int i = 0; i < pts.size(); i++) {
		if (*pts[i].selected) {
			dragIndices.push_back(i);
			dragStartPositions.push_back(*pts[i].pos);
		}
	}

	if (dragIndices.empty()) {
		dragIndices.push_back(primaryIndex);
		dragStartPositions.push_back(*pts[primaryIndex].pos);
	}

	dragStartPrimaryPos = *pts[primaryIndex].pos;
}
int RenderContext::ComputeHoverIndex(
	const std::vector<EditablePoint>& pts,
	const glm::mat4& projection,
	const glm::mat4& view,
	float vx, float vy, float vw, float vh,
	glm::vec2 mouse,
	float hoverRadius)
{
	int hoverIndex = -1;
	float minDist = 999999.0f;

	for (int i = 0; i < pts.size(); i++) {
		glm::vec4 clip = projection * view * glm::vec4(*pts[i].pos, 1.0f);
		if (clip.w <= 0.0f) continue;

		glm::vec3 ndc = glm::vec3(clip) / clip.w;

		glm::vec2 screen;
		screen.x = vx + (ndc.x * 0.5f + 0.5f) * vw;
		screen.y = vy + (1.0f - (ndc.y * 0.5f + 0.5f)) * vh;

		float dist = glm::distance(mouse, screen);
		if (dist < hoverRadius && dist < minDist) {
			hoverIndex = i;
			minDist = dist;
		}
	}
	return hoverIndex;
}
void RenderContext::HandleClickSelection(
	std::vector<EditablePoint>& pts,
	int hoverIndex,
	int& primaryIndex,
	int& anchorIndex,
	bool ctrlDown)
{
	if (hoverIndex < 0) {
		anchorIndex = -1;
		return;
	}

	if (ctrlDown) {
		*pts[hoverIndex].selected = !*pts[hoverIndex].selected;
		primaryIndex = hoverIndex;
		anchorIndex = hoverIndex;
	}
	else {
		for (auto& p : pts)
			*p.selected = false;

		*pts[hoverIndex].selected = true;
		primaryIndex = hoverIndex;
		anchorIndex = hoverIndex;
	}
}
void RenderContext::HandleMouseUp()
{
	for (auto& row : rows)
	{
		if (row.value && row.value->type == UIItemType::Button && row.value->pressed)
		{
			row.value->pressed = false;
		}
	}
	if (!editingItem)
		return;

	if (editingItem->draggingValue)
	{
		editingItem->draggingValue = false;

		UIItemState afterUI = editingItem->toState();
		kmp::KMP afterKMP = loadedKMP;

		pushHistory(
			editingItem->id,
			dragBeforeKMP,
			afterKMP,
			dragBeforeUI,
			afterUI
		);

		editingItem = nullptr;
		return;
	}
}



bool RenderContext::Update(float deltaTime)
{

	for (auto& row : rows)
	{
		float y = 10.0f + row.drawIndex * style.rowHeight - listScroll;

		UIItem* item = row.value;
		if (!item) continue;
		if (item->type != UIItemType::Button) continue;

		float uiScale = mUIWidth / 512.0f;

		float boxX = item->indent + style.labelOffsetX * uiScale;
		float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;
		float boxW = 340.0f * uiScale;
		float boxH = style.valueHeight;

		bool overBox =
			Input::GetMousePosition().x >= boxX && Input::GetMousePosition().x <= boxX + boxW &&
			Input::GetMousePosition().y >= boxY && Input::GetMousePosition().y <= boxY + boxH;

		item->hovered = overBox;
	}

	altDown = Input::GetKey(GLFW_KEY_LEFT_ALT) || Input::GetKey(GLFW_KEY_RIGHT_ALT);

	if (gMain->mView)
		mCamera.SetOrtho(true);
	else
		mCamera.SetOrtho(false);

	if (!editingItem)
		mCamera.Update(deltaTime);

	projection = mCamera.GetProjectionMatrix();
	view = mCamera.GetViewMatrix();

	if (Input::GetMouseButtonDown(0))
		HandleClick(Input::GetMousePosition().x, Input::GetMousePosition().y);

	if (Input::GetMouseButton(0))
		HandleMouseMove(Input::GetMousePosition().x, Input::GetMousePosition().y);


	if (areaItem != nullptr && areaItem->expanded)
	{
		UIItem* posX = ctxAREA.ui.posX;
		UIItem* posY = ctxAREA.ui.posY;
		UIItem* posZ = ctxAREA.ui.posZ;
		UIItem* rotX = ctxAREA.ui.rotX;
		UIItem* rotY = ctxAREA.ui.rotY;
		UIItem* rotZ = ctxAREA.ui.rotZ;
		UIItem* scaleX = ctxAREA.ui.scaleX;
		UIItem* scaleY = ctxAREA.ui.scaleY;
		UIItem* scaleZ = ctxAREA.ui.scaleZ;
		UIItem* type = ctxAREA.ui.type;
		UIItem* shape = ctxAREA.ui.shape;
		UIItem* priority = ctxAREA.ui.priority;
		UIItem* s0 = ctxAREA.ui.setting9;
		UIItem* s1 = ctxAREA.ui.setting10;
		UIItem* s2 = ctxAREA.ui.setting11;
		UIItem* s3 = ctxAREA.ui.setting12;
		UIItem* s4 = ctxAREA.ui.setting4;
		UIItem* s5 = ctxAREA.ui.setting5;
		UIItem* s6 = ctxAREA.ui.setting6;
		UIItem* s7 = ctxAREA.ui.setting7;
		UIItem* s8 = ctxAREA.ui.setting8;
		UIItem* s9 = ctxAREA.ui.railRidingRotation;
		UIItem* s11 = ctxAREA.ui.cobjUnloadGroup;
		UIItem* s12 = ctxAREA.ui.cobjUnloadKCLFlag;
		UIItem* s13 = ctxAREA.ui.cobjUnloadFrame1;
		UIItem* s14 = ctxAREA.ui.cobjUnloadSetting;
		UIItem* s24 = ctxAREA.ui.cobjUnloadSetting2;
		UIItem* s25 = ctxAREA.ui.cobjUnloadSetting3;
		UIItem* s15 = ctxAREA.ui.cobjUnloadFrame2;
		UIItem* s16 = ctxAREA.ui.airRingBoostTime;
		UIItem* s17 = ctxAREA.ui.teleportTime;
		UIItem* s18 = ctxAREA.ui.teleportAngle;
		UIItem* s19 = ctxAREA.ui.gravityType;
		UIItem* s20 = ctxAREA.ui.antiGravity;
		UIItem* s21 = ctxAREA.ui.gravityScale;
		UIItem* s22 = ctxAREA.ui.windAngle;
		UIItem* s23 = ctxAREA.ui.windPower;
		UIItem* camera = ctxAREA.ui.cameraIndex;
		UIItem* route = ctxAREA.ui.routeIndex;
		UIItem* enemy = ctxAREA.ui.enemyIndex;
		UIItem* coob1 = ctxAREA.ui.coobStartCPIndex;
		UIItem* coob2 = ctxAREA.ui.coobEndCPIndex;

		if (!(posX && posY && posZ && rotX && rotY && rotZ && scaleX && scaleY && scaleZ && shape && type && priority && s0 && s1 && s2 && s3 && s4 && s5 && s6 && s7 && s8 && s9  && s11 && s12 && s13 && s14 && s15 && s16 && s17 && s18 &&s19 &&s20 &&s21 && s22 && s23 && s24 && s25 && camera && route && enemy && coob1 && coob2))
			return false;

		bool selectable = (primaryIndex >= 0);
		posX->selectable = selectable;
		posY->selectable = selectable;
		posZ->selectable = selectable;
		rotX->selectable = selectable;
		rotY->selectable = selectable;
		rotZ->selectable = selectable;
		scaleX->selectable = selectable;
		scaleY->selectable = selectable;
		scaleZ->selectable = selectable;
		type->selectable = selectable;
		shape->selectable = selectable;
		priority->selectable = selectable;
		s0->selectable = selectable;
		s1->selectable = selectable;
		s2->selectable = selectable;
		s3->selectable = selectable;
		s4->selectable = selectable;
		s5->selectable = selectable;
		s6->selectable = selectable;
		s7->selectable = selectable;
		s8->selectable = selectable;
		s9->selectable = selectable;
		s11->selectable = selectable;
		s12->selectable = selectable;
		s13->selectable = selectable;
		s14->selectable = selectable;
		s15->selectable = selectable;
		s16->selectable = selectable;
		s17->selectable = selectable;
		s18->selectable = selectable;
		s19->selectable = selectable;
		s20->selectable = selectable;
		s21->selectable = selectable;
		s22->selectable = selectable;
		s23->selectable = selectable;
		s24->selectable = selectable;
		s25->selectable = selectable;
		camera->selectable = selectable;
		route->selectable = selectable;
		enemy->selectable = selectable;
		coob1->selectable = selectable;
		coob2->selectable = selectable;

		ctxAREA.data = &loadedKMP.area;
		ctxAREA.type = SectionType::AREA;
		ctxAREA.createNew = CreateAREA;

		ctxAREA.ui.posX = posX;
		ctxAREA.ui.posY = posY;
		ctxAREA.ui.posZ = posZ;
		ctxAREA.ui.rotX = rotX;
		ctxAREA.ui.rotY = rotY;
		ctxAREA.ui.rotZ = rotZ;
		ctxAREA.ui.scaleX = scaleX;
		ctxAREA.ui.scaleY = scaleY;
		ctxAREA.ui.scaleZ = scaleZ;
		ctxAREA.ui.type = type;
		ctxAREA.ui.shape = shape;
		ctxAREA.ui.priority = priority;
		ctxAREA.ui.setting9 = s0;
		ctxAREA.ui.setting10 = s1;
		ctxAREA.ui.setting11 = s2;
		ctxAREA.ui.setting12 = s3;
		ctxAREA.ui.setting4 = s4;
		ctxAREA.ui.setting5 = s5;
		ctxAREA.ui.setting6 = s6;
		ctxAREA.ui.setting7 = s7;
		ctxAREA.ui.setting8 = s8;
		ctxAREA.ui.cameraIndex = camera;
		ctxAREA.ui.routeIndex = route;
		ctxAREA.ui.enemyIndex = enemy;
		ctxAREA.ui.coobStartCPIndex = coob1;
		ctxAREA.ui.coobEndCPIndex = coob2;

		ctxAREA.ui.railRidingRotation = s9;
		ctxAREA.ui.cobjUnloadGroup = s11;
		ctxAREA.ui.cobjUnloadKCLFlag = s12;
		ctxAREA.ui.cobjUnloadFrame1 = s13;
		ctxAREA.ui.cobjUnloadSetting = s14;
		ctxAREA.ui.cobjUnloadSetting2 = s24;
		ctxAREA.ui.cobjUnloadSetting3 = s25;
		ctxAREA.ui.cobjUnloadFrame2 = s15;
		ctxAREA.ui.airRingBoostTime = s16;
		ctxAREA.ui.teleportTime = s17;
		ctxAREA.ui.teleportAngle = s18;
		ctxAREA.ui.gravityType = s19;
		ctxAREA.ui.antiGravity = s20;
		ctxAREA.ui.gravityScale = s21;
		ctxAREA.ui.windAngle = s22;
		ctxAREA.ui.windPower = s23;

		ProcessSectionEditing(
			ctxAREA,
			primaryIndex,
			anchorIndex,
			hoverIndex,
			dragIndices,
			dragStartPositions,
			dragStartPrimaryPos,
			projection, view,
			vx, vy, vw, vh,
			altDown
		);
		ApplyTextEditToPoint(ctxAREA);
	}
	if (areaItem != nullptr && !areaItem->expanded)
		SetAreaItemInvisible();
		if (startPositionItem != nullptr && startPositionItem->expanded)
	{
		UIItem* posX = ctxKTPT.ui.posX;
		UIItem* posY = ctxKTPT.ui.posY;
		UIItem* posZ = ctxKTPT.ui.posZ;
		UIItem* rotX = ctxKTPT.ui.rotX;
		UIItem* rotY = ctxKTPT.ui.rotY;
		UIItem* rotZ = ctxKTPT.ui.rotZ;
		if (!(posX && posY && posZ && rotX && rotY && rotZ))
			return false;

		bool selectable = (primaryIndex >= 0);
		posX->selectable = selectable;
		posY->selectable = selectable;
		posZ->selectable = selectable;
		rotX->selectable = selectable;
		rotY->selectable = selectable;
		rotZ->selectable = selectable;

		ctxKTPT.data = &loadedKMP.ktpt;
		ctxKTPT.createNew = CreateKTPT;

		ctxKTPT.ui.posX = posX;
		ctxKTPT.ui.posY = posY;
		ctxKTPT.ui.posZ = posZ;
		ctxKTPT.ui.rotX = rotX;
		ctxKTPT.ui.rotY = rotY;
		ctxKTPT.ui.rotZ = rotZ;

		ProcessSectionEditing(
			ctxKTPT,
			primaryIndex,
			anchorIndex,
			hoverIndex,
			dragIndices,
			dragStartPositions,
			dragStartPrimaryPos,
			projection, view,
			vx, vy, vw, vh,
			altDown
		);
		ApplyTextEditToPoint(ctxKTPT);
	}

if (cameItem != nullptr && cameItem->expanded)
	{
		UIItem* posX = ctxCAME.ui.posX;
		UIItem* posY = ctxCAME.ui.posY;
		UIItem* posZ = ctxCAME.ui.posZ;
		UIItem* rotX = ctxCAME.ui.rotX;
		UIItem* rotY = ctxCAME.ui.rotY;
		UIItem* rotZ = ctxCAME.ui.rotZ;
		UIItem* aa= ctxCAME.ui.nextCam;
		UIItem* vv = ctxCAME.ui.route;
		UIItem* time = ctxCAME.ui.time;
		UIItem* vCam = ctxCAME.ui.vCam;
		UIItem* zoomStart = ctxCAME.ui.zoomStart;
		UIItem* zoomEnd = ctxCAME.ui.zoomEnd;
		UIItem* viewStartX = ctxCAME.ui.viewStartX;
		UIItem* viewStartY = ctxCAME.ui.viewStartY;
		UIItem* viewStartZ = ctxCAME.ui.viewStartZ;
		UIItem* viewEndX = ctxCAME.ui.viewEndX;
		UIItem* viewEndY = ctxCAME.ui.viewEndY;
		UIItem* viewEndZ = ctxCAME.ui.viewEndZ;
		UIItem* shake = ctxCAME.ui.shake;
		if (!(posX && posY && posZ && rotX && rotY && rotZ&& time && vCam && zoomStart && zoomEnd && aa && vv && viewStartX && viewStartY && viewStartZ&& viewEndX&& viewEndY&& viewEndZ && shake))
			return false;

		bool selectable = (primaryIndex >= 0);
		posX->selectable = selectable;
		posY->selectable = selectable;
		posZ->selectable = selectable;
		rotX->selectable = selectable;
		rotY->selectable = selectable;
		rotZ->selectable = selectable;
		time->selectable = selectable;
		vCam->selectable = selectable;
		aa->selectable = selectable;
		vv->selectable = selectable;
		zoomStart->selectable = selectable;
		zoomEnd->selectable = selectable;
		viewStartX->selectable = selectable;
		viewStartY->selectable = selectable;
		viewStartZ->selectable = selectable;
		viewEndX->selectable = selectable;
		viewEndY->selectable = selectable;
		viewEndZ->selectable = selectable;
		shake->selectable = selectable;

		ctxCAME.data = &loadedKMP.came;
		ctxCAME.type = SectionType::CAME;
		ctxCAME.createNew = CreateCAME;

		ctxCAME.ui.posX = posX;
		ctxCAME.ui.posY = posY;
		ctxCAME.ui.posZ = posZ;
		ctxCAME.ui.rotX = rotX;
		ctxCAME.ui.rotY = rotY;
		ctxCAME.ui.rotZ = rotZ;
		ctxCAME.ui.time = time;
		ctxCAME.ui.nextCam = aa;
		ctxCAME.ui.route = vv;
		ctxCAME.ui.zoomStart = zoomStart;
		ctxCAME.ui.zoomEnd = zoomEnd;
		ctxCAME.ui.viewStartX = viewStartX;
		ctxCAME.ui.viewStartY = viewStartY;
		ctxCAME.ui.viewStartZ = viewStartZ;
		ctxCAME.ui.viewEndX = viewEndX;
		ctxCAME.ui.viewEndY = viewEndY;
		ctxCAME.ui.viewEndZ = viewEndZ;
		ctxCAME.ui.shake = shake;

		ProcessSectionEditing(
			ctxCAME,
			primaryIndex,
			anchorIndex,
			hoverIndex,
			dragIndices,
			dragStartPositions,
			dragStartPrimaryPos,
			projection, view,
			vx, vy, vw, vh,
			altDown
		);
		ApplyTextEditToPoint(ctxCAME);
	}

	if (enptItem != nullptr && enptItem->expanded)
	{
		UIItem* posX = ctxENPT.ui.posX;
		UIItem* posY = ctxENPT.ui.posY;
		UIItem* posZ = ctxENPT.ui.posZ;
		UIItem* deviation = ctxENPT.ui.deviation;
		UIItem* s1 = ctxENPT.ui.setting1;
		UIItem* s2 = ctxENPT.ui.setting2;
		UIItem* s3 = ctxENPT.ui.setting3;
		if (!(posX && posY && posZ && deviation && s1 && s2 && s3))
			return false;

		bool selectable = (primaryIndex >= 0);
		posX->selectable = selectable;
		posY->selectable = selectable;
		posZ->selectable = selectable;
		deviation->selectable = selectable;
		s1->selectable = selectable;
		s2->selectable = selectable;
		s3->selectable = selectable;

		ctxENPT.data = &loadedKMP.enpt;
		ctxENPT.type = SectionType::ENPT;
		ctxENPT.createNew = CreateENPT;

		ctxENPT.ui.posX = posX;
		ctxENPT.ui.posY = posY;
		ctxENPT.ui.posZ = posZ;
		ctxENPT.ui.deviation = deviation;
		ctxENPT.ui.setting1 = s1;
		ctxENPT.ui.setting2 = s2;
		ctxENPT.ui.setting3 = s3;

		ProcessSectionEditing(
			ctxENPT,
			primaryIndex,
			anchorIndex,
			hoverIndex,
			dragIndices,
			dragStartPositions,
			dragStartPrimaryPos,
			projection, view,
			vx, vy, vw, vh,
			altDown
		);
		ApplyTextEditToPoint(ctxENPT);
	}

	if (itptItem != nullptr && itptItem->expanded)
	{
		UIItem* posX = ctxITPT.ui.posX;
		UIItem* posY = ctxITPT.ui.posY;
		UIItem* posZ = ctxITPT.ui.posZ;
		UIItem* deviation = ctxITPT.ui.deviation;
		UIItem* s1 = ctxITPT.ui.setting1;
		UIItem* s2 = ctxITPT.ui.setting2;
		if (!(posX && posY && posZ && deviation && s1 && s2))
			return false;

		bool selectable = (primaryIndex >= 0);
		posX->selectable = selectable;
		posY->selectable = selectable;
		posZ->selectable = selectable;
		deviation->selectable = selectable;
		s1->selectable = selectable;
		s2->selectable = selectable;

		ctxITPT.data = &loadedKMP.itpt;
		ctxITPT.type = SectionType::ITPT;
		ctxITPT.createNew = CreateITPT;

		ctxITPT.ui.posX = posX;
		ctxITPT.ui.posY = posY;
		ctxITPT.ui.posZ = posZ;
		ctxITPT.ui.deviation = deviation;
		ctxITPT.ui.setting1 = s1;
		ctxITPT.ui.setting2 = s2;

		ProcessSectionEditing(
			ctxITPT,
			primaryIndex,
			anchorIndex,
			hoverIndex,
			dragIndices,
			dragStartPositions,
			dragStartPrimaryPos,
			projection, view,
			vx, vy, vw, vh,
			altDown
		);
		ApplyTextEditToPoint(ctxITPT);
	}

	if (msptItem != nullptr && msptItem->expanded)
	{
		UIItem* posX = ctxMSPT.ui.posX;
		UIItem* posY = ctxMSPT.ui.posY;
		UIItem* posZ = ctxMSPT.ui.posZ;
		UIItem* rotX = ctxMSPT.ui.rotX;
		UIItem* rotY = ctxMSPT.ui.rotY;
		UIItem* rotZ = ctxMSPT.ui.rotZ;
		UIItem* Id = ctxMSPT.ui.id;
		if (!(posX && posY && posZ && rotX && rotY && rotZ && Id))
			return false;

		bool selectable = (primaryIndex >= 0);
		posX->selectable = selectable;
		posY->selectable = selectable;
		posZ->selectable = selectable;
		rotX->selectable = selectable;
		rotY->selectable = selectable;
		rotZ->selectable = selectable;
		Id->selectable = selectable;

		ctxMSPT.data = &loadedKMP.mspt;
		ctxMSPT.createNew = CreateMSPT;

		ctxMSPT.ui.posX = posX;
		ctxMSPT.ui.posY = posY;
		ctxMSPT.ui.posZ = posZ;
		ctxMSPT.ui.rotX = rotX;
		ctxMSPT.ui.rotY = rotY;
		ctxMSPT.ui.rotZ = rotZ;
		ctxMSPT.ui.id = Id;

		ProcessSectionEditing(
			ctxMSPT,
			primaryIndex,
			anchorIndex,
			hoverIndex,
			dragIndices,
			dragStartPositions,
			dragStartPrimaryPos,
			projection, view,
			vx, vy, vw, vh,
			altDown
		);
		ApplyTextEditToPoint(ctxMSPT);
	}
	if (cnptItem != nullptr && cnptItem->expanded)
	{
		UIItem* posX = ctxCNPT.ui.posX;
		UIItem* posY = ctxCNPT.ui.posY;
		UIItem* posZ = ctxCNPT.ui.posZ;
		UIItem* rotX = ctxCNPT.ui.rotX;
		UIItem* rotY = ctxCNPT.ui.rotY;
		UIItem* rotZ = ctxCNPT.ui.rotZ;
		UIItem* Id = ctxCNPT.ui.id;
		UIItem* s1 = ctxCNPT.ui.setting1;
		if (!(posX && posY && posZ && rotX && rotY && rotZ && Id && s1))
			return false;

		bool selectable = (primaryIndex >= 0);
		posX->selectable = selectable;
		posY->selectable = selectable;
		posZ->selectable = selectable;
		rotX->selectable = selectable;
		rotY->selectable = selectable;
		rotZ->selectable = selectable;
		Id->selectable = selectable;
		s1->selectable = selectable;

		ctxCNPT.data = &loadedKMP.cnpt;
		ctxCNPT.type = SectionType::CNPT;
		ctxCNPT.createNew = CreateCNPT;

		ctxCNPT.ui.posX = posX;
		ctxCNPT.ui.posY = posY;
		ctxCNPT.ui.posZ = posZ;
		ctxCNPT.ui.rotX = rotX;
		ctxCNPT.ui.rotY = rotY;
		ctxCNPT.ui.rotZ = rotZ;
		ctxCNPT.ui.id = Id;
		ctxCNPT.ui.setting1 = s1;

		ProcessSectionEditing(
			ctxCNPT,
			primaryIndex,
			anchorIndex,
			hoverIndex,
			dragIndices,
			dragStartPositions,
			dragStartPrimaryPos,
			projection, view,
			vx, vy, vw, vh,
			altDown
		);
		ApplyTextEditToPoint(ctxCNPT);
	}
	if (jgptItem != nullptr && jgptItem->expanded)
	{
		UIItem* posX = ctxJGPT.ui.posX;
		UIItem* posY = ctxJGPT.ui.posY;
		UIItem* posZ = ctxJGPT.ui.posZ;
		UIItem* rotX = ctxJGPT.ui.rotX;
		UIItem* rotY = ctxJGPT.ui.rotY;
		UIItem* rotZ = ctxJGPT.ui.rotZ;
		UIItem* Id = ctxJGPT.ui.id;
		UIItem* soundTrig = ctxJGPT.ui.soundTrig;
		if (!(posX && posY && posZ && rotX && rotY && rotZ && Id && soundTrig))
			return false;

		bool selectable = (primaryIndex >= 0);
		posX->selectable = selectable;
		posY->selectable = selectable;
		posZ->selectable = selectable;
		rotX->selectable = selectable;
		rotY->selectable = selectable;
		rotZ->selectable = selectable;
		Id->selectable = selectable;
		soundTrig->selectable = selectable;

		ctxJGPT.data = &loadedKMP.jgpt;
		ctxJGPT.createNew = CreateJGPT;

		ctxJGPT.ui.posX = posX;
		ctxJGPT.ui.posY = posY;
		ctxJGPT.ui.posZ = posZ;
		ctxJGPT.ui.rotX = rotX;
		ctxJGPT.ui.rotY = rotY;
		ctxJGPT.ui.rotZ = rotZ;
		ctxJGPT.ui.id = Id;
		ctxJGPT.ui.soundTrig = soundTrig;

		ProcessSectionEditing(
			ctxJGPT,
			primaryIndex,
			anchorIndex,
			hoverIndex,
			dragIndices,
			dragStartPositions,
			dragStartPrimaryPos,
			projection, view,
			vx, vy, vw, vh,
			altDown
		);
		ApplyTextEditToPoint(ctxJGPT);
	}

	if (gMain->mShowAnimations)
		brresRenderer.Update(deltaTime, projection, view);

	vp = projection * view;

	if (Input::GetMouseButtonUp(0))
		HandleMouseUp();

	core->RunCallbacks();
	return true;
}
int RenderContext::findCursorPos(
	const std::string& text,
	float localX,
	float scaleX,
	const std::function<float(float)>& ratioFunc)
{
	float x = 0.0f;

	for (int i = 0; i < (int)text.size(); i++)
	{
		float w = measureTextWidth(text.substr(i, 1), scaleX, ratioFunc);

		if (localX < x + w * 0.5f)
			return i;

		x += w;
	}

	return text.size();
}
glm::vec3 RenderContext::ScreenToWorldRay(
	float mouseX, float mouseY,
	float vx, float vy,
	float vw, float vh,
	const glm::mat4& projection,
	const glm::mat4& view)
{
	float localX = mouseX - vx;
	float localY = mouseY - vy;

	float x = (2.0f * localX) / vw - 1.0f;
	float y = 1.0f - (2.0f * localY) / vh;

	glm::vec4 rayNDC(x, y, -1.0f, 1.0f);
	glm::vec4 rayEye = glm::inverse(projection) * rayNDC;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

	glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
	return rayWorld;
}
void RenderContext::SetAreaItem(UIItem* item)
{
		switch (item->listIndex)
		{
		case 0: {
			ctxAREA.ui.cameraIndex->invisible = false;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			break;
		}
		case 1: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = false;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			break;
		}
		case 2: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = false;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			break;
		}
		case 3: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = false;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = false;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = false;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			routeList.clear();
			routeList.push_back("None");
			for (int i = 1; i < loadedKMP.came.size(); i++)
			{
				routeList.push_back("Route " + std::to_string(i));
			}
			break;
		}
		case 4: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = false;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			break;
		}
		case 6: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = false;
			ctxAREA.ui.setting6->invisible = false;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			break;
		}
		case 5:
		case 7:
		case 11: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			break;
		}
		case 8:
		case 9: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = false;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			break;
		}
		case 10: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = false;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			ctxAREA.ui.coobStartCPIndexName->label = "KCP Index";
			if (ctxAREA.ui.setting11->checked)
			{
				ctxAREA.ui.setting4->invisible = false;
				if (ctxAREA.ui.setting4->listIndex == 1)
				{
					ctxAREA.ui.coobStartCPIndex->invisible = false;
					ctxAREA.ui.coobStartCPIndexName->label = "Start Index";
				}
				ctxAREA.ui.coobEndCPIndex->invisible = false;
			}
			break;
		}
		case 12: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = false;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = false;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			routeList.clear();
			routeList.push_back("None");
			for (int i = 1; i < loadedKMP.came.size(); i++)
			{
				routeList.push_back("Route " + std::to_string(i));
			}
			break;
		}
		case 13: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = false;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = false;
			ctxAREA.ui.cobjUnloadFrame1->invisible = false;
			ctxAREA.ui.cobjUnloadSetting->invisible = false;
			ctxAREA.ui.cobjUnloadSetting2->invisible = false;
			ctxAREA.ui.cobjUnloadSetting3->invisible = false;
			ctxAREA.ui.cobjUnloadFrame2->invisible = false;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			break;
		}
		case 14: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = false;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			break;
		}
		case 15: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = false;
			ctxAREA.ui.teleportAngle->invisible = false;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			break;
		}
		case 16: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = false;
			ctxAREA.ui.antiGravity->invisible = false;
			ctxAREA.ui.gravityScale->invisible = false;
			ctxAREA.ui.windAngle->invisible = true;
			ctxAREA.ui.windPower->invisible = true;
			break;
		}
		case 17: {
			ctxAREA.ui.cameraIndex->invisible = true;
			ctxAREA.ui.setting9->invisible = true;
			ctxAREA.ui.setting10->invisible = true;
			ctxAREA.ui.setting11->invisible = true;
			ctxAREA.ui.setting12->invisible = true;
			ctxAREA.ui.setting4->invisible = true;
			ctxAREA.ui.setting5->invisible = true;
			ctxAREA.ui.setting6->invisible = true;
			ctxAREA.ui.setting7->invisible = true;
			ctxAREA.ui.setting8->invisible = true;
			ctxAREA.ui.routeIndex->invisible = true;
			ctxAREA.ui.enemyIndex->invisible = true;
			ctxAREA.ui.coobStartCPIndex->invisible = true;
			ctxAREA.ui.coobEndCPIndex->invisible = true;
			ctxAREA.ui.railRidingRotation->invisible = true;
			ctxAREA.ui.cobjUnloadGroup->invisible = true;
			ctxAREA.ui.cobjUnloadKCLFlag->invisible = true;
			ctxAREA.ui.cobjUnloadFrame1->invisible = true;
			ctxAREA.ui.cobjUnloadSetting->invisible = true;
			ctxAREA.ui.cobjUnloadSetting2->invisible = true;
			ctxAREA.ui.cobjUnloadSetting3->invisible = true;
			ctxAREA.ui.cobjUnloadFrame2->invisible = true;
			ctxAREA.ui.airRingBoostTime->invisible = true;
			ctxAREA.ui.teleportTime->invisible = true;
			ctxAREA.ui.teleportAngle->invisible = true;
			ctxAREA.ui.gravityType->invisible = true;
			ctxAREA.ui.antiGravity->invisible = true;
			ctxAREA.ui.gravityScale->invisible = true;
			ctxAREA.ui.windAngle->invisible = false;
			ctxAREA.ui.windPower->invisible = false;
			break;
		}
		}

}
void RenderContext::HandleClick(float mouseX, float mouseY)
{
	bool isShiftPressed = 
        (glfwGetKey(gMain->mWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
        (glfwGetKey(gMain->mWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
	if (activeColorPicker)
	{
		float px = activeColorPicker->pickerX;
		float py = activeColorPicker->pickerY;
		float pw = activeColorPicker->pickerW;
		float ph = activeColorPicker->pickerH;

		if (mouseX >= px && mouseX <= px + pw &&
			mouseY >= py && mouseY <= py + ph)
		{
			return;
		}

		if (activeColorPicker->onCommit)
		{
			activeColorPicker->onCommit();
			std::string pathing = std::filesystem::path(mPath).filename().string();

			glfwSetWindowTitle(
				gMain->mWindow,
				("KMParse: Editing " + pathing + " (Unsaved)").c_str());
			gMain->dirty = true;

		}

		activeColorPicker->pickingColor = false;
		activeColorPicker = nullptr;
		return;
	}

	auto clearSection = [&](auto& ctx) {
		if (!ctx.data) return;
		for (auto& p : *ctx.data)
			p.selected = false;
	};

	auto normalizeNumber = [&](UIItem* item)
	{
		if (item->editBuffer.empty())
		{
			item->numberValue = item->numberDefault;
			item->editBuffer = std::to_string(item->numberDefault);
			item->cursorPos = item->editBuffer.size();
			return;
		}

		int v = std::stoi(item->editBuffer);

		v = std::clamp(v, item->numberMin, item->numberMax);

		item->numberValue = v;
		item->editBuffer = std::to_string(v);
		item->cursorPos = item->editBuffer.size();
	};
	auto normalizeFloat = [&](UIItem* item)
	{
		if (item->editBuffer.empty())
		{
			item->floatValue = item->floatDefault;
			item->editBuffer = std::format("{:.6f}", item->floatDefault);
			item->cursorPos = item->editBuffer.size();
			return;
		}

		float v = std::stof(item->editBuffer);

		v = std::clamp(v, item->floatMin, item->floatMax);

		item->floatValue = v;
		item->editBuffer = std::format("{:.6f}", v);
		item->cursorPos = item->editBuffer.size();
	};
	if (editingItem && editingItem->editing)
	{
		editingItem->editing = false;

		if (editingItem->type == UIItemType::Float)
			normalizeFloat(editingItem);
		else if (editingItem->type == UIItemType::Number)
			normalizeNumber(editingItem);

		if (editingItem->onCommit)
		{
			editingItem->onCommit();
			std::string pathing = std::filesystem::path(mPath).filename().string();

			glfwSetWindowTitle(
				gMain->mWindow,
				("KMParse: Editing " + pathing + " (Unsaved)").c_str());
			gMain->dirty = true;
			editingItem = nullptr;
		}
	}


	if (mouseX > mUIWidth)
	{
		for (auto& row : rows)
		{
			if (!row.value || row.value->type != UIItemType::List)
				continue;
				row.value->listOpen = false;
		}
		SetAreaItemInvisible();
		return;
	}

	float rowHeight = style.rowHeight;
	for (auto& row : rows)
	{
		if (!row.value || row.value->type != UIItemType::List)
			continue;

		UIItem* item = row.value;
		if (!item->listOpen)
			continue;

		if (!item->selectable)
			continue;

		float uiScale = mUIWidth / 512.0f;

		float rowY = 10.0f + row.drawIndex * style.rowHeight - listScroll;

		float boxX = item->indent + style.valueOffsetX * uiScale;
		float boxY = rowY + (style.rowHeight - style.valueHeight) * 0.5f;

		float optY = boxY + style.valueHeight;

		for (int k = 0; k < item->listValues.size(); k++)
		{
			if (mousePressedInside(boxX, optY, style.valueWidth, style.valueHeight))
			{
				item->pendingListIndex = k;
				item->listOpen = false;

				if (item->onCommit)
				{
					item->onCommit();
					std::string pathing = std::filesystem::path(mPath).filename().string();

					glfwSetWindowTitle(
						gMain->mWindow,
						("KMParse: Editing " + pathing + " (Unsaved)").c_str());
					gMain->dirty = true;
				}
				return;
			}
			else
			{
				item->listOpen = false;
			}
			optY += style.valueHeight;
		}
	}

	for (auto& row : rows)
	{
		if (row.value && row.value->invisible)
			continue;

		float y = 10.0f + row.drawIndex * style.rowHeight - listScroll;

		if (mouseY >= y && mouseY <= y + style.rowHeight)
		{
			if (row.label && row.label->type == UIItemType::Group)
			{
				for (auto& g : mainPanel.groups)
				{
					if (g.titleItem == row.label)
					{
						bool newState = !g.expanded;
						if (newState && currentOpenGroup != &g)
						{
							clearSection(ctxKTPT);
							clearSection(ctxMSPT);
							clearSection(ctxJGPT);
							clearSection(ctxITPT);
							clearSection(ctxENPT);
							clearSection(ctxCNPT);
							clearSection(ctxCAME);
							clearSection(ctxAREA);

							primaryIndex = -1;
							anchorIndex = -1;
							hoverIndex = -1;
							dragIndices.clear();

							currentOpenGroup = &g;
						}


						g.expanded = newState;

						if (newState)
						{
							for (auto& g2 : mainPanel.groups)
								if (&g2 != &g)
								{
									g2.expanded = false;
								}
						}

						break;
					}
				}
				return;
			}

			if (row.label &&
				(row.value->type == UIItemType::Number ||
					row.value->type == UIItemType::Float))
			{
				UIItem* item = row.label;
				UIItem* item2 = row.value;

				bool overLabel =
					mouseX >= item->labelHitX && mouseX <= item->labelHitX + item->labelHitW &&
					mouseY >= item->labelHitY && mouseY <= item->labelHitY + item->labelHitH;

				if (overLabel)
				{
					dragBeforeUI = item2->toState();
					dragBeforeKMP = loadedKMP;

					item2->editing = false;
					item2->draggingValue = true;
					item2->dragStartY = mouseY;

					if (item2->type == UIItemType::Float)
						item2->dragStartValue = item2->floatValue;
					else
						item2->dragStartValue = (float)item2->numberValue;

					editingItem = item2;
					return;
				}

			}
			if (row.value &&
				(row.value->type == UIItemType::Number ||
					row.value->type == UIItemType::Float))
			{
				UIItem* item = row.value;
				if (!item->selectable)
					continue;
				float uiScale = mUIWidth / 512.0f;
				float scaleX = 0.7f * uiScale;

				float indent = item->indent;
				float boxX = indent + style.valueOffsetX * uiScale;
				float boxW = 170.0f * uiScale;
				float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;
				float boxH = style.valueHeight;

				bool overBox =
					mouseX >= boxX && mouseX <= boxX + boxW &&
					mouseY >= boxY && mouseY <= boxY + boxH;

				float textX = boxX + 6 * uiScale;
				float textW = measureTextWidth(item->editBuffer, scaleX, [&](float) {return 1.0f; });
				float textH = style.rowHeight * 0.7f;
				float textY = y + (style.rowHeight - textH) * 0.5f;

				if (overBox)
				{
					item->editing = true;
					editingItem = item;

					float localX = mouseX - textX;

					float naturalWidth = measureTextWidth(item->editBuffer, scaleX,
						[&](float) { return 1.0f; });

					float ratio = std::min(1.0f, boxW / naturalWidth);
					auto ratioFunc = [ratio](float) { return ratio; };

					item->cursorPos = findCursorPos(item->editBuffer, localX, scaleX, ratioFunc);

					if (!isShiftPressed)
					{
						item->selectStart = -1;
						item->selectEnd = -1;
					}
					else
					{
						if (item->selectStart == -1)
							item->selectStart = item->cursorPos;

						item->selectEnd = item->cursorPos;
					}

					editingDrag = true;
					return;
				}

				return;
			}

			if (row.value && row.value->onClick && row.value->selectable)
			{
					if (row.value && row.value->type == UIItemType::List && row.value->selectable)
					{
						UIItem* item = row.value;

						float uiScale = mUIWidth / 512.0f;
						float boxX = item->indent + style.valueOffsetX * uiScale;
						float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;
						float boxW = 170.0f * uiScale;
						float boxH = style.valueHeight;

						bool overBox =
							mouseX >= boxX && mouseX <= boxX + boxW &&
							mouseY >= boxY && mouseY <= boxY + boxH;

						if (!overBox)
							return;

						if (row.value->listOpen)
							return;

						if (item->onClick)
							item->onClick();

						return;
					}
				}

				if (row.value && row.value->type == UIItemType::Checkbox && row.value->selectable)
				{
					UIItem* item = row.value;

					float uiScale = mUIWidth / 512.0f;
					float boxX = item->indent + 10.0f * uiScale;
					float boxY = y + 10.0f * uiScale;
					float boxSize = 20.0f * uiScale;

					bool overBox =
						mouseX >= boxX && mouseX <= boxX + boxSize &&
						mouseY >= boxY && mouseY <= boxY + boxSize;

					if (!overBox)
						return;

					if (item->onClick)
						item->onClick();

					return;
				}
				if (row.value && row.value->type == UIItemType::Color && row.value->selectable)
				{
					UIItem* item = row.value;

					float uiScale = mUIWidth / 512.0f;
					float boxX = item->indent + style.valueOffsetX * uiScale;
					float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;
					float boxW = 40.0f * uiScale;
					float boxH = style.valueHeight;

					bool overBox =
						mouseX >= boxX && mouseX <= boxX + boxW &&
						mouseY >= boxY && mouseY <= boxY + boxH;

					if (!overBox)
						return;
					if (item->onClick)
						item->onClick();
					

					return;
				}
				if (row.value && row.value->type == UIItemType::Button && row.value->selectable)
				{
					UIItem* item = row.value;

					float uiScale = mUIWidth / 512.0f;

					float boxX = item->indent + style.labelOffsetX * uiScale;
					float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;
					float boxW = 340.0f * uiScale;
					float boxH = style.valueHeight;

					bool overBox =
						mouseX >= boxX && mouseX <= boxX + boxW &&
						mouseY >= boxY && mouseY <= boxY + boxH;

					if (!overBox)
						return;
					item->pressed = true;
					if (item->onClick)
						item->onClick();
					return;
				}

			return;
		}
	}
}
void RenderContext::SetAreaItemInvisible()
{
	UIItem* s0 = ctxAREA.ui.setting9;
	UIItem* s1 = ctxAREA.ui.setting10;
	UIItem* s2 = ctxAREA.ui.setting11;
	UIItem* s3 = ctxAREA.ui.setting12;
	UIItem* s4 = ctxAREA.ui.setting4;
	UIItem* s5 = ctxAREA.ui.setting5;
	UIItem* s6 = ctxAREA.ui.setting6;
	UIItem* s7 = ctxAREA.ui.setting7;
	UIItem* s8 = ctxAREA.ui.setting8;
	UIItem* s9 = ctxAREA.ui.railRidingRotation;
	UIItem* s11 = ctxAREA.ui.cobjUnloadGroup;
	UIItem* s12 = ctxAREA.ui.cobjUnloadKCLFlag;
	UIItem* s13 = ctxAREA.ui.cobjUnloadFrame1;
	UIItem* s14 = ctxAREA.ui.cobjUnloadSetting;
	UIItem* s15 = ctxAREA.ui.cobjUnloadFrame2;
	UIItem* s16 = ctxAREA.ui.airRingBoostTime;
	UIItem* s17 = ctxAREA.ui.teleportTime;
	UIItem* s18 = ctxAREA.ui.teleportAngle;
	UIItem* s19 = ctxAREA.ui.gravityType;
	UIItem* s20 = ctxAREA.ui.antiGravity;
	UIItem* s21 = ctxAREA.ui.gravityScale;
	UIItem* s22 = ctxAREA.ui.windAngle;
	UIItem* s23 = ctxAREA.ui.windPower;
	UIItem* s24 = ctxAREA.ui.cobjUnloadSetting2;
	UIItem* s25 = ctxAREA.ui.cobjUnloadSetting3;
	UIItem* camera = ctxAREA.ui.cameraIndex;
	UIItem* route = ctxAREA.ui.routeIndex;
	UIItem* enemy = ctxAREA.ui.enemyIndex;
	UIItem* coob1 = ctxAREA.ui.coobStartCPIndex;
	UIItem* coob2 = ctxAREA.ui.coobEndCPIndex;

	if (!(s0 && s1 && s2 && s3 && s4 && s5 && s6 && s7 && s8 && s9 && s11 && s12 && s13 && s14 && s15 && s16 && s17 && s18 && s19 && s20 && s21 && s22 && s23 && s24 && s25 && camera && route && enemy && coob1 && coob2))
		return;
	s0->invisible = true;
	s1->invisible = true;
	s2->invisible = true;
	s3->invisible = true;
	s4->invisible = true;
	s5->invisible = true;
	s6->invisible = true;
	s7->invisible = true;
	s8->invisible = true;
	s9->invisible = true;
	s11->invisible = true;
	s12->invisible = true;
	s13->invisible = true;
	s14->invisible = true;
	s15->invisible = true;
	s16->invisible = true;
	s17->invisible = true;
	s18->invisible = true;
	s19->invisible = true;
	s20->invisible = true;
	s21->invisible = true;
	s22->invisible = true;
	s23->invisible = true;
	s24->invisible = true;
	s25->invisible = true;
	camera->invisible = true;
	route->invisible = true;
	enemy->invisible = true;
	coob1->invisible = true;
	coob2->invisible = true;
}
void RenderContext::HandleMouseWrap()
{
	double x, y;
	glfwGetCursorPos(gMain->mWindow, &x, &y);

	int w, h;
	glfwGetWindowSize(gMain->mWindow, &w, &h);

	const int margin = 2;
	bool wrapped = false;

	if (y <= margin) {
		glfwSetCursorPos(gMain->mWindow, x, h - margin - 1);
		wrapped = true;
	}
	else if (y >= h - margin) {
		glfwSetCursorPos(gMain->mWindow, x, margin + 1);
		wrapped = true;
	}

	if (wrapped) {
		double nx, ny;
		glfwGetCursorPos(gMain->mWindow, &nx, &ny);

		Input::ForceMousePosition((float)nx, (float)ny);

		Input::ResetMouseDelta();
		if (editingItem) 
			editingItem->dragStartY = (float)ny;
	}
}


void RenderContext::HandleMouseMove(float mouseX, float mouseY)
{
	if (editingItem && editingItem->draggingValue)
	{
		double gx, gy;
		glfwGetCursorPos(gMain->mWindow, &gx, &gy);

		HandleMouseWrap();

		glfwGetCursorPos(gMain->mWindow, &gx, &gy);

		mouseY = (float)gy;

		float dy = mouseY - editingItem->dragStartY;
		float delta = dy * 0.04f;

		if (editingItem->type == UIItemType::Float)
		{
			float newValue = editingItem->dragStartValue + delta;

			newValue = std::clamp(newValue, editingItem->floatMin, editingItem->floatMax);

			editingItem->floatValue = newValue;

			editingItem->dragStartValue = newValue;
			editingItem->dragStartY = mouseY;
		}
		else
		{
			float newValue = editingItem->dragStartValue + delta;
			newValue = std::clamp(newValue, (float)editingItem->numberMin, (float)editingItem->numberMax);

			editingItem->numberValue = (int)newValue;

			editingItem->dragStartValue = newValue;
			editingItem->dragStartY = mouseY;
		}
		gMain->dirty = true;
		return;
	}
	


	if (editingItem && editingItem->editing)
	{
		float uiScale = mUIWidth / 512.0f;
		float scaleX = 0.7f * uiScale;

		float indent = editingItem->indent;

		float boxX = indent + style.valueOffsetX * uiScale;
		float textX = boxX + 6 * uiScale;

		float localX = mouseX - textX;

		float boxW = 170.0f * uiScale;

		float naturalWidth = measureTextWidth(editingItem->editBuffer, scaleX,
			[&](float) { return 1.0f; });

		float minRatio = std::max(0.3f, uiScale * 0.4f);
		float maxRatio = 0.55f;

		float rawNumberRatio = boxW / naturalWidth;
		float ratio = std::clamp(rawNumberRatio, minRatio, maxRatio);

		auto ratioFunc = [ratio](float) { return ratio; };

		int pos = findCursorPos(editingItem->editBuffer, localX, scaleX, ratioFunc);

		editingItem->cursorPos = pos;

		if (editingItem->selectStart == -1)
			editingItem->selectStart = pos;

		editingItem->selectEnd = pos;
	}
}

inline void updateSelection(RenderContext::UIItem* item)
{
	if (item->selectStart == -1)
		item->selectStart = item->cursorPos;

	item->selectEnd = item->cursorPos;
}
inline bool hasSelection(RenderContext::UIItem* item)
{
	return item->selectStart != -1 && item->selectEnd != -1 && item->selectStart != item->selectEnd;
}
inline void deleteSelection(RenderContext::UIItem* item)
{
	if (!hasSelection(item)) return;

	int a = std::min(item->selectStart, item->selectEnd);
	int b = std::max(item->selectStart, item->selectEnd);

	item->editBuffer.erase(a, b - a);
	item->cursorPos = a;

	item->selectStart = -1;
	item->selectEnd = -1;
}
inline std::string getSelection(RenderContext::UIItem* item)
{
	if (!hasSelection(item)) return "";

	int a = std::min(item->selectStart, item->selectEnd);
	int b = std::max(item->selectStart, item->selectEnd);

	return item->editBuffer.substr(a, b - a);
}
float RenderContext::measureTextWidth(
	const std::string& text,
	float scaleX,
	const std::function<float(float)>& ratioFunc)
{
	if (!mUIFont) return 0.0f;

	const lyt::RFNT& rfnt = mUIFont->rfnt;
	std::u16string u16 = utf8ToUtf16(text);

	float x = 0.0f;
	bool first = true;

	for (char16_t ch : u16)
	{
		uint16_t glyphIndex = rfnt.cmap[ch];
		if (glyphIndex == 0xFFFF)
			glyphIndex = rfnt.defaultGlyphIndex;

		const lyt::GlyphInfo& gi = rfnt.glyphInfo[glyphIndex];

		float left = gi.cwdh.leftSideBearing * scaleX;
		float adv = gi.cwdh.advanceWidth * scaleX;

		float minAdvance = 8.0f * scaleX;
		adv = std::max(adv, minAdvance);

		if (first)
		{
			x += left;
			first = false;
		}

		float cursorBefore = x;
		float ratio = ratioFunc(cursorBefore);
		x += adv * ratio;

	}

	return x;
}
bool RenderContext::HandleKey(int key)
{
	if (!editingItem)
		return false;

	UIItem* item = editingItem;
bool isShiftPressed = 
        (glfwGetKey(gMain->mWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
        (glfwGetKey(gMain->mWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
	bool editable =
		item->type == UIItemType::Number ||
		item->type == UIItemType::Float;

	if (!editable)
		return false;

	if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)
	{
		if (item->onCommit)
		{
			item->onCommit();
			std::string pathing = std::filesystem::path(mPath).filename().string();

			glfwSetWindowTitle(
				gMain->mWindow,
				("KMParse: Editing " + pathing + " (Unsaved)").c_str());
			gMain->dirty = true;
		}
		item->editing = false;
		editingItem = nullptr;
		return true;
	}

	if (key == GLFW_KEY_LEFT)
	{
		if (item->cursorPos > 0)
			item->cursorPos--;

		if (!isShiftPressed)
			item->selectStart = item->selectEnd = -1;
		else
			updateSelection(item);

		return true;
	}

	if (key == GLFW_KEY_RIGHT)
	{
		if (item->cursorPos < item->editBuffer.size())
			item->cursorPos++;

		if (!isShiftPressed)
			item->selectStart = item->selectEnd = -1;
		else
			updateSelection(item);

		return true;
	}

	if (key == GLFW_KEY_ESCAPE)
	{
		item->editing = false;
		editingItem = nullptr;
		return true;
	}

	if (key == GLFW_KEY_BACKSPACE)
	{
		if (hasSelection(item))
			deleteSelection(item);
		else if (item->cursorPos > 0)
		{
			item->editBuffer.erase(item->cursorPos - 1, 1);
			item->cursorPos--;
		}
		return true;
	}

	if (key == GLFW_KEY_DELETE)
	{
		if (hasSelection(item))
			deleteSelection(item);
		else if (item->cursorPos < item->editBuffer.size())
			item->editBuffer.erase(item->cursorPos, 1);

		return true;
	}

	return false;
}
bool RenderContext::HandleChar(char c)
{
	if (!editingItem) return false;

	UIItem* item = editingItem;

	bool editable =
		item->type == UIItemType::Number ||
		item->type == UIItemType::Float;

	if (!editable) return false;

	if (item->type == UIItemType::Number)
	{
		if (isdigit(c))
		{
			int digitCount = 0;
			for (char ch : item->editBuffer)
				if (isdigit(ch)) digitCount++;

			if (item->maxDigits > 0 && digitCount >= item->maxDigits)
				return true;

			item->editBuffer.insert(item->cursorPos, 1, c);
			item->cursorPos++;
			return true;
		}

		if (c == '-')
		{
			if (item->cursorPos != 0)
				return true;

			if (!item->editBuffer.empty() && item->editBuffer[0] == '-')
				return true;

			item->editBuffer.insert(item->cursorPos, 1, c);
			item->cursorPos++;
			return true;
		}

		return true;
	}

	if (item->type == UIItemType::Float)
	{
		if (isdigit(c))
		{
			int digitCount = 0;
			for (char ch : item->editBuffer)
				if (isdigit(ch)) digitCount++;

			if (item->maxDigits > 0 && digitCount >= item->maxDigits)
				return true;

			item->editBuffer.insert(item->cursorPos, 1, c);
			item->cursorPos++;
			return true;
		}

		if (c == '.')
		{
			if (item->editBuffer.find('.') != std::string::npos)
				return true;

			item->editBuffer.insert(item->cursorPos, 1, c);
			item->cursorPos++;
			return true;
		}

		if (c == '-')
		{
			if (item->cursorPos != 0)
				return true;

			if (!item->editBuffer.empty() && item->editBuffer[0] == '-')
				return true;

			item->editBuffer.insert(item->cursorPos, 1, c);
			item->cursorPos++;
			return true;
		}

		return true;
	}

	return false;
}
void RenderContext::DrawTriangleRight(float x, float y, float size, const glm::vec4& color, float z)
{
	float half = size * 0.5f;

	float verts[9] = {
		x,       y,       z,
		x,       y + size, z,
		x + size, y + half, z
	};

	glUseProgram(uiProgram);
	glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, glm::value_ptr(uiProj));
	glUniform4fv(uColorLoc, 1, glm::value_ptr(color));

	glBindVertexArray(uiVAO);
	glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}
void RenderContext::DrawTriangleDown(float x, float y, float size, const glm::vec4& color, float z)
{
	float half = size * 0.5f;

	float verts[9] = {
		x,        y,       z,
		x + size, y,       z,
		x + half, y + size, z
	};

	glUseProgram(uiProgram);
	glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, glm::value_ptr(uiProj));
	glUniform4fv(uColorLoc, 1, glm::value_ptr(color));

	glBindVertexArray(uiVAO);
	glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}
void RenderContext::DrawGroup(UIItem* item, float indent, float y)
{
	float scaleX = 1.0 * mUIWidth / 512.0f;
	float scaleY = 0.7 * mUIWidth / 512.0f;
	float boxW = 500.0f * mUIWidth / 512.0f;

	float naturalWidth = measureTextWidth(item->label, scaleX,
		[&](float) { return 1.0f; });

	float ratio = std::min(1.0f, boxW / naturalWidth);

	auto ratioFunc = [ratio](float) { return ratio; };

	DrawTextBase(indent + 30 * mUIWidth / 512.0f, y + style.labelOffsetY, item->label, style.textColor, scaleX, scaleY, ratioFunc);

	if (item->expanded)
		DrawTriangleDown(indent + 10 * mUIWidth / 512.0f, y + 12 * mUIWidth / 512.0f, style.triangleSize, style.textColor, 0.25f);
	else
		DrawTriangleRight(indent + 10 * mUIWidth / 512.0f, y + 12 * mUIWidth / 512.0f, style.triangleSize, style.textColor, 0.25f);
}
void RenderContext::DrawList(UIItem* item, float indent, float y)
{
	if (item->listIndex >= item->listValues.size()) return;
	float uiScale = mUIWidth / 512.0f;

	float boxX = indent + style.valueOffsetX * uiScale;
	float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;
	float scaleX = 0.7 * mUIWidth / 512.0f;
	float scaleY = 0.4 * mUIWidth / 512.0f;
	float labelBoxW = 120.0f * uiScale;
	float valueBoxW = 170.0f * uiScale;

	float naturalWidth = measureTextWidth(item->label, scaleX, [&](float) { return 1.0f; });
	float rawRatio = labelBoxW / naturalWidth;
	float ratio = std::clamp(rawRatio, 0.5f, item->maxCompressRatio);

	auto ratioFunc = [&](float cursorX) {
		if (item->compressStartX >= 0.0f && cursorX < item->compressStartX)
			return 1.0f;
		return ratio;
	};


	DrawTextBase(indent + style.labelOffsetX, y + style.labelOffsetY, item->label, style.textColor, 1.0 * mUIWidth / 512.0f, 0.7 * mUIWidth / 512.0f, ratioFunc);
	DrawUIBase(boxX, boxY, valueBoxW, style.valueHeight, style.valueBgColor, 0.3f);

	std::string current = item->listValues[item->listIndex]; 
	float naturalWidth2 = measureTextWidth(current, scaleX, [&](float) { return 1.0f; }); 
	float ratio2 = std::min(1.0f, (140.0f * uiScale) / naturalWidth2);

	DrawTextBase(boxX + 6, boxY, current, glm::vec4(0.0, 0.0, 0.0, 1), scaleX, scaleY, [&](float cursorX){ return cursorX < item->compressStartX ? 1.0f : ratio2; });

	DrawTriangleDown(boxX + valueBoxW - 16 * uiScale, boxY + 6 * uiScale, style.triangleSize, glm::vec4(0, 0, 0, 1), 0.25f);

	item->indent = indent;
}
void RenderContext::DrawCheckbox(UIItem* item, float indent, float y)
{
	float uiScale = mUIWidth / 512.0f;

	float boxX = indent + 10.0f * uiScale;
	float boxY = y + 10.0f * uiScale;
	float boxSize = 20.0f * uiScale;

	float scaleX = 1.0 * uiScale;
	float scaleY = 0.7 * uiScale;

	DrawUIBase(boxX, boxY, boxSize, boxSize, glm::vec4(1, 1, 1, 1), 0.3f);

	float boxW = 80.0f * uiScale;
	float naturalWidth = measureTextWidth(item->label, scaleX, [&](float) { return 1.0f; });
	float rawRatio = boxW / naturalWidth;
	float ratio = std::clamp(rawRatio, 0.6f, item->maxCompressRatio);

	auto ratioFunc = [&](float cursorX) {
		if (item->compressStartX >= 0.0f && cursorX < item->compressStartX)
			return 1.0f;
		return ratio;
	};
	if (item->checked)
		DrawUIBase(boxX + 4.0f * uiScale,
			boxY + 4.0f * uiScale,
			boxSize - 8.0f * uiScale,
			boxSize - 8.0f * uiScale,
			glm::vec4(0.2, 0.6, 1.0, 1), 0.5f);

	DrawTextBase(boxX + 30.0f * uiScale,
		boxY - 2.0f * uiScale,
		item->label,
		textColor,
		scaleX,
		scaleY, ratioFunc);
	item->indent = indent;
}
void RenderContext::DrawNumber(UIItem* item, float indent, float y)
{
	float uiScale = mUIWidth / 512.0f;
	float scaleX = 0.7f * uiScale;
	float scaleY = 0.4f * uiScale;

	float boxX = indent + style.valueOffsetX * uiScale;
	float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;
	float labelBoxW = 170.0f * uiScale;


	float naturalWidth = measureTextWidth(item->label, scaleX, [&](float) { return 1.0f; });
	float rawRatio = labelBoxW / naturalWidth;
	float ratio = std::clamp(rawRatio, item->minCompressRatio, item->maxCompressRatio);

	auto ratioFunc = [&](float cursorX) {
		if (item->compressStartX >= 0.0f && cursorX < item->compressStartX)
			return 1.0f;
		return ratio;
	};


	std::string text =
		item->editing ? item->editBuffer
		: std::to_string((int)item->numberValue);

	float naturalNumberWidth = measureTextWidth(text, scaleX, [&](float) {return 1.0f; });
	float numberBoxW = 170.0f * uiScale;

	float minRatio = std::max(0.3f, uiScale * 0.4f);
	float maxRatio = 0.5f;

	float rawNumberRatio = numberBoxW / naturalNumberWidth;
	float numberRatio = std::clamp(rawNumberRatio, minRatio, maxRatio);

	auto ratioFuncNumber = [numberRatio](float) { return numberRatio; };
	float labelX = indent + style.labelOffsetX;
	float labelY = y + style.labelOffsetY - 50.0f * uiScale;
	DrawTextBase(labelX,
		labelY,
		item->label,
		style.textColor,
		scaleX,
		scaleY, ratioFunc);

	DrawUIBase(boxX, boxY, numberBoxW, style.valueHeight, style.valueBgColor, 0.1f);

	if (item->editing && hasSelection(item))
	{
		int a = std::min(item->selectStart, item->selectEnd);
		int b = std::max(item->selectStart, item->selectEnd);

		float x1 = measureTextWidth(item->editBuffer.substr(0, a), scaleX, ratioFuncNumber);
		float x2 = measureTextWidth(item->editBuffer.substr(0, b), scaleX, ratioFuncNumber);

		DrawUIBase(boxX + 6 * uiScale + x1,
			boxY + 4 * uiScale,
			x2 - x1,
			style.valueHeight - 8 * uiScale,
			glm::vec4(0.3, 0.5, 1.0, 0.5), 0.3f);
	}
	if (item->editing)
	{
		bool showCursor = (fmod(glfwGetTime(), 1.0) < 0.5);

		if (showCursor)
		{
			float cursorX = measureTextWidth(item->editBuffer.substr(0, item->cursorPos), scaleX, ratioFuncNumber);

			DrawUIBase(
				boxX + 6 * uiScale + cursorX,
				boxY + 4 * uiScale,
				2.0f * uiScale,
				style.valueHeight - 8 * uiScale,
				glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), 0.4f
			);
		}
	}

	DrawTextBase(boxX + 6 * uiScale,
		boxY + 4 * uiScale,
		text,
		bg2Color,
		scaleX,
		scaleY, ratioFuncNumber);
}
void RenderContext::DrawFloat(UIItem* item, float indent, float y)
{
	float uiScale = mUIWidth / 512.0f;
	float scaleX = 0.7f * uiScale;
	float scaleY = 0.4f * uiScale;

	float boxX = indent + style.valueOffsetX * uiScale;
	float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;
	float labelBoxW = 170.0f * uiScale;

	float naturalWidth = measureTextWidth(item->label, scaleX, [&](float) { return 1.0f; });
	float rawRatio = labelBoxW / naturalWidth;
	float ratio = std::clamp(rawRatio, item->minCompressRatio, item->maxCompressRatio);

	auto ratioFunc = [&](float cursorX) {
		if (item->compressStartX >= 0.0f && cursorX < item->compressStartX)
			return 1.0f;
		return ratio;
	};

	std::string text = item->editing ? item->editBuffer
		: std::format("{:.6f}", item->floatValue);

	float naturalNumberWidth = measureTextWidth(text, scaleX, [&](float) {return 1.0f; });
	float numberBoxW = 170.0f * uiScale;

	float minRatio = std::max(0.3f, uiScale * 0.4f);
	float maxRatio = 0.5f;

	float rawNumberRatio = numberBoxW / naturalNumberWidth;
	float numberRatio = std::clamp(rawNumberRatio, minRatio, maxRatio);
	auto ratioFuncNumber = [numberRatio](float) { return numberRatio; };
	DrawTextBase(indent + style.labelOffsetX,
		y + style.labelOffsetY - 50.0f * uiScale,
		item->label,
		style.textColor,
		scaleX,
		scaleY, ratioFunc);
	item->labelHitX = indent + style.labelOffsetX;
	item->labelHitY = y + style.labelOffsetY - 50.0f * uiScale;

	DrawUIBase(boxX, boxY, numberBoxW, style.valueHeight, style.valueBgColor, 0.1f);


	if(item->editing && hasSelection(item))
	{
		int a = std::min(item->selectStart, item->selectEnd);
		int b = std::max(item->selectStart, item->selectEnd);

		float x1 = measureTextWidth(item->editBuffer.substr(0, a), scaleX, ratioFuncNumber);
		float x2 = measureTextWidth(item->editBuffer.substr(0, b), scaleX, ratioFuncNumber);

		DrawUIBase(boxX + 6 * uiScale + x1,
			boxY + 4 * uiScale,
			x2 - x1,
			style.valueHeight - 8 * uiScale,
			glm::vec4(0.3, 0.5, 1.0, 0.5), 0.3f);
	}

	if (item->editing)
	{
		bool showCursor = (fmod(glfwGetTime(), 1.0) < 0.5);

		if (showCursor)
		{
			float cursorX = measureTextWidth(item->editBuffer.substr(0, item->cursorPos), scaleX, ratioFuncNumber);

			DrawUIBase(
				boxX + 6 * uiScale + cursorX,
				boxY + 4 * uiScale,
				2.0f * uiScale,
				style.valueHeight - 8 * uiScale,
				glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), 0.4f
			);
		}
	}

	DrawTextBase(boxX + 6 * uiScale,
		boxY + 4 * uiScale,
		text,
		bg2Color,
		scaleX,
		scaleY, ratioFuncNumber);
}
glm::vec4 RenderContext::HSVtoRGB(const glm::vec3& hsv)
{
	float h = hsv.x * 6.0f;
	float s = hsv.y;
	float v = hsv.z;

	int i = int(floor(h));
	float f = h - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);

	glm::vec3 rgb;
	switch (i % 6) {
	case 0: rgb = { v, t, p }; break;
	case 1: rgb = { q, v, p }; break;
	case 2: rgb = { p, v, t }; break;
	case 3: rgb = { p, q, v }; break;
	case 4: rgb = { t, p, v }; break;
	case 5: rgb = { v, p, q }; break;
	}
	return glm::vec4(rgb, 1.0f);
}

bool RenderContext::mousePressedInside(float x, float y, float w, float h)
{
	if (!Input::GetMouseButtonDown(0))
		return false;

	float mx = Input::GetMousePosition().x;
	float my = Input::GetMousePosition().y;

	return (mx >= x && mx <= x + w &&
		my >= y && my <= y + h);
}
void RenderContext::DrawSlider(
	float x, float y, float w, float h,
	float& value, bool& dragging)
{
	float uiScale = mUIWidth / 512.0f;

	float padY = 8.0f * uiScale;
	float padX = 8.0f * uiScale;

	float centerY = y + h * 0.5f;

	float hitX = x - padX;
	float hitY = centerY - (h * 0.5f + padY);
	float hitW = w + padX * 2.0f;
	float hitH = h + padY * 2.0f;

	if (Input::GetMouseButtonDown(0))
	{
		if (mousePressedInside(hitX, hitY, hitW, hitH))
			dragging = true;
	}

	if (!Input::GetMouseButton(0))
		dragging = false;

	if (dragging)
	{
		float mx = Input::GetMousePosition().x;
		float local = std::clamp(mx - x, 0.0f, w);
		value = local / w;
	}

	float handleX = x + value * w;
	float handleW = 8.0f * uiScale;
	float handleH = h + 4.0f * uiScale;
	float handleY = y - 2.0f * uiScale;

	DrawUIBase(handleX - handleW * 0.5f, handleY, handleW, handleH,
		glm::vec4(1.0f), 0.3f);
}
std::string RenderContext::FormatColorCode(const glm::vec4& c)
{
	auto toHex = [](float v)
	{
		int iv = std::clamp(int(v * 255.0f), 0, 255);
		char buf[3];
		sprintf(buf, "%02X", iv);
		return std::string(buf);
	};

	return "#" +
		toHex(c.r) +
		toHex(c.g) +
		toHex(c.b) +
		toHex(c.a);
}
void RenderContext::DrawCheckerboard(float x, float y, float w, float h)
{
	float uiScale = mUIWidth / 512.0f;

	float cell = 6.0f * uiScale;

	glm::vec4 c1 = glm::vec4(0.75f, 0.75f, 0.75f, 1.0f);
	glm::vec4 c2 = glm::vec4(0.55f, 0.55f, 0.55f, 1.0f);

	int nx = (int)(w / cell) + 1;
	int ny = (int)(h / cell) + 1;

	for (int iy = 0; iy < ny; iy++)
	{
		for (int ix = 0; ix < nx; ix++)
		{
			bool even = ((ix + iy) % 2 == 0);
			glm::vec4 col = even ? c1 : c2;

			float px = x + ix * cell;
			float py = y + iy * cell;

			DrawUIBase(px, py, cell, cell, col, 0.0f);
		}
	}
}
void RenderContext::DrawColorPickerPopup(UIItem* item, float x, float y)
{
	float uiScale = mUIWidth / 512.0f;

	float w = 200 * uiScale;
	float h = 240 * uiScale;

	DrawUIBase(x, y, w, h, glm::vec4(0.1, 0.1, 0.1, 0.95), 0.2f);

	float barX = x + 10 * uiScale;
	float barW = w - 20 * uiScale;
	float barH = 12 * uiScale;

	float previewX = x + 10 * uiScale;
	float previewY = y + 10 * uiScale;
	float previewW = 40 * uiScale;
	float previewH = 40 * uiScale;

	DrawCheckerboard(previewX, previewY, previewW, previewH);
	DrawUIBase(previewX, previewY, previewW, previewH, item->colorValue, 0.0f);
	for (int i = 0; i < barW; i++)
	{
		float t = (float)i / (float)barW;
		glm::vec3 rgb = HSVtoRGB(glm::vec3(t, 1.0f, 1.0f));
		DrawUIBase(barX + i, y + 60 * uiScale, 1, barH, glm::vec4(rgb, 1.0f), 0.0f);
	}
	DrawSlider(barX, y + 60 * uiScale, barW, barH,
		item->hsv.x, item->dragHue);
	for (int i = 0; i < barW; i++)
	{
		float t = (float)i / (float)barW;
		glm::vec3 rgb = HSVtoRGB(glm::vec3(item->hsv.x, t, item->hsv.z));
		DrawUIBase(barX + i, y + 90 * uiScale, 1, barH, glm::vec4(rgb, 1.0f), 0.0f);
	}
	DrawSlider(barX, y + 90 * uiScale, barW, barH,
		item->hsv.y, item->dragSat);

	for (int i = 0; i < barW; i++)
	{
		float t = (float)i / (float)barW;
		glm::vec3 rgb = HSVtoRGB(glm::vec3(item->hsv.x, item->hsv.y, t));
		DrawUIBase(barX + i, y + 120 * uiScale, 1, barH, glm::vec4(rgb, 1.0f), 0.0f);
	}
	DrawSlider(barX, y + 120 * uiScale, barW, barH,
		item->hsv.z, item->dragVal);

	glm::vec4 bg = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
	DrawUIBase(barX, y + 150 * uiScale, barW, barH, bg, 0.2f);
	for (int i = 0; i < barW; i++)
	{
		float t = (float)i / (float)barW;
		glm::vec3 rgb = HSVtoRGB(item->hsv);
		DrawUIBase(barX + i, y + 150 * uiScale, 1, barH,
			glm::vec4(rgb, t), 0.0f);
	}
	DrawSlider(barX, y + 150 * uiScale, barW, barH,
		item->alpha, item->dragAlpha);

	float codeX = x + 10 * uiScale;
	float codeY = y + 170 * uiScale;
	float codeW = w - 20 * uiScale;
	float codeH = 40 * uiScale;

	DrawUIBase(codeX, codeY, codeW, codeH, glm::vec4(0.2, 0.2, 0.2, 1), 0.1f);

	DrawTextBase(
		codeX + 4 * uiScale,
		codeY + 4 * uiScale,
		item->colorCode,
		glm::vec4(1, 1, 1, 1),
		0.7f * uiScale,
		0.7f * uiScale,
		[&](float) { return 1.0f; }
	);

	if (item->dragHue || item->dragSat || item->dragVal || item->dragAlpha) {
		glm::vec3 rgb = HSVtoRGB(item->hsv);
		item->colorValue = glm::vec4(rgb, item->alpha);

		if (!item->editingColorCode)
			item->colorCode = FormatColorCode(item->colorValue);
	}
}
void RenderContext::DrawColor(UIItem* item, float indent, float y)
{
	float uiScale = mUIWidth / 512.0f;

	float scaleX = 0.7f * uiScale;
	float scaleY = 0.4f * uiScale;

	float labelX = indent + style.labelOffsetX;
	float labelY = y + style.labelOffsetY;

	float boxX = indent + style.valueOffsetX * uiScale;
	float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;
	float boxW = 40.0f * uiScale;
	float boxH = style.valueHeight;

	DrawTextBase(labelX, labelY, item->label, style.textColor,
		scaleX, scaleY, [&](float) { return 1.0f; });
	DrawUIBase(boxX - 2, boxY - 2, boxW + 4, boxH + 4, glm::vec4(1.0f,1.0f,1.0f,1.0f), 0.3f);
	DrawUIBase(boxX, boxY, boxW, boxH, item->colorValue, 0.3f);

	item->pickerX = boxX;
	item->pickerY = boxY + boxH + 4.0f * uiScale;
	item->pickerW = 200.0f * uiScale;
	item->pickerH = 240.0f * uiScale;

}
void RenderContext::DrawLabel(UIItem* item, float indent, float y)
{
	float scaleX = 1.0 * mUIWidth / 512.0f;
	float scaleY = 0.7 * mUIWidth / 512.0f;
	float boxW = 80.0f * mUIWidth / 512.0f;

	float naturalWidth = measureTextWidth(item->label, scaleX, [&](float) { return 1.0f; });
	float rawRatio = boxW / naturalWidth;
	float ratio = std::clamp(rawRatio, item->minCompressRatio, item->maxCompressRatio);

	auto ratioFunc = [&](float cursorX) {
		if (item->compressStartX >= 0.0f && cursorX < item->compressStartX)
			return 1.0f;
		return ratio;
	};
	float labelW = measureTextWidth(item->label, scaleX, ratioFunc);

	float labelH = style.rowHeight * scaleY * 2.0f;

	item->labelHitX = indent + style.labelOffsetX;
	item->labelHitY = y + style.labelOffsetY;
	item->labelHitW = labelW;
	item->labelHitH = style.rowHeight;
	DrawTextBase(
		indent + style.labelOffsetX,
		y + style.labelOffsetY,
		item->label,
		style.textColor,
		scaleX,
		scaleY, ratioFunc
	);
}
void RenderContext::DrawButton(UIItem* item, float indent, float y)
{
	if (!item->selectable) return;
	float uiScale = mUIWidth / 512.0f;

	float boxX = indent + style.labelOffsetX * uiScale;
	float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;
	float boxW = 320.0f * uiScale;
	float boxH = style.valueHeight;
	glm::vec4 bg = style.valueBgColor;
	if (item->pressed) bg = glm::vec4(bg.r * 0.85f, bg.g * 0.85f, bg.b * 0.85f, bg.a); 
	else if (item->hovered) bg = glm::vec4(bg.r * 1.05f, bg.g * 1.05f, bg.b * 1.05f, bg.a);
	DrawUIBase(boxX, boxY, boxW, boxH, bg, 0.3f);
}
void RenderContext::DrawValue(UIItem* item, float indent, float y)
{
	if (!item->selectable) return;
	switch (item->type)
	{
	case UIItemType::List:   DrawList(item, indent, y); break;
	case UIItemType::Number: DrawNumber(item, indent, y); break;
	case UIItemType::Float:  DrawFloat(item, indent, y); break;
	case UIItemType::Color:  DrawColor(item, indent, y); break;
	case UIItemType::Checkbox: DrawCheckbox(item, indent, y); break;
	default: break;
	}
}
void RenderContext::DrawRowBackground(const UIRow& row, float y, int index, float indent)
{
	glm::vec4 bg = (index % 2 == 0 ? style.rowBgColor1 : style.rowBgColor2);
	DrawUIBase(indent, y, mUIWidth - indent, style.rowHeight, bg, 0.0f);
}
glm::mat4 rotationFromTo(const glm::vec3& from, const glm::vec3& to)
{
	glm::vec3 f = glm::normalize(from);
	glm::vec3 t = glm::normalize(to);

	float cosTheta = glm::dot(f, t);

	if (cosTheta > 0.9999f)
		return glm::mat4(1.0f);

	if (cosTheta < -0.9999f)
	{
		glm::vec3 axis = glm::normalize(glm::cross(glm::vec3(1, 0, 0), f));
		if (glm::length(axis) < 0.01f)
			axis = glm::normalize(glm::cross(glm::vec3(0, 1, 0), f));
		return glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), axis);
	}

	glm::vec3 axis = glm::normalize(glm::cross(f, t));
	float angle = acosf(cosTheta);

	return glm::rotate(glm::mat4(1.0f), angle, axis);
}
void RenderContext::Render(GLFWwindow* mWindow, float deltaTime)
{
	elapsedTime += deltaTime;
	brresRenderer.packets.clear();
	int fbw, fbh;
	glfwGetFramebufferSize(mWindow, &fbw, &fbh);

	mUIWidth = fbw / 5.0f;
	float uiScale = mUIWidth / 512.0f;

	style.rowHeight = 40.0f * uiScale;
	style.valueHeight = 24.0f * uiScale;
	style.valueWidth = 120.0f * uiScale;
	style.labelOffsetX = 12.0f * uiScale;
	style.labelOffsetY = 6.0f * uiScale;
	style.triangleSize = 12.0f * uiScale;

	uiProj = glm::ortho(
		0.0f, (float)fbw,
		(float)fbh, 0.0f,
		-1.0f, 1.0f
	);
	if (mPrevWinWidth != fbw || mPrevWinHeight != fbh)
	{
		glDeleteFramebuffers(1, &mFbo);
		glDeleteTextures(1, &brresRenderer.zbufferTex);
		brresRenderer.zbufferTex = 0;

		InitFbo(fbw, fbh);

		mPrevWinWidth = fbw;
		mPrevWinHeight = fbh;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
	glViewport(mUIWidth, 0, fbw, fbh);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_FRAMEBUFFER_SRGB);
	if (!mLights.lights.empty())
	{
		SetLights(mLights);
	}
	if (!gMain->mKCLView)
		brresRenderer.Renderer(fbw, fbh, view, projection, bfgRes, gMain->mShowFogs);
	else
		kclRenderer.Render(vp, glm::vec3(glm::inverse(view)[3]));
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	vx = viewport[0];
	vy = viewport[1];
	vw = viewport[2];
	vh = viewport[3];
	if (startPositionItem != nullptr && startPositionItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.ktpt.size(); i++)
		{
			glUseProgram(ktptProgram);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			auto& p = loadedKMP.ktpt[i];
			glm::mat4 dir = glm::mat4(1.0f);

			dir = glm::translate(dir, p.pos);

			dir = glm::rotate(dir, glm::radians(p.rot.z), glm::vec3(0, 0, 1));

			dir = glm::rotate(dir, glm::radians(p.rot.y), glm::vec3(0, 1, 0));

			dir = glm::rotate(dir, glm::radians(p.rot.x), glm::vec3(1, 0, 0));

			float dist = glm::length(mCamera.GetPosition() - p.pos);
			float scale = dist * 0.00005f;
			float scale2 = dist * 0.00007f;
			if (i == hoverIndex && !editingDrag && !dragPending)
			{
				scale *= 1.5f;
				scale2 *= 1.5f;
			}
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_FALSE);
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(0, -1, 0));
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(-1, 0, 0));
			if (i == 0)
			{
				glm::mat4 mvpZone = vp * dir;

				glm::vec4 zoneColor = glm::vec4(0.25, 0.25, 1, 0.5);
				glUniform4fv(u_color, 1, glm::value_ptr(zoneColor));
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpZone));

				if (loadedKMP.stgi.dist == 0)
				{
					glBindVertexArray(startZoneWideModel->vao);
					glDrawArrays(GL_TRIANGLES, 0, startZoneWideModel->vertexCount);
				}
				else
				{
					glBindVertexArray(startZoneNarrowModel->vao);
					glDrawArrays(GL_TRIANGLES, 0, startZoneNarrowModel->vertexCount);
				}
			}
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
			dir = glm::rotate(dir, glm::radians(-180.0f), glm::vec3(0, 1, 0));
			dir = glm::rotate(dir, glm::radians(0.0f), glm::vec3(0, 0, 1));
			glm::mat4 sphereM = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
			glm::mat4 sphereM2 = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale2));
			glm::mat4 mvpSphere = vp * sphereM;
			glm::mat4 mvpSphere2 = vp * sphereM2;

			glm::vec4 sphereColor = glm::vec4(0, 0, 1, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
			glBindVertexArray(modelPoint->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);

			glUseProgram(normalProgram);
			glm::vec4 pathColor = glm::vec4(0.75, 0.75, 1, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(pathColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelPath->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPath->vertexCount);

			glm::vec4 arrowColor = glm::vec4(0, 0, 1, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrow->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrow->vertexCount);

			glm::vec4 arrowUpColor = glm::vec4(0.25, 0.25, 1, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowUpColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrowUp->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrowUp->vertexCount);
			if (loadedKMP.ktpt[i].selected)
			{
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				glUseProgram(ktptProgram);
				glm::vec4 sphereColorSelection = glm::vec4(0.5, 0.5, 1, 1);

				glUniform4fv(u_color, 1, glm::value_ptr(sphereColorSelection));
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPointSelection->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPointSelection->vertexCount);

				glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPoint->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
			}

		}
	}

	if (cameItem != nullptr && cameItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.came.size(); i++)
		{
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glUseProgram(ktptProgram);
			auto& p = loadedKMP.came[i];
			glm::mat4 dir = glm::mat4(1.0f);

			auto VSDir = glm::translate(dir, p.viewStart);
			auto VEDir = glm::translate(dir, p.viewEnd);
			dir = glm::translate(dir, p.pos);
			dir = glm::rotate(dir, glm::radians(p.rot.z), glm::vec3(0, 0, 1));

			dir = glm::rotate(dir, glm::radians(p.rot.y), glm::vec3(0, 1, 0));

			dir = glm::rotate(dir, glm::radians(p.rot.x), glm::vec3(1, 0, 0));
			float dist = glm::length(mCamera.GetPosition() - p.pos);
			float dist2 = glm::length(mCamera.GetPosition() - p.viewStart);
			float dist3 = glm::length(mCamera.GetPosition() - p.viewEnd);
			float scale = dist * 0.00005f;
			float scale2 = dist * 0.00007f;
			float scale3 = dist2 * 0.00005f;
			float scale4 = dist3 * 0.00005f;
			if (i == hoverIndex && !editingDrag && !dragPending)
			{
				scale *= 1.5f;
				scale2 *= 1.5f;
			}
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(0, -1, 0));
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(-1, 0, 0));
			dir = glm::rotate(dir, glm::radians(-180.0f), glm::vec3(0, 1, 0));
			dir = glm::rotate(dir, glm::radians(0.0f), glm::vec3(0, 0, 1));
			glm::mat4 sphereM = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
			glm::mat4 sphereM2 = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale2));
			glm::mat4 sphereM3 = VSDir * glm::scale(glm::mat4(1.0f), glm::vec3(scale3));
			glm::mat4 sphereM4 = VEDir * glm::scale(glm::mat4(1.0f), glm::vec3(scale4));
			glm::mat4 mvpSphere = vp * sphereM;
			glm::mat4 mvpSphere2 = vp * sphereM2;
			glm::mat4 mvpSphere3 = vp * sphereM3;
			glm::mat4 mvpSphere4 = vp * sphereM4;
			glm::vec4 sphereColor = glm::vec4(0.5, 0.1, 0.7, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
			glBindVertexArray(modelPoint->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);

			glUseProgram(normalProgram);
			glm::vec4 pathColor = glm::vec4(0.7, 0.1, 1, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(pathColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelPath->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPath->vertexCount);

			glm::vec4 arrowColor = glm::vec4(0.5, 0.1, 0.8, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrow->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrow->vertexCount);

			glm::vec4 arrowUpColor = glm::vec4(0.75, 0.1, 1, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowUpColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrowUp->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrowUp->vertexCount);

			glm::vec4 viewStartSphere = glm::vec4(0.0, 1.0, 0.0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(viewStartSphere));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere3));
			glBindVertexArray(modelPoint->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);

			glm::vec4 viewEndSphere = glm::vec4(1.0, 0.0, 0.0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(viewEndSphere));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere4));
			glBindVertexArray(modelPoint->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
			if (loadedKMP.came[i].selected)
			{

				glUseProgram(ktptProgram);
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				glm::vec4 sphereColorSelection = glm::vec4(0.7, 0.1, 1, 1);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColorSelection));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPointSelection->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPointSelection->vertexCount);

				glm::vec4 viewStartSphere = glm::vec4(0.5, 1.0, 0.5, 1);
				glUniform4fv(u_color, 1, glm::value_ptr(viewStartSphere));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere3));
				glBindVertexArray(modelPointSelection->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPointSelection->vertexCount);

				glm::vec4 viewEndSphere = glm::vec4(1.0, 0.5, 0.5, 1);
				glUniform4fv(u_color, 1, glm::value_ptr(viewEndSphere));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere4));
				glBindVertexArray(modelPointSelection->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPointSelection->vertexCount);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPoint->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
			}

		}
	}

	if (jgptItem != nullptr && jgptItem->expanded)
	{

		for (size_t i = 0; i < loadedKMP.jgpt.size(); i++)
		{
			glUseProgram(ktptProgram);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			auto& p = loadedKMP.jgpt[i];
			glm::mat4 dir = glm::mat4(1.0f);

			dir = glm::translate(dir, p.pos);

			dir = glm::rotate(dir, glm::radians(p.rot.z), glm::vec3(0, 0, 1));

			dir = glm::rotate(dir, glm::radians(p.rot.y), glm::vec3(0, 1, 0));

			dir = glm::rotate(dir, glm::radians(p.rot.x), glm::vec3(1, 0, 0));

			float dist = glm::length(mCamera.GetPosition() - p.pos);
			float scale = dist * 0.00005f;
			float scale2 = dist * 0.00007f;
			if (i == hoverIndex && !editingDrag && !dragPending)
			{
				scale *= 1.5f;
				scale2 *= 1.5f;
			}
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(0, -1, 0));
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(-1, 0, 0));
			dir = glm::rotate(dir, glm::radians(-180.0f), glm::vec3(0, 1, 0));
			dir = glm::rotate(dir, glm::radians(0.0f), glm::vec3(0, 0, 1));
			glm::mat4 sphereM = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
			glm::mat4 sphereM2 = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale2));
			glm::mat4 mvpSphere = vp * sphereM;
			glm::mat4 mvpSphere2 = vp * sphereM2;
			glm::vec4 sphereColor = glm::vec4(0.55, 0.55, 0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
			glBindVertexArray(modelPoint->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
			glUseProgram(normalProgram);
			glm::vec4 pathColor = glm::vec4(0.85, 0.85, 0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(pathColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelPath->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPath->vertexCount);

			glm::vec4 arrowColor = glm::vec4(0.75, 0.75, 0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrow->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrow->vertexCount);

			glm::vec4 arrowUpColor = glm::vec4(0.5, 0.5, 0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowUpColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrowUp->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrowUp->vertexCount);
						dir = glm::rotate(dir, glm::radians(180.0f), glm::vec3(1, 0, 0));
						dir = glm::rotate(dir, glm::radians(180.0f), glm::vec3(0, 1, 0));
			int k = 0;
			for (int ix = -600; ix <= 0; ix += 300)
			{
				for (int jy = -450; jy <= 450; jy += 300)
				{
					glm::mat4 slotM =
						dir * glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -550)) *
						glm::translate(glm::mat4(1.0f), glm::vec3(ix, jy, 0));

					glm::mat4 mvpSlot = vp * slotM;

					glm::vec4 slotColor = glm::vec4(0.75f, 0.75f, 0.0f, 1.0f);
					glUniform4fv(u_color, 1, glm::value_ptr(slotColor));
					glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSlot));

					glBindVertexArray(modelPlayerPos->vao);
					glDrawArrays(GL_TRIANGLES, 0, modelPlayerPos->vertexCount);

					k++;
				}
			}
			if (loadedKMP.jgpt[i].selected)
			{
				glUseProgram(ktptProgram);
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				glm::vec4 sphereColorSelection = glm::vec4(0.95, 0.95, 0, 1);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColorSelection));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPointSelection->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPointSelection->vertexCount);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPoint->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
			}
		}
	}

	if (msptItem != nullptr && msptItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.mspt.size(); i++)
		{
			glUseProgram(ktptProgram);
			auto& p = loadedKMP.mspt[i];
			glm::mat4 dir = glm::mat4(1.0f);

			dir = glm::translate(dir, p.pos);

			dir = glm::rotate(dir, glm::radians(p.rot.z), glm::vec3(0, 0, 1));

			dir = glm::rotate(dir, glm::radians(p.rot.y), glm::vec3(0, 1, 0));

			dir = glm::rotate(dir, glm::radians(p.rot.x), glm::vec3(1, 0, 0));

			float dist = glm::length(mCamera.GetPosition() - p.pos);
			float scale = dist * 0.00005f;
			float scale2 = dist * 0.00007f;
			if (i == hoverIndex && !editingDrag && !dragPending)
			{
				scale *= 1.5f;
				scale2 *= 1.5f;
			}
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(0, -1, 0));
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(-1, 0, 0));
			dir = glm::rotate(dir, glm::radians(-180.0f), glm::vec3(0, 1, 0));
			dir = glm::rotate(dir, glm::radians(0.0f), glm::vec3(0, 0, 1));
			glm::mat4 sphereM = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
			glm::mat4 sphereM2 = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale2));
			glm::mat4 mvpSphere = vp * sphereM;
			glm::mat4 mvpSphere2 = vp * sphereM2;
			glm::vec4 sphereColor = glm::vec4(0.5, 0, 1, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
			glBindVertexArray(modelPoint->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glUseProgram(normalProgram);
			glm::vec4 pathColor = glm::vec4(0.87, 0.75, 1, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(pathColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelPath->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPath->vertexCount);

			glm::vec4 arrowColor = glm::vec4(0.5, 0, 1, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrow->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrow->vertexCount);

			glm::vec4 arrowUpColor = glm::vec4(0.5, 0.25, 1, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowUpColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrowUp->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrowUp->vertexCount);

			if (loadedKMP.mspt[i].selected)
			{
				glUseProgram(ktptProgram);
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				glm::vec4 sphereColorSelection = glm::vec4(0.75, 0.5, 1, 1);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColorSelection));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPointSelection->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPointSelection->vertexCount);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPoint->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
			}
		}
	}

	if (areaItem != nullptr && areaItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.area.size(); i++)
		{
			glUseProgram(ktptProgram);
			auto& p = loadedKMP.area[i];
			glm::mat4 dir = glm::mat4(1.0f);

			dir = glm::translate(dir, p.pos);

			dir = glm::rotate(dir, glm::radians(p.rot.z), glm::vec3(0, 0, 1));

			dir = glm::rotate(dir, glm::radians(p.rot.y), glm::vec3(0, 1, 0));

			dir = glm::rotate(dir, glm::radians(p.rot.x), glm::vec3(1, 0, 0));
			float dist = glm::length(mCamera.GetPosition() - p.pos);
			float scale = dist * 0.00005f;
			float scale2 = dist * 0.00007f;
			if (i == hoverIndex && !editingDrag && !dragPending)
			{
				scale *= 1.5f;
				scale2 *= 1.5f;
			}
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(0, -1, 0));
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(-1, 0, 0));
			auto dir2 = glm::rotate(dir, glm::radians(180.0f), glm::vec3(0, 1, 0));
			dir2 = glm::rotate(dir2, glm::radians(-90.0f), glm::vec3(0, 0, 1));
			auto areaScale = glm::scale(dir2, p.scale);
			dir = glm::rotate(dir, glm::radians(-180.0f), glm::vec3(0, 1, 0));
			dir = glm::rotate(dir, glm::radians(0.0f), glm::vec3(0, 0, 1));
			glm::mat4 sphereM = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
			glm::mat4 sphereM2 = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale2));
			glm::mat4 mvpSphere = vp * sphereM;
			glm::mat4 mvpSphere2 = vp * sphereM2;
			glm::mat4 mvpArea = vp * areaScale;
			glm::vec4 sphereColor = glm::vec4(1, 0.5, 0, 1);
			glm::vec4 areaColor = glm::vec4(1, 0.7, 0, 0.5);
			glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
			glBindVertexArray(modelPoint->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glUseProgram(normalProgram);
			glm::vec4 pathColor = glm::vec4(1, 0.7, 0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(pathColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelPath->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPath->vertexCount);

			glm::vec4 arrowColor = glm::vec4(1, 0.35, 0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrow->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrow->vertexCount);

			glm::vec4 arrowUpColor = glm::vec4(0.75, 0.5, 0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowUpColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrowUp->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrowUp->vertexCount);
			glUseProgram(ktptProgram);
			glUniform4fv(u_color, 1, glm::value_ptr(areaColor));
			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpArea));
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			if (loadedKMP.area[i].shape == 0)
			{
				glBindVertexArray(modelAreaBox->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelAreaBox->vertexCount);
			}
			else
			{
				glBindVertexArray(modelAreaCylinder->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelAreaCylinder->vertexCount);
			}
			glDisable(GL_BLEND);
			if (loadedKMP.area[i].selected)
			{
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				glm::vec4 sphereColorSelection = glm::vec4(1, 0.7, 0, 1);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColorSelection));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPointSelection->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPointSelection->vertexCount);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPoint->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
			}
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
		}
	}

	if (cnptItem != nullptr && cnptItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.cnpt.size(); i++)
		{
			glUseProgram(normalProgram);
			auto& p = loadedKMP.cnpt[i];
			glm::mat4 dir = glm::mat4(1.0f);

			dir = glm::translate(dir, p.pos);

			dir = glm::rotate(dir, glm::radians(p.rot.z), glm::vec3(0, 0, 1));

			dir = glm::rotate(dir, glm::radians(p.rot.y), glm::vec3(0, 1, 0));

			dir = glm::rotate(dir, glm::radians(p.rot.x), glm::vec3(1, 0, 0));

			float dist = glm::length(mCamera.GetPosition() - p.pos);
			float scale = dist * 0.00005f;
			float scale2 = dist * 0.00007f;
			if (i == hoverIndex && !editingDrag && !dragPending)
			{
				scale *= 1.5f;
				scale2 *= 1.5f;
			}
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(0, -1, 0));
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(-1, 0, 0));
			dir = glm::rotate(dir, glm::radians(-180.0f), glm::vec3(0, 1, 0));
			dir = glm::rotate(dir, glm::radians(0.0f), glm::vec3(0, 0, 1));
			glm::mat4 sphereM = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
			glm::mat4 sphereM2 = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale2));
			glm::mat4 mvpSphere = vp * sphereM;
			glm::mat4 mvpSphere2 = vp * sphereM2;

						glUseProgram(ktptProgram);
			glm::vec4 sphereColor = glm::vec4(1, 0, 0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
			glBindVertexArray(modelPoint->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glUseProgram(normalProgram);
			glm::vec4 pathColor = glm::vec4(1, 0.75, 0.75, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(pathColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelPath->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelPath->vertexCount);

			glm::vec4 arrowColor = glm::vec4(1, 0, 0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrow->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrow->vertexCount);

			glm::vec4 arrowUpColor = glm::vec4(1, 0, 0, 1);
			glUniform4fv(u_color, 1, glm::value_ptr(arrowUpColor));

			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere));
			glBindVertexArray(modelArrowUp->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelArrowUp->vertexCount);
			if (loadedKMP.cnpt[i].selected)
			{
				glUseProgram(ktptProgram);
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				glm::vec4 sphereColorSelection = glm::vec4(1, 0.5, 0.5, 1);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColorSelection));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPointSelection->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPointSelection->vertexCount);
				glm::mat4 scale3 = glm::scale(glm::mat4(1.0f), glm::vec3(1000000.0f, 1.0f, 100000.0f));
				glm::mat4 planeM = dir * scale3;
				glm::mat4 mvpPlane = vp * planeM;
				glm::vec4 panelColor = glm::vec4(1, 0.5, 0.5, 0.5);
				glUniform4fv(u_color, 1, glm::value_ptr(panelColor));
				glBindVertexArray(modelPanel->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPanel->vertexCount);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPoint->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
			}
		}
	}
	if (enptItem != nullptr && enptItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.enph.size(); i++)
		{
			auto& path = loadedKMP.enph[i];

			int start = path.start;
			int end = start + path.num - 1;
			glUseProgram(normalProgram);
			for (int p = start; p < end; p++)
			{
				glm::vec4 pathColor =
					(loadedKMP.enpt[p].s1 == 1)
					? glm::vec4(1, 0.5, 0.75, 1)
					: glm::vec4(1, 0.5, 0, 1);

				glm::vec3 posA = loadedKMP.enpt[p].pos;
				glm::vec3 posB = loadedKMP.enpt[p + 1].pos;

				glm::vec3 dir = posB - posA;
				float len = glm::length(dir);
				len /= 1000.0f;
				glm::vec3 dirNorm = glm::normalize(dir);


				float distA = glm::length(mCamera.GetPosition() - posA);
				float distB = glm::length(mCamera.GetPosition() - posB);

				float scaleA = distA * 0.00003f;
				float scaleB = distB * 0.00003f;

				float scale = std::min(scaleA, scaleB);

				glm::mat4 m = glm::mat4(1.0f);

				glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(len, scale, scale));

				glm::mat4 R = rotationFromTo(glm::vec3(-1, 0, 0), dirNorm);

				glm::mat4 T = glm::translate(glm::mat4(1.0f), posA);

				m = T * R * S;
				glm::mat4 mArrow = glm::translate(glm::mat4(1.0f), posB);
				mArrow *= rotationFromTo(glm::vec3(-1, 0, 0), dirNorm);
				mArrow = glm::scale(mArrow, glm::vec3(scale));

				glm::mat4 mvp = vp * m;
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
				glUniform4fv(u_color, 1, glm::value_ptr(pathColor));

				glBindVertexArray(modelPath->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPath->vertexCount);

				glm::mat4 mvpArrow = vp * mArrow;
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpArrow));
				glUniform4fv(u_color, 1, glm::value_ptr(glm::vec4(1, 0.75, 0, 1)));

				glBindVertexArray(modelArrow->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelArrow->vertexCount);
			}

			for (int g = 0; g < 6; g++)
			{
				int nextPath = path.next[g];
				if (nextPath == 0xFF || nextPath >= loadedKMP.enph.size()) continue;

				int lastPoint = end;
				int nextIndex = loadedKMP.enph[nextPath].start;

				glm::vec4 pathColor =
					(loadedKMP.enpt[lastPoint].s1 == 1)
					? glm::vec4(1, 0.5, 0.75, 1)
					: glm::vec4(1, 0.5, 0, 1);

				glm::vec3 posA = loadedKMP.enpt[lastPoint].pos;
				glm::vec3 posB = loadedKMP.enpt[nextIndex].pos;

				glm::vec3 dir = posB - posA;
				float len = glm::length(dir);
				len /= 1000.0f;
				glm::vec3 dirNorm = glm::normalize(dir);

				float distA = glm::length(mCamera.GetPosition() - posA);
				float distB = glm::length(mCamera.GetPosition() - posB);

				float scaleA = distA * 0.00003f;
				float scaleB = distB * 0.00003f;

				float scale = std::min(scaleA, scaleB);

				glm::mat4 m = glm::mat4(1.0f);

				glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(len, scale, scale));

				glm::mat4 R = rotationFromTo(glm::vec3(-1, 0, 0), dirNorm);

				glm::mat4 T = glm::translate(glm::mat4(1.0f), posA);

				m = T * R * S;

				glm::mat4 mArrow = glm::translate(glm::mat4(1.0f), posB);
				mArrow *= rotationFromTo(glm::vec3(-1, 0, 0), dirNorm);
				mArrow = glm::scale(mArrow, glm::vec3(scale));

				glm::mat4 mvp = vp * m;
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
				glUniform4fv(u_color, 1, glm::value_ptr(pathColor));

				glBindVertexArray(modelPath->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPath->vertexCount);

				glm::mat4 mvpArrow = vp * mArrow;
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpArrow));
				glUniform4fv(u_color, 1, glm::value_ptr(glm::vec4(1, 0.75, 0, 1)));

				glBindVertexArray(modelArrow->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelArrow->vertexCount);
			}
		}

		for (size_t i = 0; i < loadedKMP.enpt.size(); i++)
		{
			GLboolean prevCull = glIsEnabled(GL_CULL_FACE);
			GLboolean prevBlend = glIsEnabled(GL_BLEND);
			GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);

			GLint prevDepthFunc; glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
			GLboolean prevDepthMask; glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
			glUseProgram(ktptProgram);
			auto& p = loadedKMP.enpt[i];
			glm::mat4 dir = glm::mat4(1.0f);

			dir = glm::translate(dir, p.pos);

			float dist = glm::length(mCamera.GetPosition() - p.pos);
			float scale = dist * 0.00005f;
			float scale2 = dist * 0.00007f;
			if (i == hoverIndex && !editingDrag && !dragPending)
			{
				scale *= 1.5f;
				scale2 *= 1.5f;
			}
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(0, -1, 0));
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(-1, 0, 0));
			dir = glm::rotate(dir, glm::radians(-180.0f), glm::vec3(0, 1, 0));
			dir = glm::rotate(dir, glm::radians(0.0f), glm::vec3(0, 0, 1));
			glm::mat4 sphereM = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
			glm::mat4 sphereM2 = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale2));
			glm::mat4 mvpSphere = vp * sphereM;
			glm::mat4 mvpSphere2 = vp * sphereM2;
			float sizeCircle = p.deviation * 50.f;
			glm::mat4 circleM = dir * glm::scale(glm::mat4(1.0f), glm::vec3(sizeCircle));
			glm::mat4 mvpSphere3 = vp * circleM;
			glm::vec4 sizeColor = glm::vec4(1, 0.5, 0, 0.5);
			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere3));
			glUniform4fv(u_color, 1, glm::value_ptr(sizeColor));

			/*glDisable(GL_CULL_FACE);
			glDepthFunc(GL_GEQUAL);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glDepthMask(GL_TRUE);

			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(-1.0f, -1.0f);
			glUniform4fv(u_color, 1, glm::value_ptr(sizeColor));
			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere3));
			glBindVertexArray(modelSizeCircle->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelSizeCircle->vertexCount);
			glDisable(GL_POLYGON_OFFSET_FILL);*/
			glDepthFunc(GL_LESS);
			glDisable(GL_CULL_FACE);
			glm::vec4 sphereColor = p.s1 == 0 ? glm::vec4(0.6, 0, 0, 1) : p.s1 == 1 ? glm::vec4(1, 0.5, 0.95, 1) : glm::vec4(1, 0, 0, 1);
			if (loadedKMP.enpt[i].selected)
			{
				glDepthMask(GL_FALSE);

				glm::vec4 sphereColorSelection = glm::vec4(1, 0.5, 0.5, 1);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColorSelection));

				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPointSelection->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPointSelection->vertexCount);

				glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPoint->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
				glDepthMask(GL_TRUE);
				glEnable(GL_DEPTH_TEST);
			}
			else
			{
				glDepthMask(GL_TRUE);
				glEnable(GL_DEPTH_TEST);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPoint->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
			}
			if (prevCull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
			if (prevBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
			if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

			glDepthFunc(prevDepthFunc);
			glDepthMask(prevDepthMask);
		}
	}

	if (itptItem != nullptr && itptItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.itph.size(); i++)
		{
			auto& path = loadedKMP.itph[i];

			int start = path.start;
			int end = start + path.num - 1;
			glUseProgram(normalProgram);
			for (int p = start; p < end; p++)
			{
				glm::vec4 pathColor;
				glm::vec4 arrowColor;
				if (loadedKMP.itpt[p].s2 == 0)
				{
					pathColor = glm::vec4(0.5, 1, 0, 1);
					arrowColor = glm::vec4(0.1, 0.8, 0, 1);
				}
				if (loadedKMP.itpt[p].s2 == 1)
				{
					pathColor = glm::vec4(0.5, 0.5, 0.5, 1);
					arrowColor = glm::vec4(0.85, 0.85, 0.85, 1);
				}
				if (loadedKMP.itpt[p].s2 == 2)
				{
					pathColor = glm::vec4(0.5, 1, 0.8, 1);
					arrowColor = glm::vec4(0.1, 0.8, 0, 1);
				}
				if (loadedKMP.itpt[p].s2 == 3)
				{
					pathColor = glm::vec4(0.8, 1, 0.5, 1);
					arrowColor = glm::vec4(0.85, 0.85, 0.85, 1);
				}
				glm::vec3 posA = loadedKMP.itpt[p].pos;
				glm::vec3 posB = loadedKMP.itpt[p + 1].pos;

				glm::vec3 dir = posB - posA;
				float len = glm::length(dir);
				len /= 1000.0f;
				glm::vec3 dirNorm = glm::normalize(dir);


				float distA = glm::length(mCamera.GetPosition() - posA);
				float distB = glm::length(mCamera.GetPosition() - posB);

				float scaleA = distA * 0.00003f;
				float scaleB = distB * 0.00003f;

				float scale = std::min(scaleA, scaleB);

				glm::mat4 m = glm::mat4(1.0f);

				glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(len, scale, scale));

				glm::mat4 R = rotationFromTo(glm::vec3(-1, 0, 0), dirNorm);

				glm::mat4 T = glm::translate(glm::mat4(1.0f), posA);

				m = T * R * S;
				glm::mat4 mArrow = glm::translate(glm::mat4(1.0f), posB);
				mArrow *= rotationFromTo(glm::vec3(-1, 0, 0), dirNorm);
				mArrow = glm::scale(mArrow, glm::vec3(scale));

				glm::mat4 mvp = vp * m;
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
				glUniform4fv(u_color, 1, glm::value_ptr(pathColor));

				glBindVertexArray(modelPath->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPath->vertexCount);

				glm::mat4 mvpArrow = vp * mArrow;
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpArrow));
				glUniform4fv(u_color, 1, glm::value_ptr(arrowColor));

				glBindVertexArray(modelArrow->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelArrow->vertexCount);
			}

			for (int g = 0; g < 6; g++)
			{
				int nextPath = path.next[g];
				if (nextPath == 0xFF || nextPath >= loadedKMP.itph.size()) continue;

				int lastPoint = end;
				int nextIndex = loadedKMP.itph[nextPath].start;

				glm::vec4 pathColor;
				glm::vec4 arrowColor;
				if (loadedKMP.itpt[lastPoint].s2 == 0)
				{
					pathColor = glm::vec4(0.5, 1, 0, 1);
					arrowColor = glm::vec4(0.1, 0.8, 0, 1);
				}
				if (loadedKMP.itpt[lastPoint].s2 == 1)
				{
					pathColor = glm::vec4(0.5, 0.5, 0.5, 1);
					arrowColor = glm::vec4(0.85, 0.85, 0.85, 1);
				}
				if (loadedKMP.itpt[lastPoint].s2 == 2)
				{
					pathColor = glm::vec4(0.5, 1, 0.8, 1);
					arrowColor = glm::vec4(0.1, 0.8, 0, 1);
				}
				if (loadedKMP.itpt[lastPoint].s2 == 3)
				{
					pathColor = glm::vec4(0.8, 1, 0.5, 1);
					arrowColor = glm::vec4(0.85, 0.85, 0.85, 1);
				}

				glm::vec3 posA = loadedKMP.itpt[lastPoint].pos;
				glm::vec3 posB = loadedKMP.itpt[nextIndex].pos;

				glm::vec3 dir = posB - posA;
				float len = glm::length(dir);
				len /= 1000.0f;
				glm::vec3 dirNorm = glm::normalize(dir);

				float distA = glm::length(mCamera.GetPosition() - posA);
				float distB = glm::length(mCamera.GetPosition() - posB);

				float scaleA = distA * 0.00003f;
				float scaleB = distB * 0.00003f;

				float scale = std::min(scaleA, scaleB);

				glm::mat4 m = glm::mat4(1.0f);

				glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(len, scale, scale));

				glm::mat4 R = rotationFromTo(glm::vec3(-1, 0, 0), dirNorm);

				glm::mat4 T = glm::translate(glm::mat4(1.0f), posA);

				m = T * R * S;

				glm::mat4 mArrow = glm::translate(glm::mat4(1.0f), posB);
				mArrow *= rotationFromTo(glm::vec3(-1, 0, 0), dirNorm);
				mArrow = glm::scale(mArrow, glm::vec3(scale));

				glm::mat4 mvp = vp * m;
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
				glUniform4fv(u_color, 1, glm::value_ptr(pathColor));

				glBindVertexArray(modelPath->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPath->vertexCount);

				glm::mat4 mvpArrow = vp * mArrow;
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpArrow));
				glUniform4fv(u_color, 1, glm::value_ptr(arrowColor));

				glBindVertexArray(modelArrow->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelArrow->vertexCount);
			}
		}
		for (size_t i = 0; i < loadedKMP.itpt.size(); i++)
		{
			GLboolean prevCull = glIsEnabled(GL_CULL_FACE);
			GLboolean prevBlend = glIsEnabled(GL_BLEND);
			GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);

			GLint prevDepthFunc; glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
			GLboolean prevDepthMask; glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
			glUseProgram(ktptProgram);
			auto& p = loadedKMP.itpt[i];
			glm::mat4 dir = glm::mat4(1.0f);

			dir = glm::translate(dir, p.pos);

			float dist = glm::length(mCamera.GetPosition() - p.pos);
			float scale = dist * 0.00005f;
			float scale2 = dist * 0.00007f;
			if (i == hoverIndex && !editingDrag && !dragPending)
			{
				scale *= 1.5f;
				scale2 *= 1.5f;
			}
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(0, -1, 0));
			dir = glm::rotate(dir, glm::radians(90.0f), glm::vec3(-1, 0, 0));
			dir = glm::rotate(dir, glm::radians(-180.0f), glm::vec3(0, 1, 0));
			dir = glm::rotate(dir, glm::radians(0.0f), glm::vec3(0, 0, 1));
			glm::mat4 sphereM = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
			glm::mat4 sphereM2 = dir * glm::scale(glm::mat4(1.0f), glm::vec3(scale2));
			glm::mat4 mvpSphere = vp * sphereM;
			glm::mat4 mvpSphere2 = vp * sphereM2;
			float sizeCircle = p.deviation * 50.f;
			glm::mat4 circleM = dir * glm::scale(glm::mat4(1.0f), glm::vec3(sizeCircle));
			glm::mat4 mvpSphere3 = vp * circleM;
			glm::vec4 sphereColor = p.s1 == 0 ? glm::vec4(0, 0.4, 0, 1) : p.s1 == 1 ? glm::vec4(0.75, 0.75, 0.75, 1) : glm::vec4(0, 0.8, 0, 1);
			glm::vec4 sizeColor = glm::vec4(0.25, 0.8, 0, 0.5);
			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere3));
			glUniform4fv(u_color, 1, glm::value_ptr(sizeColor));

			/*glDisable(GL_CULL_FACE);
			glDepthFunc(GL_GEQUAL);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glDepthMask(GL_TRUE);

			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(-1.0f, -1.0f);
			glUniform4fv(u_color, 1, glm::value_ptr(sizeColor));
			glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere3));
			glBindVertexArray(modelSizeCircle->vao);
			glDrawArrays(GL_TRIANGLES, 0, modelSizeCircle->vertexCount);
			glDisable(GL_POLYGON_OFFSET_FILL);*/
			glDepthFunc(GL_LESS);
			glDisable(GL_CULL_FACE);
			if (loadedKMP.itpt[i].selected)
			{
				glDepthMask(GL_FALSE);

				glm::vec4 sphereColorSelection = glm::vec4(0.4, 1, 0.1, 1);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColorSelection));
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPointSelection->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPointSelection->vertexCount);

				glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPointSelection->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPointSelection->vertexCount);
				glDepthMask(GL_TRUE);
				glEnable(GL_DEPTH_TEST);
			}
			else
			{
				glDepthMask(GL_TRUE);
				glEnable(GL_DEPTH_TEST);
				glUniform4fv(u_color, 1, glm::value_ptr(sphereColor));
				glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvpSphere2));
				glBindVertexArray(modelPoint->vao);
				glDrawArrays(GL_TRIANGLES, 0, modelPoint->vertexCount);
			}
			if (prevCull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
			if (prevBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
			if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

			for (int p = 0; p < loadedKMP.enph.size(); p++)
			{

			}

			glDepthFunc(prevDepthFunc);
			glDepthMask(prevDepthMask);
		}
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_FRAMEBUFFER_SRGB);
	posteffect2.updateBDOFUniforms(vp, elapsedTime, bdofRes);
	if (gMain->mShowPostEffects)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
		glViewport(0, 0, fbw, fbh);
		posteffect2.Render(fbw, fbh, mFbo, bblmRes, mColorTex);
	}
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_COLOR_LOGIC_OP);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mFbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDisable(GL_BLEND);

	glBlitFramebuffer(
		0, 0, fbw, fbh,
		0, 0, fbw, fbh,
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST
	);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fbw, fbh);

	rows.clear();
	flatten(mainPanel, rows);

	listHeight = rows.size() * style.rowHeight + 20.0f;
	float maxScroll = std::max(0.0f, listHeight - mPrevWinHeight);
	listScroll = std::clamp(listScroll, 0.0f, maxScroll);

	DrawUIBase(0, 0, mUIWidth, fbh, bgColor, 0.01f);
	activeColorPicker = nullptr;

	for (auto& row : rows)
	{
		if (row.value &&
			row.value->type == UIItemType::Color &&
			row.value->pickingColor)
		{
			activeColorPicker = row.value;
			break;
		}
	}
	int drawIndex = 0;

	for (int i = 0; i < (int)rows.size(); i++)
	{
		UIRow& row = rows[i];

		if (row.value && row.value->invisible)
			continue;

		float y = 10.0f + drawIndex * style.rowHeight - listScroll;
		float indent = row.depth * 20.0f * uiScale;

		DrawRowBackground(row, y, drawIndex, indent);

		if (row.label) {
			if (row.label->type == UIItemType::Group)
				DrawGroup(row.label, indent, y);
			else
			{
				if (row.value->type == UIItemType::Button)
				{
					DrawButton(row.value, indent, y);
					style.textColor = glm::vec4(0,0, 0, 1);
					DrawLabel(row.label, indent, y);
					style.textColor = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
				}
				else
				{
					style.textColor = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
					DrawLabel(row.label, indent, y);
				}
			}
		}

		if (row.value)
			DrawValue(row.value, indent, y);
		row.drawIndex = drawIndex;
		drawIndex++;
	}

	float barX = mUIWidth - 8.0f;
	float barW = 8.0f;
	float barH = mPrevWinHeight;

	DrawUIBase(barX, 0, barW, barH, barColor, 0.02f);

	float knobH = (mPrevWinHeight / listHeight) * barH;
	knobH = std::max(knobH, 20.0f);

	float knobY = (maxScroll > 0.0f) ? (listScroll / maxScroll) * (barH - knobH) : 0.0f;

	DrawUIBase(barX, knobY, barW, knobH, knobColor, 0.03f);

	if (activeColorPicker)
	{
		DrawColorPickerPopup(
			activeColorPicker,
			activeColorPicker->pickerX,
			activeColorPicker->pickerY
		);
	}

	for (int i = 0; i < (int)rows.size(); ++i)
	{
		UIRow& row = rows[i];
		if (!row.value || row.value->type != UIItemType::List)
			continue;

		UIItem* item = row.value;
		if (!item->listOpen)
			continue;

		float y = 10.0f + row.drawIndex * style.rowHeight - listScroll;
		float indent = row.depth * 20.0f * uiScale;

		float boxX = indent + style.valueOffsetX * uiScale;
		float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;

		float optY = boxY + style.valueHeight;

		for (int i = 0; i < item->listValues.size(); i++)
		{
			if (item->listIndex >= item->listValues.size()) continue;
			style.valueWidth = 246.0f * uiScale;
			glm::vec4 bg = (i == item->listIndex)
				? glm::vec4(0.3, 0.3, 0.3, 1)
				: glm::vec4(0.2, 0.2, 0.2, 1);

			std::string current = item->listValues[i];
			float boxW2 = 230.0f * mUIWidth / 512.0f;

			float naturalWidth2 = measureTextWidth(
				current,
				0.7f * uiScale,
				[&](float) { return 1.0f; }
			);

			float ratio2 = std::min(1.0f, boxW2 / naturalWidth2);

			auto ratioFunc2 = [&](float) {
				return ratio2;
			};

			DrawUIBase(boxX, optY, style.valueWidth, style.valueHeight, bg, 0.2f);

			DrawTextBase(
				boxX + 6,
				optY + style.labelOffsetY,
				current,
				glm::vec4(1, 1, 1, 1),
				0.7f * uiScale,
				0.4f * uiScale,
				ratioFunc2
			);

			optY += style.valueHeight;
		}
	}
	overTextbox2 = false;
	overLabel2 = false;
	for (int i = 0; i < (int)rows.size(); i++)
	{
		UIRow& row = rows[i];

		if (!row.value)
			continue;

		if (row.value->type != UIItemType::Number &&
			row.value->type != UIItemType::Float)
			continue;
		if (!row.value->selectable)
			continue;
		if (row.label)
		{
			if (Input::GetMousePosition().x >= row.label->labelHitX && Input::GetMousePosition().x <= row.label->labelHitX + row.label->labelHitW && Input::GetMousePosition().y >= row.label->labelHitY && Input::GetMousePosition().y <= row.label->labelHitY + row.label->labelHitH) { overLabel2 = true; break; }
		}
		float y = 10.0f + row.drawIndex * style.rowHeight - listScroll;
		float indent = row.depth * 20.0f * uiScale;

		float boxX = indent + style.valueOffsetX * uiScale;
		float boxY = y + (style.rowHeight - style.valueHeight) * 0.5f;
		float boxW = 170.0f * uiScale;
		float boxH = style.valueHeight;

		if (Input::GetMousePosition().x >= boxX && Input::GetMousePosition().x <= boxX + boxW &&
			Input::GetMousePosition().y >= boxY && Input::GetMousePosition().y <= boxY + boxH)
		{
			overTextbox2 = true;
			break;
		}
	}
	if (startPositionItem != nullptr && startPositionItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.ktpt.size(); i++)
		{
			auto& p = loadedKMP.ktpt[i];
			if (loadedKMP.ktpt[i].selected)
			{
				DrawUIBase(mUIWidth, 0, (125.0f * uiScale) * 2.4, style.valueHeight, glm::vec4(1.0, 1.0, 1.0, 1), 0.2f);
				DrawTextBase(
					mUIWidth,
					0,
					"Selected KTPT Index: " + std::to_string(i),
					glm::vec4(0.0, 0.0, 0.0, 1),
					0.6f * uiScale,
					0.6f * uiScale,
					[&](float) { return 1.0f; }
				);
			}
		}
	}
	if (jgptItem != nullptr && jgptItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.jgpt.size(); i++)
		{
			auto& p = loadedKMP.jgpt[i];
			if (loadedKMP.jgpt[i].selected)
			{
				DrawUIBase(mUIWidth, 0, (125.0f * uiScale) * 2.4, style.valueHeight, glm::vec4(1.0, 1.0, 1.0, 1), 0.2f);
				DrawTextBase(
					mUIWidth,
					0,
					"Selected JGPT Index: " + std::to_string(i),
					glm::vec4(0.0, 0.0, 0.0, 1),
					0.6f * uiScale,
					0.6f * uiScale,
					[&](float) { return 1.0f; }
				);
			}
		}
	}
	if (msptItem != nullptr && msptItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.mspt.size(); i++)
		{
			auto& p = loadedKMP.mspt[i];
			if (loadedKMP.mspt[i].selected)
			{
				DrawUIBase(mUIWidth, 0, (125.0f * uiScale) * 2.4, style.valueHeight, glm::vec4(1.0, 1.0, 1.0, 1), 0.2f);
				DrawTextBase(
					mUIWidth,
					0,
					"Selected MSPT Index: " + std::to_string(i),
					glm::vec4(0.0, 0.0, 0.0, 1),
					0.6f * uiScale,
					0.6f * uiScale,
					[&](float) { return 1.0f; }
				);
			}
		}
	}
	if (areaItem != nullptr && areaItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.area.size(); i++)
		{
			auto& p = loadedKMP.area[i];
			if (loadedKMP.area[i].selected)
			{
				DrawUIBase(mUIWidth, 0, (125.0f * uiScale) * 2.4, style.valueHeight, glm::vec4(1.0, 1.0, 1.0, 1), 0.2f);
				DrawTextBase(
					mUIWidth,
					0,
					"Selected AREA Index: " + std::to_string(i),
					glm::vec4(0.0, 0.0, 0.0, 1),
					0.6f * uiScale,
					0.6f * uiScale,
					[&](float) { return 1.0f; }
				);
			}
		}
	}
	if (cameItem != nullptr && cameItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.came.size(); i++)
		{
			auto& p = loadedKMP.came[i];
			if (loadedKMP.came[i].selected)
			{
				DrawUIBase(mUIWidth, 0, (125.0f * uiScale) * 2.4, style.valueHeight, glm::vec4(1.0, 1.0, 1.0, 1), 0.2f);
				DrawTextBase(
					mUIWidth,
					0,
					"Selected CAME Index: " + std::to_string(i),
					glm::vec4(0.0, 0.0, 0.0, 1),
					0.6f * uiScale,
					0.6f * uiScale,
					[&](float) { return 1.0f; }
				);
			}
		}
	}
	if (cnptItem != nullptr && cnptItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.cnpt.size(); i++)
		{
			auto& p = loadedKMP.cnpt[i];
			if (loadedKMP.cnpt[i].selected)
			{
				DrawUIBase(mUIWidth, 0, (125.0f * uiScale) * 2.4, style.valueHeight, glm::vec4(1.0, 1.0, 1.0, 1), 0.2f);
				DrawTextBase(
					mUIWidth,
					0,
					"Selected CNPT Index: " + std::to_string(i),
					glm::vec4(0.0, 0.0, 0.0, 1),
					0.6f * uiScale,
					0.6f * uiScale,
					[&](float) { return 1.0f; }
				);
			}
		}
	}
	if (enptItem != nullptr && enptItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.enpt.size(); i++)
		{
			auto& p = loadedKMP.enpt[i];
			if (loadedKMP.enpt[i].selected)
			{
				DrawUIBase(mUIWidth, 0, (125.0f * uiScale) * 2.4, style.valueHeight, glm::vec4(1.0, 1.0, 1.0, 1), 0.2f);
				DrawTextBase(
					mUIWidth,
					0,
					"Selected ENPT Index: " + std::to_string(i),
					glm::vec4(0.0, 0.0, 0.0, 1),
					0.6f * uiScale,
					0.6f * uiScale,
					[&](float) { return 1.0f; }
				);
			}
		}
	}
	if (itptItem != nullptr && itptItem->expanded)
	{
		for (size_t i = 0; i < loadedKMP.itpt.size(); i++)
		{
			auto& p = loadedKMP.itpt[i];
			if (loadedKMP.itpt[i].selected)
			{
				DrawUIBase(mUIWidth, 0, (125.0f * uiScale) * 2.4, style.valueHeight, glm::vec4(1.0, 1.0, 1.0, 1), 0.2f);
				DrawTextBase(
					mUIWidth,
					0,
					"Selected ITPT Index: " + std::to_string(i),
					glm::vec4(0.0, 0.0, 0.0, 1),
					0.6f * uiScale,
					0.6f * uiScale,
					[&](float) { return 1.0f; }
				);
			}
		}
	}
	for (auto& row : rows)
	{
		if (row.value && row.value->type == UIItemType::List)
		{
			if (row.value->listOpen)
			{
				glfwSetCursor(gMain->mWindow, arrowCursor);
				return;
			}
		}
	}
	if (overLabel2)glfwSetCursor(gMain->mWindow, resizeCursor);
	else if (overTextbox2)glfwSetCursor(gMain->mWindow, ibeamCursor);
	else 
		glfwSetCursor(gMain->mWindow, arrowCursor);
	
}
void RenderContext::undo()
{
	if (undoStack.empty()) return;

	ProjectState st = undoStack.back();
	undoStack.pop_back();

	redoStack.push_back(st);
	if (redoStack.size() > 127)
		redoStack.erase(redoStack.begin());

	UIItem* item = findItemById(st.itemId);
	if (item)
		applyState(item, st.before);

	loadedKMP = st.beforeKMP;
	ctxKTPT.data = &loadedKMP.ktpt;
	ctxJGPT.data = &loadedKMP.jgpt;
	ctxITPT.data = &loadedKMP.itpt;
	ctxENPT.data = &loadedKMP.enpt;
	ctxAREA.data = &loadedKMP.area;
	ctxCAME.data = &loadedKMP.came;
	ctxMSPT.data = &loadedKMP.mspt;
	ctxGOBJ.data = &loadedKMP.gobj;
	ctxPOTI.data = &loadedKMP.poti;
	ctxCKPT.data = &loadedKMP.ckpt;

	for (auto& p : *ctxKTPT.data) p.selected = false;
	for (auto& p : *ctxJGPT.data) p.selected = false;
	for (auto& p : *ctxMSPT.data) p.selected = false;
	for (auto& p : *ctxITPT.data) p.selected = false;
	for (auto& p : *ctxENPT.data) p.selected = false;
	for (auto& p : *ctxAREA.data) p.selected = false;
	for (auto& p : *ctxCAME.data) p.selected = false;
	for (auto& p : *ctxCKPT.data) p.selected = false;
	for (auto& p : *ctxGOBJ.data) p.selected = false;
	for (auto& p : *ctxPOTI.data) p.selected = false;
	dragIndices.clear();
	primaryIndex = -1;
	anchorIndex = -1;
	hoverIndex = -1;
	if (loadedKMP == initKMP)
	{
		gMain->dirty = false;
		glfwSetWindowTitle(
			gMain->mWindow,
			("KMParse: Editing " + std::filesystem::path(mPath).filename().string()).c_str()
		);
	}
	else
	{
		gMain->dirty = true;
	}
	SetAreaItemInvisible();
}

void RenderContext::redo()
{
	if (redoStack.empty()) return;

	ProjectState st = redoStack.back();
	redoStack.pop_back();

	undoStack.push_back(st);

	UIItem* item = findItemById(st.itemId);
	if (item)
		applyState(item, st.after);

	loadedKMP = st.afterKMP;

	ctxKTPT.data = &loadedKMP.ktpt;
	ctxJGPT.data = &loadedKMP.jgpt;
	ctxITPT.data = &loadedKMP.itpt;
	ctxENPT.data = &loadedKMP.enpt;
	ctxAREA.data = &loadedKMP.area;
	ctxCAME.data = &loadedKMP.came;
	ctxMSPT.data = &loadedKMP.mspt;
	ctxGOBJ.data = &loadedKMP.gobj;
	ctxPOTI.data = &loadedKMP.poti;
	ctxCKPT.data = &loadedKMP.ckpt;

	for (auto& p : *ctxKTPT.data) p.selected = false;
	for (auto& p : *ctxJGPT.data) p.selected = false;
	for (auto& p : *ctxMSPT.data) p.selected = false;
	for (auto& p : *ctxITPT.data) p.selected = false;
	for (auto& p : *ctxENPT.data) p.selected = false;
	for (auto& p : *ctxAREA.data) p.selected = false;
	for (auto& p : *ctxCAME.data) p.selected = false;
	for (auto& p : *ctxCKPT.data) p.selected = false;
	for (auto& p : *ctxGOBJ.data) p.selected = false;
	for (auto& p : *ctxPOTI.data) p.selected = false;
	dragIndices.clear();
	primaryIndex = -1;
	anchorIndex = -1;
	hoverIndex = -1;
	if (loadedKMP == initKMP)
	{
		gMain->dirty = false;
		glfwSetWindowTitle(
			gMain->mWindow,
			("KMParse: Editing " + std::filesystem::path(mPath).filename().string()).c_str()
		);
	}
	else
	{
		gMain->dirty = true;
	}
	SetAreaItemInvisible();
}

void RenderContext::loadCourseBrres()
{
	if (mPath.empty()) return;
	isViewerOpen = !isViewerOpen;

	archive = Archive::U8::Create();
	bStream::CFileStream modelArchive(mPath, bStream::Endianess::Big, bStream::OpenMode::In);
	if (!archive->Load(&modelArchive))
	{
		std::filesystem::path parent = std::filesystem::path(mPath).parent_path();
		for (int i = 0; i < loadedKMP.gobj.size(); i++)
		{
			auto s = loadedKMP.gobj[i];
			switch (s.id)
			{
			case 2:
			{
				std::filesystem::path p = parent / "Psea.brres";
				if (std::filesystem::exists(p))
				{
					bStream::CFileStream stream(p.string(), bStream::Endianess::Big, bStream::OpenMode::In);
					brresRenderer2.Loader(&stream, 0,s.pos,s.rot,s.scale);
					brresRenderer2.Loader(&stream, 1, s.pos, s.rot, s.scale);
					brresRenderer2.Loader(&stream, 2, s.pos, s.rot, s.scale);
					brresRenderer2.Loader(&stream, 3, s.pos, s.rot, s.scale);
					brresRenderer2.Loader(&stream, 4, s.pos, s.rot, s.scale);
				}
				break;
			}
			case 101:
			{
				std::filesystem::path p = parent / "itembox.brres";
				if (std::filesystem::exists(p))
				{
					bStream::CFileStream stream(p.string(), bStream::Endianess::Big, bStream::OpenMode::In);
					brresRenderer2.Loader(&stream, 0, s.pos, s.rot, s.scale);
				}
				break;
			}
		}
	}
}
	else
	{
		for (int i = 0; i < loadedKMP.gobj.size(); i++)
		{
			auto s = loadedKMP.gobj[i];
			switch (s.id)
			{
			case 2:
			{
				auto modelFile = archive->Get<Archive::File>("Psea.brres");
				if (!modelFile) {
					modelFile = archive->Get<Archive::File>("./Psea.brres");
				}
				if (modelFile)
				{
					bStream::CMemoryStream stream(modelFile->GetData(), modelFile->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
					brresRenderer2.Loader(&stream, 0, s.pos, s.rot, s.scale);
					brresRenderer2.Loader(&stream, 1, s.pos, s.rot, s.scale);
					brresRenderer2.Loader(&stream, 2, s.pos, s.rot, s.scale);
					brresRenderer2.Loader(&stream, 3, s.pos, s.rot, s.scale);
					brresRenderer2.Loader(&stream, 4, s.pos, s.rot, s.scale);
				}
			}
			case 101:
			{
				auto modelFile = archive->Get<Archive::File>("itembox.brres");
				if (!modelFile) {
					modelFile = archive->Get<Archive::File>("./itembox.brres");
				}
				if (modelFile)
				{
					bStream::CMemoryStream stream(modelFile->GetData(), modelFile->GetSize(), bStream::Endianess::Big, bStream::OpenMode::In);
					brresRenderer2.Loader(&stream, 0, s.pos, s.rot, s.scale);
				}
			}
			}
		}
	}
	brresRenderer2.Renderer(mPrevWinWidth, mPrevWinHeight, view, projection, bfgRes, gMain->mShowFogs);
}

void RenderContext::errorCheck()
{

}

