#pragma once
#include <glm/glm.hpp>
#include <deque>
#include "Camera.h"
#include "nw4r/brres.h"
#include "egg/blight.h"
#include "egg/posteffect.h"
#include "egg/bfg.h"
#include "cache.h"
#include "kmp/kmp.h"
#include "kmp/kcl.h"
#include "gctoolsplusplus/Archive.hpp"
#include "nw4r/brlyt.h"
#include "nlohmann/json.hpp"
#include "KMParseCode/ModelBuilder.h"
#include "KMParseCode/Input.h"
class RenderContext {
	float elapsedTime = 0.0f;
	int mPrevWinWidth;
	int mPrevWinHeight;
	int mUIWidth;
	glm::mat4 uiProj;
	GLuint uiVAO = 0;
	GLuint uiVBO = 0;
	lyt::ResFont* mUIFont = nullptr;
	const char* uiVS = R"(
#version 330 core
layout(location = 0) in vec3 a_Pos;
uniform mat4 u_proj;

void main()
{
    gl_Position = u_proj * vec4(a_Pos, 1.0);
}
)";
	GLuint uiProgram = CreateShaderProgram(uiVS, R"(
#version 330 core
uniform vec4 u_color;
out vec4 FragColor;
void main()
{
    FragColor = u_color;
}
)");

public:
	uint32_t mFbo, mRbo;
	GLFWwindow* child = nullptr;
	struct UIItem;
	enum class UIItemType {
		Label,
		List,
		Number,
		Float,
		Color,
		Checkbox,
		Button,
		Group
	};
	struct UIItemState
	{
		UIItemType type;

		int numberValue;
		float floatValue;
		int listIndex;

		glm::vec4 colorValue;
		glm::vec3 hsv;
		float alpha;

		std::string colorCode;

		bool checked;
		bool selectable;
		bool expanded;
		std::string editBuffer;
		int cursorPos;
		int selectStart;
		int selectEnd;
	};

	struct ProjectState {
		int itemId;
		UIItemState before;
		UIItemState after;
		kmp::KMP beforeKMP;
		kmp::KMP afterKMP;
	};
	std::vector<ProjectState> undoStack;
	std::vector<ProjectState> redoStack;
	ProjectState currentState;
	void applyState(UIItem* item, const UIItemState& s)
	{
		item->type = s.type;

		item->numberValue = s.numberValue;
		item->floatValue = s.floatValue;
		item->listIndex = s.listIndex;

		item->colorValue = s.colorValue;
		item->hsv = s.hsv;
		item->alpha = s.alpha;
		item->colorCode = s.colorCode;

		item->checked = s.checked;

		item->editBuffer = s.editBuffer;

		item->cursorPos = s.cursorPos;
		item->selectStart = s.selectStart;
		item->selectEnd = s.selectEnd;

		item->selectable = s.selectable;

		item->expanded = s.expanded;
		item->editing = false;
	}

	void pushHistory(int itemId,
		const kmp::KMP& before,
		const kmp::KMP& after,
		const UIItemState& beforeUI,
		const UIItemState& afterUI)
	{
		ProjectState st;
		st.itemId = itemId;
		st.beforeKMP = before;
		st.afterKMP = after;
		st.before = beforeUI;
		st.after = afterUI;

		undoStack.push_back(st);

		if (undoStack.size() > 127)
			undoStack.erase(undoStack.begin());

		redoStack.clear();
	}




	struct UIStyle {
		float rowHeight = 40.0f;

		float labelOffsetX = 12.0f;
		float labelOffsetY = 12.0f;

		float valueOffsetX = 300.0f;
		float valueWidth = 120.0f;
		float valueHeight = 24.0f;

		glm::vec4 rowBgColor1 = glm::vec4(0.11f, 0.11f, 0.11f, 1.0f);
		glm::vec4 rowBgColor2 = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);

		glm::vec4 textColor = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
		glm::vec4 valueBgColor = glm::vec4(0.90f, 0.90f, 0.90f, 1.0f);

		float triangleSize = 12.0f;

		float indentWidth = 20.0f;
	};
	struct UIItem {
		int id;
		static int nextId;

		UIItem(UIItemType t, const std::string& lbl)
			: type(t), label(lbl)
		{
			id = nextId++;
		}

		UIItemType type;
		std::string label;
		bool invisible = false;

		int pendingListIndex = -1;

		float minCompressRatio = 0.75f;
		float maxCompressRatio = 1.0f;

		int maxDigits = -1;

		bool selectable = true;

		float compressStartX = -1.0f;

		int numberDefault = 1;
		int numberMin = 1;
		int numberMax = 12;

		glm::vec4 colorValue;
		bool pickingColor = false;

		float floatDefault = 0.0f;
		float floatMin = 0.0f;
		float floatMax = 999.0f;

		int numberValue;
		float floatValue;
		bool checked;

		std::vector<std::string> listValues;
		int listIndex;

		float indent = 0.0f;

		bool expanded;

		bool hovered = false;

		bool pressed = false;

		std::function<void()> onClick;
		std::function<void()> onCommit;

		bool editing = false;
		std::string editBuffer;

		int cursorPos = 0;
		int selectStart = -1;
		int selectEnd = -1;

		glm::vec3 hsv = glm::vec3(0.0f);
		float alpha = 1.0f;
		bool dragHue = false;
		bool dragSat = false;
		bool dragVal = false;
		bool dragAlpha = false;

		bool draggingValue = false;
		float dragStartY = 0.0f;
		float dragStartValue = 0.0f;

		float pickerX = 0.0f;
		float pickerY = 0.0f;
		float pickerW = 0.0f;
		float pickerH = 0.0f;

		float labelHitX = 0.0f;
		float labelHitY = 0.0f;
		float labelHitW = 0.0f;
		float labelHitH = 0.0f;

		std::string colorCode = "#FFFFFFFF";
		bool editingColorCode = false;

		bool listOpen = false;

		UIItemState colorBeforeState; 
		bool hasColorBeforeState = false;
		UIItemState toState() const
		{
			UIItemState s{};
			s.type = type;

			s.numberValue = numberValue;
			s.floatValue = floatValue;
			s.listIndex = listIndex;

			s.editBuffer = editBuffer;
			s.cursorPos = cursorPos;
			s.selectStart = selectStart;
			s.selectEnd = selectEnd;
			s.selectable = selectable;
			s.colorValue = colorValue;
			s.hsv = hsv;
			s.alpha = alpha;
			s.colorCode = colorCode;

			s.checked = checked;

			return s;
		}
	};
	struct FlatItem {
		UIItem* item;
		int depth;
	};
	struct UIRow {
		UIItem* label;
		UIItem* value;
		int depth;
		int drawIndex = 0;
	};
	struct UIGroup {
		UIItem* titleItem;
		bool expanded = true;
		int depth = 0;

		std::vector<UIRow> rows;
	};
	struct UIPanel
	{
		std::string title;
		bool open = true;

		std::vector<UIGroup> groups;
		std::vector<UIRow> rows;
	};
	UIStyle style;
	std::vector<UIRow> rows;
	std::deque<std::unique_ptr<UIItem>> ownedItems;
	bool editingDrag = false;
	UIItem* editingItem = nullptr;
	void flatten(const UIPanel& panel, std::vector<UIRow>& out)
	{
		for (auto& g : mainPanel.groups)
		{
			if (g.titleItem)
				g.titleItem->expanded = g.expanded;
		}

		for (auto& row : panel.rows)
			out.push_back(row);

		for (const UIGroup& group : panel.groups)
		{
			UIRow titleRow;
			titleRow.label = group.titleItem;
			titleRow.value = nullptr;
			titleRow.depth = group.depth;
			out.push_back(titleRow);

			if (group.expanded)
			{
				for (auto& row : group.rows)
				{
					UIRow r = row;
					r.depth = group.depth + 1;
					out.push_back(r);
				}
			}
		}
	}
	UIItem* findItemById(int id)
	{
		for (auto& r : rows)
			if (r.value && r.value->id == id)
				return r.value;
		return nullptr;
	}

	struct PointUI {
		UIItem* posX = nullptr;
		UIItem* posY = nullptr;
		UIItem* posZ = nullptr;

		UIItem* rotX = nullptr;
		UIItem* rotY = nullptr;
		UIItem* rotZ = nullptr;

		UIItem* scaleX = nullptr;
		UIItem* scaleY = nullptr;
		UIItem* scaleZ = nullptr;

		// AREA / CAME 
		UIItem* type = nullptr;
		UIItem* route = nullptr;

		// AREA
		UIItem* shape = nullptr;
		UIItem* priority = nullptr;
		UIItem* cameraIndex = nullptr;
		UIItem* routeIndex = nullptr;
		UIItem* enemyIndex = nullptr;
		UIItem* setting1 = nullptr;
		UIItem* setting2 = nullptr;

		// CAME
		UIItem* nextCam = nullptr;
		UIItem* shake = nullptr;
		UIItem* vCam = nullptr;
		UIItem* vZoom = nullptr;
		UIItem* vView = nullptr;

		UIItem* zoomStart = nullptr;
		UIItem* zoomEnd = nullptr;

		UIItem* viewStartX = nullptr;
		UIItem* viewStartY = nullptr;
		UIItem* viewStartZ = nullptr;

		UIItem* viewEndX = nullptr;
		UIItem* viewEndY = nullptr;
		UIItem* viewEndZ = nullptr;

		UIItem* time = nullptr;

		UIItem* id = nullptr;
		UIItem* soundTrig = nullptr;

		UIItem* deviation = nullptr;

		UIItem* setting3 = nullptr;
		UIItem* setting4 = nullptr;
		UIItem* setting5 = nullptr;
		UIItem* setting6 = nullptr;
		UIItem* setting7 = nullptr;
		UIItem* setting8 = nullptr;
		UIItem* setting9 = nullptr;
		UIItem* setting10 = nullptr;
		UIItem* setting11 = nullptr;
		UIItem* setting12 = nullptr;

		UIItem* coobStartCPIndex = nullptr;
		UIItem* coobStartCPIndexName = nullptr;
		UIItem* coobEndCPIndex = nullptr;

		//BlueLeopard's Extended KMP Area
		UIItem* railRidingRotation = nullptr;

		UIItem* cobjUnloadGroup = nullptr;
		UIItem* cobjUnloadKCLFlag = nullptr;
		UIItem* cobjUnloadFrame1 = nullptr;
		UIItem* cobjUnloadSetting = nullptr;
		UIItem* cobjUnloadSetting2 = nullptr;
		UIItem* cobjUnloadSetting3 = nullptr;
		UIItem* cobjUnloadFrame2 = nullptr;

		UIItem* airRingBoostTime = nullptr;

		UIItem* teleportTime = nullptr;
		UIItem* teleportAngle = nullptr;

		UIItem*  gravityType = nullptr;
		UIItem* antiGravity = nullptr;
		UIItem* gravityScale = nullptr;

		UIItem* windAngle = nullptr;
		UIItem* windPower = nullptr;

		UIItem* posX1 = nullptr;
		UIItem* posX2 = nullptr;
		UIItem* posZ1 = nullptr;
		UIItem* posZ2 = nullptr;
		UIItem* typeCK = nullptr;
		UIItem* respawn = nullptr;
		UIItem* editingY = nullptr;

		//POTI
		UIItem* button1 = nullptr;
		UIItem* button2 = nullptr;
	};

	class UIPanelBuilder {
	public:
		RenderContext& ctx;
		UIPanel& panel;

		UIPanelBuilder(RenderContext& c, UIPanel& p)
			: ctx(c), panel(p) {
		}

		UIItem* makeItem(UIItemType type, const std::string& label)
		{
			ctx.ownedItems.emplace_back(
				std::make_unique<UIItem>(type, label)
			);
			return ctx.ownedItems.back().get();
		}

		UIGroup& addGroup(const std::string& title, bool expanded = true)
		{
			panel.groups.emplace_back();
			UIGroup& g = panel.groups.back();

			g.titleItem = makeItem(UIItemType::Group, title);
			g.titleItem->expanded = expanded;
			g.expanded = expanded;

			g.depth = 0;

			return g;
		}


		UIRow& addCheckbox(UIGroup& g, const std::string& text, bool checked, bool editable, bool inv, std::function<void(bool)> onChange)
		{
			UIItem* label = makeItem(UIItemType::Label, text);
			UIItem* value = makeItem(UIItemType::Checkbox, "");
			value->selectable = editable;
			value->checked = checked;
			int id = value->id;
			value->onClick = [rc = &ctx, value, onChange, id]() {
				if (!rc) return;

				UIItemState beforeUI = value->toState();
				kmp::KMP before = rc->loadedKMP;

				value->checked = !value->checked;

				onChange(value->checked);

				kmp::KMP after = rc->loadedKMP;
				UIItemState afterUI = value->toState();

				rc->pushHistory(id, before, after, beforeUI, afterUI);
			};
			value->invisible = inv;
			UIRow row;
			row.label = label;
			row.value = value;
			row.depth = g.depth + 1;

			g.rows.push_back(row);
			return g.rows.back();
		}

		// --- Number -----------------------------------------------------

		UIRow& addNumber(UIGroup& g, const std::string& text, int initialValue,
			int minValue, int maxValue, int defaultValue, int maxDigits, bool editable, bool inv,
			std::function<void(int)> onChange)
		{
			UIItem* label = makeItem(UIItemType::Label, text);
			UIItem* val = makeItem(UIItemType::Number, "");

			val->numberValue = initialValue;
			val->editBuffer = std::to_string(initialValue);
			val->cursorPos = val->editBuffer.size();

			val->maxDigits = maxDigits;
			val->numberMin = minValue;
			val->numberMax = maxValue;
			val->numberDefault = defaultValue;
			val->selectable = editable;
			float indent = (g.depth + 1) * ctx.style.indentWidth;
			label->indent = indent;
			val->indent = indent;
			val->invisible = inv;
			val->onClick = [val]() {
				val->editing = true;
			};

			int id = val->id;

			val->onCommit = [rc = &ctx, val, onChange, id]() {
				if (!rc) return;

				kmp::KMP before = rc->loadedKMP;
				UIItemState beforeUI = val->toState();
				int v;
				try {
					v = std::stoi(val->editBuffer);
				}
				catch (...) {
					v = val->numberDefault;
				}
				v = std::clamp(v, val->numberMin, val->numberMax);
				val->numberValue = v;
				val->editBuffer = std::to_string(v);
								
				onChange(v);

				kmp::KMP after = rc->loadedKMP;
				UIItemState afterUI = val->toState();

				rc->pushHistory(id, before, after, beforeUI, afterUI);
			};


			UIRow row;
			row.label = label;
			row.value = val;
			row.depth = g.depth + 1;

			g.rows.push_back(row);
			return g.rows.back();
		}

		glm::vec3 RGBtoHSV(const glm::vec4& rgb)
		{
			float r = rgb.r;
			float g = rgb.g;
			float b = rgb.b;

			float maxc = std::max<float>(r, std::max<float>(g, b));
			float minc = std::min<float>(r, std::min<float>(g, b));
			float delta = maxc - minc;

			float h = 0.0f;
			float s = 0.0f;
			float v = maxc;

			if (delta > 0.00001f)
			{
				s = delta / maxc;

				if (maxc == r)
					h = (g - b) / delta;
				else if (maxc == g)
					h = 2.0f + (b - r) / delta;
				else
					h = 4.0f + (r - g) / delta;

				h /= 6.0f;
				if (h < 0.0f)
					h += 1.0f;
			}
			else
			{
				h = 0.0f;
				s = 0.0f;
			}

			return glm::vec3(h, s, v);
		}

		UIRow& addColor(
			UIGroup& g,
			const std::string& text,
			const glm::vec4& defaultColor,
			std::function<void(const glm::vec4&)> onChange)
		{
			UIItem* label = makeItem(UIItemType::Label, text);
			UIItem* val = makeItem(UIItemType::Color, "");

			val->colorValue = defaultColor;
			val->hsv = RGBtoHSV(defaultColor);
			val->alpha = defaultColor.a;
			val->colorCode = ctx.FormatColorCode(defaultColor);
			val->pickingColor = false;

			float indent = (g.depth + 1) * ctx.style.indentWidth;
			label->indent = indent;
			val->indent = indent;

			val->onClick = [rc = &ctx, val]() {
				if (!val->pickingColor) {
					val->colorBeforeState = val->toState();
					val->hasColorBeforeState = true;
				}

				val->pickingColor = !val->pickingColor;
			};


			int id = val->id;
			val->onCommit = [rc = &ctx, val, onChange, id]() {
				if (!rc) return;

				kmp::KMP before = rc->loadedKMP;
				UIItemState beforeUI = val->hasColorBeforeState ? val->colorBeforeState : val->toState();

				glm::vec3 rgb = rc->HSVtoRGB(val->hsv);
				val->colorValue = glm::vec4(rgb, val->alpha);
				val->colorCode = rc->FormatColorCode(val->colorValue);

				onChange(val->colorValue);

				kmp::KMP after = rc->loadedKMP;
				UIItemState afterUI = val->toState();

				rc->pushHistory(id, before, after, beforeUI, afterUI);
			};


			UIRow row;
			row.label = label;
			row.value = val;
			row.depth = g.depth + 1;

			g.rows.push_back(row);
			return g.rows.back();
		}

		// --- Float ------------------------------------------------------

		UIRow& addFloat(UIGroup& g, const std::string& text, float initialValue,
			float minValue, float maxValue, float defaultValue, int maxDigits,bool editable, bool inv,
			std::function<void(float)> onChange)

		{
			UIItem* label = makeItem(UIItemType::Label, text);
			UIItem* val = makeItem(UIItemType::Float, "");

			val->floatValue = initialValue;

			val->selectable = editable;
			val->editBuffer = std::format("{:.6f}", initialValue);
			val->cursorPos = val->editBuffer.size();
			val->selectStart = -1;
			val->selectEnd = -1;

			float indent = (g.depth + 1) * ctx.style.indentWidth;
			label->indent = indent;
			val->indent = indent;

			val->maxDigits = maxDigits;
			val->floatMin = minValue;
			val->floatMax = maxValue;
			val->floatDefault = defaultValue;

			val->onClick = [val]() {
				val->editing = true;
			};
			val->invisible = inv;
			int id = val->id;
			val->onCommit = [rc = &ctx, valPtr = val, onChange, id]() {
				if (!rc) return;

				kmp::KMP before = rc->loadedKMP;
				UIItemState beforeUI = valPtr->toState();

				float v;
				try {
					v = std::stof(valPtr->editBuffer);
				}
				catch (...) {
					v = valPtr->floatDefault;
				}

				v = std::clamp(v, valPtr->floatMin, valPtr->floatMax);
				valPtr->floatValue = v;
				valPtr->editBuffer = std::format("{:.6f}", v);

				onChange(v);

				kmp::KMP after = rc->loadedKMP;
				UIItemState afterUI = valPtr->toState();

				rc->pushHistory(id, before, after, beforeUI, afterUI);
			};

			UIRow row;
			row.label = label;
			row.value = val;
			row.depth = g.depth + 1;

			g.rows.push_back(row);
			return g.rows.back();
		}



		// --- List -------------------------------------------------------

		UIRow& addList(UIGroup& g, const std::string& text,
			const std::vector<std::string>& values, int index, bool editable, bool inv,
			std::function<void(int)> onChange)
		{
			UIItem* label = makeItem(UIItemType::Label, text);
			UIItem* val = makeItem(UIItemType::List, "");

			val->listValues = values;
			val->listIndex = index;
			val->selectable = editable;
			val->onClick = [val]() {
				val->listOpen = !val->listOpen;
			};
			val->invisible = inv;
			int id = val->id;
			val->onCommit = [rc = &ctx, val, onChange, id]() {
				if (!rc) return;

				kmp::KMP before = rc->loadedKMP;
				UIItemState beforeUI = val->toState();

				val->listIndex = val->pendingListIndex;

				onChange(val->listIndex);

				kmp::KMP after = rc->loadedKMP;
				UIItemState afterUI = val->toState();

				rc->pushHistory(id, before, after, beforeUI, afterUI);
			};


			UIRow row;
			row.label = label;
			row.value = val;
			row.depth = g.depth + 1;

			g.rows.push_back(row);
			return g.rows.back();
		}

		UIRow& addButton(
			UIGroup& g,
			const std::string& text,
			std::function<void()> onClick
		)
		{
			UIItem* label = makeItem(UIItemType::Label, text);
			UIItem* val = makeItem(UIItemType::Button, "");

			val->selectable = true;
			val->onClick = [val, onClick]() {
				if (onClick) onClick();
			};

			UIRow row;
			row.label = label;
			row.value = val;
			row.depth = g.depth + 1;

			g.rows.push_back(row);
			return g.rows.back();
		}
	};

	struct EditablePoint {
		glm::vec3* pos = nullptr;
		glm::vec3* rot = nullptr;
		bool* selected = nullptr;

		uint8_t* type = nullptr;
		uint8_t* shape = nullptr;
		glm::vec3* scale = nullptr;
		uint8_t* priority = nullptr;

		uint16_t* setting1 = nullptr;
		uint16_t* setting2 = nullptr;
		uint8_t* cameraIndex = nullptr;
		uint8_t* routeIndex = nullptr;
		uint8_t* enemyIndex = nullptr;

		uint8_t* nextCam = nullptr;
		uint8_t* shake = nullptr;

		uint16_t* vCam = nullptr;
		uint16_t* vZoom = nullptr;
		uint16_t* vView = nullptr;

		uint8_t* start = nullptr;
		uint8_t* movie = nullptr;

		float* zoomStart = nullptr;
		float* zoomEnd = nullptr;

		glm::vec3* viewStart = nullptr;
		glm::vec3* viewEnd = nullptr;

		float* time = nullptr;

		uint16_t* id = nullptr;
		uint16_t* sound = nullptr;

		float* deviation = nullptr;
		uint16_t* s1 = nullptr;
		uint16_t* s2 = nullptr;
		uint8_t* s3 = nullptr;
		uint16_t* pad = nullptr;

		uint16_t* objSetting1 = nullptr;
		uint16_t* objSetting2 = nullptr;
		uint16_t* objSetting3 = nullptr;
		uint16_t* objSetting4 = nullptr;
		uint16_t* objSetting5 = nullptr;
		uint16_t* objSetting6 = nullptr;
		uint16_t* objSetting7 = nullptr;
		uint16_t* objSetting8 = nullptr;

		float* x1 = nullptr;
		float* x2 = nullptr;
		float* z1 = nullptr;
		float* z2 = nullptr;

		uint8_t* respawn = nullptr;

		uint16_t* num = nullptr;

		uint32_t* presence = nullptr;
	};
	template <typename T>
	struct EditableTraitsBase {
		static glm::vec3* pos(T&) { return nullptr; }
		static glm::vec3* rot(T&) { return nullptr; }
		static bool* selected(T&) { return nullptr; }

		static float* x1(T&) { return nullptr; }
		static float* x2(T&) { return nullptr; }
		static float* z1(T&) { return nullptr; }
		static float* z2(T&) { return nullptr; }

		static uint8_t* respawn(T&) { return nullptr; }

		static uint8_t* type(T&) { return nullptr; }
		static uint8_t* shape(T&) { return nullptr; }
		static glm::vec3* scale(T&) { return nullptr; }
		static uint8_t* priority(T&) { return nullptr; }

		static uint16_t* setting1(T&) { return nullptr; }
		static uint16_t* setting2(T&) { return nullptr; }
		static uint8_t* cameraIndex(T&) { return nullptr; }
		static uint8_t* routeIndex(T&) { return nullptr; }
		static uint8_t* enemyIndex(T&) { return nullptr; }
		static uint16_t* pad(T&) { return nullptr; }

		static uint8_t* nextCam(T&) { return nullptr; }
		static uint8_t* shake(T&) { return nullptr; }

		static uint16_t* vCam(T&) { return nullptr; }
		static uint16_t* vZoom(T&) { return nullptr; }
		static uint16_t* vView(T&) { return nullptr; }

		static uint8_t* start(T&) { return nullptr; }
		static uint8_t* movie(T&) { return nullptr; }

		static float* zoomStart(T&) { return nullptr; }
		static float* zoomEnd(T&) { return nullptr; }

		static glm::vec3* viewStart(T&) { return nullptr; }
		static glm::vec3* viewEnd(T&) { return nullptr; }

		static float* time(T&) { return nullptr; }

		static uint16_t* id(T&) { return nullptr; }
		static uint16_t* soundTrig(T&) { return nullptr; }

		static float* deviation(T&) { return nullptr; }

		static uint16_t* num(T&) { return nullptr; }
		static uint16_t* s1(T&) { return nullptr; }
		static uint16_t* s2(T&) { return nullptr; }
		static uint8_t* s3(T&) { return nullptr; }

		static uint16_t* objSettings1(T&) { return nullptr; }
		static uint16_t* objSettings2(T&) { return nullptr; }
		static uint16_t* objSettings3(T&) { return nullptr; }
		static uint16_t* objSettings4(T&) { return nullptr; }
		static uint16_t* objSettings5(T&) { return nullptr; }
		static uint16_t* objSettings6(T&) { return nullptr; }
		static uint16_t* objSettings7(T&) { return nullptr; }
		static uint16_t* objSettings8(T&) { return nullptr; }

		static uint32_t* presence(T&) { return nullptr; }
	};

	template <typename T> 
	struct EditableTraits : EditableTraitsBase<T> {};
	template <>
	struct EditableTraits<kmp::KTPT> : EditableTraitsBase<kmp::KTPT> {
		static glm::vec3* pos(kmp::KTPT& p) { return &p.pos; }
		static glm::vec3* rot(kmp::KTPT& p) { return &p.rot; }
		static bool* selected(kmp::KTPT& p) { return &p.selected; }
	};

	template <>
	struct EditableTraits<kmp::ENPT> : EditableTraitsBase<kmp::ENPT> {
		static glm::vec3* pos(kmp::ENPT& p) { return &p.pos; }
		static float* deviation(kmp::ENPT& p) { return &p.deviation; }
		static uint16_t* s1(kmp::ENPT& p) { return &p.s1; }
		static uint16_t* s2(kmp::ENPT& p) { return (uint16_t*)&p.s2; }
		static uint8_t* s3(kmp::ENPT& p) { return &p.s3; }
		static bool* selected(kmp::ENPT& p) { return &p.selected; }
	};

	template <>
	struct EditableTraits<kmp::ITPT> : EditableTraitsBase<kmp::ITPT> {
		static glm::vec3* pos(kmp::ITPT& p) { return &p.pos; }
		static float* deviation(kmp::ITPT& p) { return &p.deviation; }
		static uint16_t* s1(kmp::ITPT& p) { return &p.s1; }
		static uint16_t* s2(kmp::ITPT& p) { return &p.s2; }
		static bool* selected(kmp::ITPT& p) { return &p.selected; }
	};

	template <>
	struct EditableTraits<kmp::GOBJ> : EditableTraitsBase<kmp::GOBJ> {
		static uint16_t* id(kmp::GOBJ& p) { return &p.id; }
		static glm::vec3* pos(kmp::GOBJ& p) { return &p.pos; }
		static glm::vec3* rot(kmp::GOBJ& p) { return &p.rot; }
		static glm::vec3* scale(kmp::GOBJ& p) { return &p.scale; }
		static uint16_t* objSettings1(kmp::GOBJ& p) { return &p.args[0]; }
		static uint16_t* objSettings2(kmp::GOBJ& p) { return &p.args[1]; }
		static uint16_t* objSettings3(kmp::GOBJ& p) { return &p.args[2]; }
		static uint16_t* objSettings4(kmp::GOBJ& p) { return &p.args[3]; }
		static uint16_t* objSettings5(kmp::GOBJ& p) { return &p.args[4]; }
		static uint16_t* objSettings6(kmp::GOBJ& p) { return &p.args[5]; }
		static uint16_t* objSettings7(kmp::GOBJ& p) { return &p.args[6]; }
		static uint16_t* objSettings8(kmp::GOBJ& p) { return &p.args[7]; }

		static uint32_t* presence(kmp::GOBJ& p) { return &p.presence; }
		static bool* selected(kmp::GOBJ& p) { return &p.selected; }
	};

	template <>
	struct EditableTraits<kmp::AREA> : EditableTraitsBase<kmp::AREA> {
		static glm::vec3* pos(kmp::AREA& p) { return &p.pos; }
		static glm::vec3* rot(kmp::AREA& p) { return &p.rot; }
		static bool* selected(kmp::AREA& p) { return &p.selected; }

		static uint8_t* type(kmp::AREA& p) { return &p.type; }
		static uint8_t* shape(kmp::AREA& p) { return &p.shape; }
		static glm::vec3* scale(kmp::AREA& p) { return &p.scale; }
		static uint8_t* priority(kmp::AREA& p) { return &p.priority; }

		static uint16_t* s1(kmp::AREA& p) { return &p.s1; }
		static uint16_t* s2(kmp::AREA& p) { return &p.s2; }
		static uint8_t* cameraIndex(kmp::AREA& p) { return &p.cam; }
		static uint8_t* routeIndex(kmp::AREA& p) { return &p.route; }
		static uint8_t* enemyIndex(kmp::AREA& p) { return &p.enemy; }
		static uint16_t* pad(kmp::AREA& p) { return &p.pad; }
	};

	template <>
	struct EditableTraits<kmp::CAME> : EditableTraitsBase<kmp::CAME> {
		static glm::vec3* pos(kmp::CAME& p) { return &p.pos; }
		static glm::vec3* rot(kmp::CAME& p) { return &p.rot; }
		static bool* selected(kmp::CAME& p) { return &p.selected; }

		static uint8_t* type(kmp::CAME& p) { return &p.type; }
		static uint8_t* nextCam(kmp::CAME& p) { return &p.nextCam; }
		static uint8_t* shake(kmp::CAME& p) { return &p.shake; }
		static uint8_t* routeIndex(kmp::CAME& p) { return &p.route; }

		static uint16_t* vCam(kmp::CAME& p) { return &p.vCam; }
		static uint16_t* vZoom(kmp::CAME& p) { return &p.vZoom; }
		static uint16_t* vView(kmp::CAME& p) { return &p.vView; }

		static uint8_t* start(kmp::CAME& p) { return &p.start; }
		static uint8_t* movie(kmp::CAME& p) { return &p.movie; }

		static float* zoomStart(kmp::CAME& p) { return &p.zoomStart; }
		static float* zoomEnd(kmp::CAME& p) { return &p.zoomEnd; }

		static glm::vec3* viewStart(kmp::CAME& p) { return &p.viewStart; }
		static glm::vec3* viewEnd(kmp::CAME& p) { return &p.viewEnd; }

		static float* time(kmp::CAME& p) { return &p.time; }
	};

	template <>
	struct EditableTraits<kmp::JGPT> : EditableTraitsBase<kmp::JGPT> {
		static glm::vec3* pos(kmp::JGPT& p) { return &p.pos; }
		static glm::vec3* rot(kmp::JGPT& p) { return &p.rot; }
		static uint16_t* id(kmp::JGPT& p) { return &p.id; }
		static uint16_t* soundTrig(kmp::JGPT& p) { return &p.sound; }
		static bool* selected(kmp::JGPT& p) { return &p.selected; }
	};

	template <>
	struct EditableTraits<kmp::CKPT> : EditableTraitsBase<kmp::CKPT> {
		static float* x1(kmp::CKPT& p) { return &p.x1; }
		static float* x2(kmp::CKPT& p) { return &p.x2; }
		static float* z1(kmp::CKPT& p) { return &p.z1; }
		static float* z2(kmp::CKPT& p) { return &p.z2; }
		static uint8_t* type(kmp::CKPT&& p) { return &p.type; }
		static uint8_t* respawn(kmp::CKPT&& p) { return &p.respawn; }
		static bool* selected(kmp::CKPT&& p) { return &p.selected; }
	};

	template <>
	struct EditableTraits<kmp::POTI> : EditableTraitsBase<kmp::POTI>
	{
		static bool* selected(kmp::POTI& p) {
			return &p.selected;
		}

		static glm::vec3* pos(kmp::POTI_Point& pt) {
			return &pt.pos;
		}

		static glm::vec3* rot(kmp::POTI_Point&) {
			return nullptr;
		}

		static bool* selected(kmp::POTI_Point&) {
			return nullptr;
		}

		static uint16_t* num(kmp::POTI& p) {
			return &p.num;
		}

		static uint8_t* s1(kmp::POTI& p) {
			return &p.s1;
		}

		static uint8_t* s2(kmp::POTI& p) {
			return &p.s2;
		}
	};

	template <>
	struct EditableTraits<kmp::CNPT> : EditableTraitsBase<kmp::CNPT> {
		static glm::vec3* pos(kmp::CNPT& p) { return &p.pos; }
		static glm::vec3* rot(kmp::CNPT& p) { return &p.rot; }
		static uint16_t* id(kmp::CNPT& p) { return &p.id; }
		static uint16_t* s1(kmp::CNPT& p) { return &p.effect; }
		static bool* selected(kmp::CNPT& p) { return &p.selected; }
	};

	template <>
	struct EditableTraits<kmp::MSPT> : EditableTraitsBase<kmp::MSPT> {
		static glm::vec3* pos(kmp::MSPT& p) { return &p.pos; }
		static glm::vec3* rot(kmp::MSPT& p) { return &p.rot; }
		static uint16_t* id(kmp::MSPT& p) { return &p.id; }
		static bool* selected(kmp::MSPT& p) { return &p.selected; }
	};

	template <typename T>
	std::vector<EditablePoint> MakeEditable(std::vector<T>& src) {
		std::vector<EditablePoint> out;
		out.reserve(src.size());

		for (auto& p : src) {
			EditablePoint ep;

			ep.pos = EditableTraits<T>::pos(p);
			ep.rot = EditableTraits<T>::rot(p);
			ep.scale = EditableTraits<T>::scale(p);

			ep.selected = EditableTraits<T>::selected(p);

			ep.type = EditableTraits<T>::type(p);
			ep.routeIndex = EditableTraits<T>::routeIndex(p);

			//AREA
			ep.shape = EditableTraits<T>::shape(p);
			ep.priority = EditableTraits<T>::priority(p);

			ep.cameraIndex = EditableTraits<T>::cameraIndex(p);
			ep.enemyIndex = EditableTraits<T>::enemyIndex(p);

			ep.setting1 = EditableTraits<T>::setting1(p);
			ep.setting2 = EditableTraits<T>::setting2(p);
			ep.pad = EditableTraits<T>::pad(p);

			//CAME
			ep.nextCam = EditableTraits<T>::nextCam(p);
			ep.shake = EditableTraits<T>::shake(p);

			ep.vCam = EditableTraits<T>::vCam(p);
			ep.vZoom = EditableTraits<T>::vZoom(p);
			ep.vView = EditableTraits<T>::vView(p);

			ep.start = EditableTraits<T>::start(p);
			ep.movie = EditableTraits<T>::movie(p);

			ep.zoomStart = EditableTraits<T>::zoomStart(p);
			ep.zoomEnd = EditableTraits<T>::zoomEnd(p);

			ep.viewStart = EditableTraits<T>::viewStart(p);
			ep.viewEnd = EditableTraits<T>::viewEnd(p);

			ep.time = EditableTraits<T>::time(p);

			//JGPT CKPT MSPT
			ep.id = EditableTraits<T>::id(p);
			ep.sound = EditableTraits<T>::soundTrig(p);

			ep.deviation = EditableTraits<T>::deviation(p);
			ep.s1 = EditableTraits<T>::s1(p);
			ep.s2 = EditableTraits<T>::s2(p);
			ep.s3 = EditableTraits<T>::s3(p);

			ep.objSetting1 = EditableTraits<T>::objSettings1(p);
			ep.objSetting2 = EditableTraits<T>::objSettings2(p);
			ep.objSetting3 = EditableTraits<T>::objSettings3(p);
			ep.objSetting4 = EditableTraits<T>::objSettings4(p);
			ep.objSetting5 = EditableTraits<T>::objSettings5(p);
			ep.objSetting6 = EditableTraits<T>::objSettings6(p);
			ep.objSetting7 = EditableTraits<T>::objSettings7(p);
			ep.objSetting8 = EditableTraits<T>::objSettings8(p);

			ep.x1 = EditableTraits<T>::x1(p);
			ep.x2 = EditableTraits<T>::x2(p);
			ep.z1 = EditableTraits<T>::z1(p);
			ep.z2 = EditableTraits<T>::z2(p);

			ep.respawn = EditableTraits<T>::respawn(p);

			ep.num = EditableTraits<T>::num(p);

			out.push_back(ep);
		}
		return out;
	}

	template <typename T>
	std::vector<int> DeleteSelected(std::vector<T>& pts) {
		std::vector<int> deleted;

		for (int i = (int)pts.size() - 1; i >= 0; --i) {
			if (pts[i].selected) {
				deleted.push_back(i);
				pts.erase(pts.begin() + i);
			}
		}

		return deleted;
	}

	template <typename T>
	void SelectAll(std::vector<T>& pts) {
		for (auto& p : pts)
			p.selected = true;
	}

	template <typename T, typename Func>
	void applyToSelected(std::vector<T>& pts, int primaryIndex, Func fn)
	{
		if (primaryIndex < 0)
			return;

		for (auto& p : pts)
			if (p.selected)
				fn(p);
	}

	template <typename T>
	using CreateNewFunc = std::function<T(const T* src, const glm::vec3& hit)>;

	template <typename T>
	int AddPointAtClick(
		std::vector<T>& pts,
		const glm::vec3& hit,
		int hoverIndex,
		int primaryIndex,
		CreateNewFunc<T> createNew)
	{
		T newPt;

		if (hoverIndex >= 0) {
			newPt = createNew(&pts[hoverIndex], hit);
		}
		else if (primaryIndex >= 0) {
			newPt = createNew(&pts[primaryIndex], hit);
		}
		else {
			newPt = createNew(nullptr, hit);
		}

		for (auto& p : pts)
			p.selected = false;

		newPt.selected = true;
		pts.push_back(newPt);

		return (int)pts.size() - 1;
	}

	enum class SectionType {
		ENPT, ITPT, AREA, POTI, CAME, CNPT,other
	};

	template<typename T>
	struct SectionContext {
		std::vector<T>* data = nullptr;
		CreateNewFunc<T> createNew = nullptr;
		PointUI ui;
		SectionType type = SectionType::other;

		SectionContext() = default;
		SectionContext(std::vector<T>& d, CreateNewFunc<T> c, SectionType t)
			: data(&d), createNew(c), type(t) {
		}
	};
	static std::vector<std::string> cameraList;
	static std::vector<std::string> cannonList;
	static std::vector<std::string> routeList;
	static std::vector<std::string> fogList;
	bool RaycastOrPlane(glm::vec3& hit)
	{
		glm::vec3 rayOrigin = mCamera.GetPosition();
		glm::vec3 rayDir = ScreenToWorldRay(
			Input::GetMousePosition().x,
			Input::GetMousePosition().y,
			vx, vy, vw, vh,
			projection, view
		);

		if (RaycastKCL(rayOrigin, rayDir, hit))
			return true;

		if (RayPlaneIntersection(
			rayOrigin,
			rayDir,
			glm::vec3(0, 0, 0),
			glm::vec3(0, 1, 0),
			hit))
		{
			return true;
		}

		return false;
	}
	static kmp::KTPT CreateKTPT(const kmp::KTPT* src, const glm::vec3& hit)
	{
		kmp::KTPT out{};
		if (src) {
			out = *src;
			out.pos = hit;
		}
		else {
			out.pos = hit;
			out.rot = glm::vec3(0);
		}
		return out;
	}

	static kmp::ENPT CreateENPT(const kmp::ENPT* src, const glm::vec3& hit)
	{
		kmp::ENPT out{};
		if (src) {
			out = *src;
			out.pos = hit;
		}
		else {
			out.pos = hit;
			out.deviation = 10;
			out.s1 = 0;
			out.s2 = 0;
			out.s3 = 0;
		}
		return out;
	}

	static kmp::CAME CreateCAME(const kmp::CAME* src, const glm::vec3& hit)
	{
		kmp::CAME out{};
		if (src) {
			out = *src;
			out.pos = hit;
		}
		else {
			out.pos = hit;
			out.rot = glm::vec3(0);
			out.viewStart = glm::vec3(0);
			out.viewEnd = glm::vec3(0);
			out.start = 0;
			out.movie = 0;
			out.nextCam = 0;
			out.route = 0;
			out.vCam = 0;
			out.vZoom = 0;
			out.vView = 0;
			out.zoomStart = 0;
			out.zoomEnd = 0;
		}
		cameraList.push_back("Camera " + std::to_string(cameraList.size()));
		return out;
	}

	static kmp::AREA CreateAREA(const kmp::AREA* src, const glm::vec3& hit)
	{
		kmp::AREA out{};
		if (src) {
			out = *src;
			out.pos = hit;
		}
		else {
			out.pos = hit;
			out.rot = glm::vec3(0);
			out.scale = glm::vec3(1);
			out.shape = 0;
			out.type = 0;
			out.cam = 0;
			out.priority = 0;
			out.s1 = 0;
			out.s2 = 0;
			out.route = 0;
		}
		return out;
	}

	static kmp::ITPT CreateITPT(const kmp::ITPT* src, const glm::vec3& hit)
	{
		kmp::ITPT out{};
		if (src) {
			out = *src;
			out.pos = hit;
		}
		else {
			out.pos = hit;
			out.deviation = 10;
			out.s1 = 0;
			out.s2 = 0;
		}
		return out;
	}

	static kmp::GOBJ CreateGOBJ(const kmp::GOBJ* src, const glm::vec3& hit)
	{
		kmp::GOBJ out{};
		if (src) {
			out = *src;
			out.pos = hit;
		}
		else {
			out.pos = hit;
			out.rot = glm::vec3(0);
			out.scale = glm::vec3(1);

			out.id = 101;
			out.args[0] = 0;
			out.args[1] = 0;
			out.args[2] = 0;
			out.args[3] = 0;
			out.args[4] = 0;
			out.args[5] = 0;
			out.args[6] = 0;
			out.args[7] = 0;
			out.presence = 7;
		}
		return out;
	}

	static kmp::JGPT CreateJGPT(const kmp::JGPT* src, const glm::vec3& hit)
	{
		kmp::JGPT out{};
		if (src) {
			out = *src;
			out.pos = hit;
		}
		else {
			out.pos = hit;
			out.rot = glm::vec3(0);
			out.id = 65535;
			out.sound = 65535;
		}
		return out;
	}

	static kmp::MSPT CreateMSPT(const kmp::MSPT* src, const glm::vec3& hit)
	{
		kmp::MSPT out{};
		if (src) {
			out = *src;
			out.pos = hit;
		}
		else {
			out.pos = hit;
			out.rot = glm::vec3(0);
			out.id = 65535;
		}
		return out;
	}

	static kmp::CNPT CreateCNPT(const kmp::CNPT* src, const glm::vec3& hit)
	{
		kmp::CNPT out{};
		if (src) {
			out = *src;
			out.pos = hit;
		}
		else {
			out.pos = hit;
			out.rot = glm::vec3(0);
			out.id = 65535;
			out.effect = 0;
		}
		cannonList.push_back("Cannon " + std::to_string(cannonList.size()));
		return out;
	}

	int hoverIndex = -1;
	int fixedHoverIndex = -1;
	std::vector<int> dragIndices;
	int primaryIndex = -1;
	std::vector<glm::vec3> dragStartPositions;
	glm::vec3 dragStartPrimaryPos;

	glm::vec3 dragPlaneNormal = glm::vec3(0, 1, 0);
	glm::vec3 dragPlanePoint = glm::vec3(0, 0, 0);
	UIPanel mainPanel;
	float itemHeight = 42.0f;
	float listHeight;
	float clickCooldown = 0.0f;
	float listScroll = 0.0f;
	int selected = -1;
	glm::vec4 itemColor = glm::vec4(0.75f, 0.75f, 0.75f, 1.0f);
	glm::vec4 selColor = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
	glm::vec4 barColor = glm::vec4(0.35f, 0.35f, 0.35f, 1.0f);
	glm::vec4 knobColor = glm::vec4(0.55f, 0.55f, 0.55f, 1.0f);
	glm::vec4 textColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.1f);
	glm::vec4 bgColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glm::vec4 bg2Color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	bool loaded = false;
	void SetActivity(const std::string& details, const std::string& state);
	~RenderContext();
	void DrawSlider(
		float x, float y,
		float w, float h,
		float& value,
		bool& dragging
	);
	std::string FormatColorCode(const glm::vec4& c);
	void DrawCheckerboard(float x, float y, float w, float h);
	bool mousePressedInside(float x, float y, float w, float h);
	lyt::TDDraw ddraw;
	lyt::CharWriter writer;
	std::string OpenFileDialog();
	std::string SaveFileDialogSZS();
	std::string SaveFileDialogKMP();
	bool LoadKMP(std::filesystem::path path);
	std::u16string utf8ToUtf16(const std::string& utf8);
	void DrawTextBase(
		float x, float y,
		const std::string& text,
		const glm::vec4& color,
		float scaleX, float scaleY,
		const std::function<float(float)>& ratioFunc);
	UIItem* activeColorPicker = nullptr;
	void DrawTextCenter(float x, float y, float w, float h, const std::string& text, const glm::vec4& color);
	void DrawColorPickerPopup(UIItem* item, float x, float y);
	void Reset();
	void InitFbo(int width, int height);
	void SaveSZS(std::string path);
	void SaveKMP(std::string path);
	GLuint backgroundProgram;
	void LoadFile(std::string path);
	void DrawList(UIItem* item, float indent, float y);
	SceneCamera mCamera;
	void SetLights(bres::LightSet& lights);
	void SetLights2(bres::LightSet& lights);
	RenderContext();
	void BuildPanels();
	void DrawUIBase(float x, float y, float w, float h, const glm::vec4& color, float z);
	bool RayPlaneIntersection(glm::vec3 ro, glm::vec3 rd, glm::vec3 p0, glm::vec3 n, glm::vec3& out);
	bool allEqualPosX();
	bool allEqualPosY();
	bool allEqualPosZ();
	bool allEqualRotX();
	bool allEqualRotY();
	bool allEqualRotZ();
	void SetAreaItemInvisible();
	bool RayTriangleIntersection(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, glm::vec3& outHit);
	bool RaycastKCL(const glm::vec3& origin, const glm::vec3& dir, glm::vec3& outHit);
	template <typename T>
	void ApplyTextEditToPoint(SectionContext<T>& ctx);
	void UpdateUIFromSelection(const PointUI& ui, const std::vector<EditablePoint>& pts, int primaryIndex);
	void BeginDragSelection(int primaryIndex, const std::vector<EditablePoint>& pts, std::vector<int>& dragIndices, std::vector<glm::vec3>& dragStartPositions, glm::vec3& dragStartPrimaryPos);
	int ComputeHoverIndex(const std::vector<EditablePoint>& pts, const glm::mat4& projection, const glm::mat4& view, float vx, float vy, float vw, float vh, glm::vec2 mouse, float hoverRadius);
	void HandleClickSelection(std::vector<EditablePoint>& pts, int hoverIndex, int& primaryIndex, int& anchorIndex, bool ctrlDown);
	void HandleMouseUp();
	bool Update(float deltaTime);
	int findCursorPos(const std::string& text, float localX, float scaleX, const std::function<float(float)>& ratioFunc);
	glm::vec3 ScreenToWorldRay(float mouseX, float mouseY, float vx, float vy, float vw, float vh, const glm::mat4& projection, const glm::mat4& view);
	void SetAreaItem(UIItem* item);
	void HandleClick(float mouseX, float mouseY);
	void HandleMouseWrap();
	void HandleMouseMove(float mouseX, float mouseY);
	float measureTextWidth(
		const std::string& text,
		float scaleX,
		const std::function<float(float)>& ratioFunc);
	bool HandleKey(int key);
	bool HandleChar(char c);
	void DrawTriangleRight(float x, float y, float size, const glm::vec4& color, float z);
	void DrawTriangleDown(float x, float y, float size, const glm::vec4& color, float z);
	void DrawGroup(UIItem* item, float indent, float y);
	void DrawCheckbox(UIItem* item, float indent, float y);
	void DrawNumber(UIItem* item, float indent, float y);
	void DrawFloat(UIItem* item, float indent, float y);
	glm::vec4 HSVtoRGB(const glm::vec3& hsv);
	void DrawColor(UIItem* item, float indent, float y);
	void DrawLabel(UIItem* item, float indent, float y);
	void DrawButton(UIItem* item, float indent, float y);
	void DrawValue(UIItem* item, float indent, float y);
	void DrawRowBackground(const UIRow& row, float y, int index, float indent);
	glm::mat4 childView;
	glm::mat4 childProjection;
	glm::vec3 childCamPos;
	glm::vec3 childCamRot;
	void Render(GLFWwindow* mWindow, float deltaTime);
	void undo();
	void redo();
	glm::mat4 projection;
	glm::mat4 view;
	glm::vec4 frustum[6];
	glm::mat4 vp;
	int clickedIndex = -1;
	std::vector<bool> wasSelectedBeforeDown;
	GfxModel* modelPoint = nullptr;
	GfxModel* modelPointSelection = nullptr;
	GfxModel* modelPath = nullptr;
	GfxModel* modelArrow = nullptr;
	GfxModel* modelArrowUp = nullptr;
	GfxModel* startZoneWideModel = nullptr;
	GfxModel* startZoneNarrowModel = nullptr;
	GfxModel* modelSizeCircle = nullptr;
	GfxModel* modelPlayerPos = nullptr;
	GfxModel* modelPanel = nullptr;
	GfxModel* modelPanelCK = nullptr;
	GfxModel* modelPanelWithoutBacksize = nullptr;
	GfxModel* modelAreaBox = nullptr;
	GfxModel* modelAreaCylinder = nullptr;
	GLuint ktptProgram = CreateShaderProgram(R"(#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

uniform mat4 u_mvp;
uniform vec4 u_color;

out vec4 v_Color;

void main()
{
    gl_Position = u_mvp * vec4(pos, 1.0);
    v_Color = u_color;
}
)", R"(#version 330 core

in vec4 v_Color;
out vec4 FragColor;

void main()
{
    FragColor = v_Color;
}
)");
GLuint normalProgram = CreateShaderProgram(R"(#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

uniform mat4 u_mvp;
uniform vec4 u_color;

out vec3 v_Normal;
out vec4 v_Color;
void main()
{
    gl_Position = u_mvp * vec4(pos, 1.0);
    v_Normal = normal;
    v_Color = u_color;
}
)", R"(
#version 330 core

in vec3 v_Normal;
in vec4 v_Color;
out vec4 FragColor;
void main()
{
    vec3 n = normalize(v_Normal);
    vec3 lightDir = vec3(0, 0, 1); 
    float lightIncidence = max(0.0, dot(normalize(lightDir), n));

    FragColor = v_Color * (0.3 + 0.7 * lightIncidence);
}
)");
	GLuint u_mvp = glGetUniformLocation(ktptProgram, "u_mvp");
	GLuint u_color = glGetUniformLocation(ktptProgram, "u_color");
	bool mAboutOpen = false;
	bool mOptionsOpen;
	bool mCourseOpen = false;
	bool bIsDockingSetUp = false;
	bres brresRenderer;
	bres brresRenderer2;
	SKclIO kclRenderer;
	blight posteffect1;
	posteffects posteffect2;
	fog bfg;
	posteffects::BBLM bblmRes;
	posteffects::BDOF bdofRes;
	GLuint mColorTex;
	fog::BFG bfgRes;
	std::string mPath;
	bres::LightSet mLights;
	std::shared_ptr<Archive::U8> archive;
	kmp kmpLoader;
	kmp::KMP loadedKMP;
	kmp::KMP initKMP;
	kmp::KMP dragBeforeKMP;

	GLuint uProjLoc;
	GLuint uColorLoc;

	UIItem* courseInfoItem = nullptr;
	UIItem* startPositionItem = nullptr;
	UIItem* courseTypeLabelItem = nullptr;
	UIItem* courseTypeValueItem = nullptr;
	UIItem* lapLabelItem = nullptr;
	UIItem* lapValueItem = nullptr;
	UIItem* speedLabelItem = nullptr;
	UIItem* speedValueItem = nullptr;
	UIItem* flareLabelItem = nullptr;
	UIItem* flareValueItem = nullptr;
	UIItem* flareColorLabelItem = nullptr;
	UIItem* flareColorValueItem = nullptr;
	UIItem* poleLabelItem = nullptr;
	UIItem* poleValueItem = nullptr;
	UIItem* startZoneLabelItem = nullptr;
	UIItem* startZoneValueItem = nullptr;
	UIItemState dragBeforeUI;

	int posXId_start = -1, posYId_start = -1, posZId_start = -1; int rotXId_start = -1, rotYId_start = -1, rotZId_start = -1;
	bool altDown = false;
	int vx;
	int vy;
	int vw;
	int vh;
	int anchorIndex = -1;
	bool dragPending = false; 
	bool overLabel2 = false;
	bool overTextbox2 = false;
	glm::vec2 dragStartMouse; 
	float dragThreshold = 4.0f;
	SectionContext<kmp::KTPT> ctxKTPT;
	SectionContext<kmp::ENPT> ctxENPT;
	SectionContext<kmp::ITPT> ctxITPT;
	SectionContext<kmp::GOBJ> ctxGOBJ;
	SectionContext<kmp::AREA> ctxAREA;
	SectionContext<kmp::CAME> ctxCAME;
	SectionContext<kmp::JGPT> ctxJGPT;
	SectionContext<kmp::CKPT> ctxCKPT;
	SectionContext<kmp::CNPT> ctxCNPT;
	SectionContext<kmp::POTI> ctxPOTI;
	SectionContext<kmp::MSPT> ctxMSPT;
	UIGroup* currentOpenGroup = nullptr;
	float editingY = 0.0f;

	UIItem* jgptItem = nullptr; 
	UIItem* ckptItem = nullptr;
	UIItem* gobjItem = nullptr;
	UIItem* potiItem = nullptr;
	UIItem* areaItem = nullptr; 
	UIItem* cameItem = nullptr;
	UIItem* msptItem = nullptr;
	UIItem* cnptItem = nullptr;
	UIItem* itptItem = nullptr;
	UIItem* enptItem = nullptr;
	UIItem* opened = nullptr;
	uint8_t cRoute = -1;

	bool isViewerOpen = false;
	void loadCourseBrres();
	void errorCheck();
	GLFWcursor* ibeamCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
	GLFWcursor* arrowCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	GLFWcursor* resizeCursor = glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
	template<typename T>
	void ProcessSectionEditing(SectionContext<T>& ctx, int& primaryIndex, int& anchorIndex, int& hoverIndex, std::vector<int>& dragIndices, std::vector<glm::vec3>& dragStartPositions, glm::vec3& dragStartPrimaryPos, glm::mat4 projection, glm::mat4 view, float vx, float vy, float vw, float vh, bool altDown);
};