#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdarg>
#include <typeinfo>
#define Rectangle rlRectangle
#ifdef _WIN32
	#include "raylib_win32.h"
#else
extern "C" {
    #include "raylib.h"
    #include "rlgl.h"
}
#include "raymath.h"
#endif
#undef Rectangle

enum class HorizontalAlign : uint8_t {
	Left = 1 << 0,
	Center = 1 << 1,
	Right = 1 << 2
};

enum class VerticalAlign : uint8_t {
	Top = 1 << 3,
	Middle = 1 << 4,
	Bottom = 1 << 5
};

enum class TextAlign : uint8_t {
	None = 0
};

inline TextAlign operator|(HorizontalAlign a, VerticalAlign b) {
	return static_cast<TextAlign>(
		static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline TextAlign operator|(VerticalAlign a, HorizontalAlign b) {
	return static_cast<TextAlign>(
		static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline TextAlign operator|(TextAlign a, HorizontalAlign b) {
	return static_cast<TextAlign>(
		static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline TextAlign operator|(TextAlign a, VerticalAlign b) {
	return static_cast<TextAlign>(
		static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool HasFlag(TextAlign value, HorizontalAlign flag) {
	return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

inline bool HasFlag(TextAlign value, VerticalAlign flag) {
	return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

inline Vector2 GetAlignedPosition(const char* text, rlRectangle rec, float fontsize, float spacing, TextAlign align)
{
	Vector2 measure = MeasureTextEx(GetFontDefault(), text, fontsize, spacing);
	float posX = rec.x;
	float posY = rec.y;

	if (HasFlag(align, HorizontalAlign::Center))
		posX += (rec.width - measure.x) / 2.0f;
	else if (HasFlag(align, HorizontalAlign::Right))
		posX += rec.width - measure.x;

	if (HasFlag(align, VerticalAlign::Middle))
		posY += (rec.height - measure.y) / 2.0f;
	else if (HasFlag(align, VerticalAlign::Bottom))
		posY += rec.height - measure.y;

	return { posX, posY };
}

inline void DrawTextAligned(const char* text, rlRectangle rec, float fontsize, Color rgba, TextAlign align)
{
	Vector2 pos = GetAlignedPosition(text, rec, fontsize, 0.0f, align);
	DrawText(text, static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(fontsize), rgba);
}

inline void DrawTextAligned(const char* text, float x, float y, float width, float height, float fontsize, Color rgba, TextAlign align)
{
	DrawTextAligned(text, { x, y, width, height }, fontsize, rgba, align);
}

inline void DrawTextAlignedEx(const Font& font, const char* text, rlRectangle rec, float fontsize, float spacing, Color rgba, TextAlign align)
{
	Vector2 pos = GetAlignedPosition(text, rec, fontsize, spacing, align);
	DrawTextEx(font, text, { pos.x, pos.y }, fontsize, spacing, rgba);
}

inline void DrawTextAlignedEx(const Font& font, const char* text, float x, float y, float width, float height, float fontsize, float spacing, Color rgba, TextAlign align)
{
	DrawTextAlignedEx(font, text, { x, y, width, height }, fontsize, spacing, rgba, align);
}

template<typename T>
	requires std::is_integral_v<T> || std::is_floating_point_v<T>
struct Padding {
	T left{};
	T top{};
	T right{};
	T bottom{};
};

template<typename T>
	requires std::is_integral_v<T> || std::is_floating_point_v<T>
using Margin = Padding<T>;

template<typename T>
	requires std::is_integral_v<T> || std::is_floating_point_v<T>
struct Rectangle {
	T x{};
	T y{};
	T width{};
	T height{};

	constexpr Rectangle() = default;

	constexpr Rectangle(T x, T y, T width, T height)
		: x(x), y(y), width(width), height(height)
	{ }

	constexpr operator rlRectangle() const {
		return rlRectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height) };
	}

	constexpr Rectangle(const rlRectangle& rect) 
		: x(static_cast<T>(rect.x)),
		  y(static_cast<T>(rect.y)),
		  width(static_cast<T>(rect.width)),
		  height(static_cast<T>(rect.height))
	{ }

	constexpr Rectangle& operator=(const rlRectangle& rect) {
		x = static_cast<T>(rect.x);
		y = static_cast<T>(rect.y);
		width = static_cast<T>(rect.width);
		height = static_cast<T>(rect.height);
		return *this;
	}

	template<typename U>
		requires std::is_integral_v<U> || std::is_floating_point_v<U>
	constexpr operator Rectangle<U>() const {
		return Rectangle<U>{ static_cast<U>(x), static_cast<U>(y), static_cast<U>(width), static_cast<U>(height) };
	}

#ifdef _WIN32
	constexpr operator RECT() const {
		return RECT{ static_cast<LONG>(x), static_cast<LONG>(y), static_cast<LONG>(x + width), static_cast<LONG>(y + height) };
	}

	constexpr Rectangle(const RECT& rect)
		: x(static_cast<T>(rect.left)),
		  y(static_cast<T>(rect.top)),
		  width(static_cast<T>(rect.right - rect.left)),
		  height(static_cast<T>(rect.bottom - rect.top))
	{ }

	constexpr Rectangle& operator=(const RECT& rect) {
		x = static_cast<T>(rect.left);
		y = static_cast<T>(rect.top);
		width = static_cast<T>(rect.right - rect.left);
		height = static_cast<T>(rect.bottom - rect.top);
		return *this;
	}
#endif

	constexpr Rectangle<T> operator+(const Padding<T>& pad) const {
		return Rectangle<T>{
			x + pad.left,
			y + pad.top,
			width - (pad.left + pad.right),
			height - (pad.top + pad.bottom)
		};
	}

	constexpr Rectangle<T> operator-(const Padding<T>& pad) const {
		return Rectangle<T>{
			x - pad.left,
			y - pad.top,
			width + (pad.left + pad.right),
			height + (pad.top + pad.bottom)
		};
	}

	template<typename U>
		requires std::is_integral_v<U> || std::is_floating_point_v<U>
	constexpr bool contains(U px, U py) const {
		return (px >= static_cast<U>(x)) && (px < static_cast<U>(x + width)) &&
			(py >= static_cast<U>(y)) && (py < static_cast<U>(y + height));
	}

	template<typename U>
		requires std::is_integral_v<U> || std::is_floating_point_v<U>
	constexpr bool intersects(const Rectangle<U>&other) const {
		return !(other.x >= static_cast<U>(x + width) ||
			other.x + other.width <= static_cast<U>(x) ||
			other.y >= static_cast<U>(y + height) ||
			other.y + other.height <= static_cast<U>(y));
	}

	constexpr T right() const {
		return x + width;
	}

	constexpr T bottom() const {
		return y + height;
	}
};

template<typename T>
	requires std::is_integral_v<T> || std::is_floating_point_v<T>
struct MarginRectangle {
	Rectangle<T> rect{};
	Margin<T> margin{};

	constexpr MarginRectangle() = default;

	constexpr MarginRectangle(T x, T y, T width, T height, Margin<T> margin = {})
		: rect{ x, y, width, height }, margin(margin)
	{ }

	constexpr MarginRectangle(Rectangle<T> r, Margin<T> m = {})
		: rect(r), margin(m)
	{ }

	constexpr Rectangle<T> with_margin() const {
		return rect - margin; // expands outward
	}

	constexpr Rectangle<T> without_margin() const {
		return rect; // base rectangle
	}
};

template<typename T>
	requires std::is_integral_v<T> || std::is_floating_point_v<T>
struct PaddedRectangle {
	Rectangle<T> rect{};
	Padding<T> padding{};

	constexpr PaddedRectangle() = default;

	constexpr PaddedRectangle(T x, T y, T width, T height, Padding<T> padding = {})
		: rect{ x, y, width, height }, padding(padding)
	{ }

	constexpr PaddedRectangle(Rectangle<T> r, Padding<T> p = {})
		: rect(r), padding(p)
	{ }

	constexpr Rectangle<T> with_padding() const {
		return rect + padding; // expands inward visually (shrinks usable content)
	}

	constexpr Rectangle<T> without_padding() const {
		return rect; // base rectangle, no padding
	}
};

template<typename T>
	requires std::is_integral_v<T> || std::is_floating_point_v<T>
struct Box {
	Rectangle<T> rect{};
	Padding<T> padding{};
	Margin<T> margin{};

	constexpr Box() = default;

	constexpr Box(T x, T y, T width, T height, Padding<T> padding = {}, Margin<T> margin = {})
		: rect{ x, y, width, height }, padding(padding), margin(margin)
	{ }

	constexpr Box(Rectangle<T> r, Padding<T> p = {}, Margin<T> m = {})
		: rect(r), padding(p), margin(m)
	{ }

	constexpr Rectangle<T> with_padding() const {
		return rect + padding; // expands inward visually (shrinks usable content)
	}

	constexpr Rectangle<T> without_padding() const {
		return rect; // base rectangle, no padding
	}

	constexpr Rectangle<T> with_margin() const {
		return rect - margin; // expands outward
	}

	constexpr Rectangle<T> without_margin() const {
		return rect; // base rectangle
	}
};

namespace File
{
	inline bool Exists(const std::filesystem::path& filePath) {
		return std::filesystem::exists(filePath);
	}

	inline bool Delete(const std::filesystem::path& filePath) {
		return std::filesystem::remove(filePath);
	}

	inline bool Copy(const std::filesystem::path& sourcePath, const std::filesystem::path& destPath, bool overwrite = false) {
		std::filesystem::copy_options options = std::filesystem::copy_options::none;
		if (overwrite) {
			options |= std::filesystem::copy_options::overwrite_existing;
		}

		return std::filesystem::copy_file(sourcePath, destPath, options);
	}

	inline void Move(const std::filesystem::path& sourcePath, const std::filesystem::path& destPath, bool overwrite = false) {
		if (overwrite && std::filesystem::exists(destPath)) {
			std::filesystem::remove(destPath);
		}

		std::filesystem::rename(sourcePath, destPath);
	}
}

namespace Directory
{
	inline bool Exists(const std::filesystem::path& dirPath) {
		return std::filesystem::exists(dirPath);
	}

	inline bool Create(const std::filesystem::path& dirPath) {
		return std::filesystem::create_directory(dirPath);
	}

	inline bool Delete(const std::filesystem::path& dirPath) {
		return std::filesystem::remove(dirPath);
	}

	inline std::vector<std::filesystem::path> GetFiles(const std::filesystem::path& dirPath) {
		std::vector<std::filesystem::path> files;
		for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
			if (entry.is_regular_file()) {
				files.push_back(entry.path());
			}
		}
		return files;
	}

	inline std::vector<std::filesystem::path> GetDirectories(const std::filesystem::path& dirPath) {
		std::vector<std::filesystem::path> directories;
		for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
			if (entry.is_directory()) {
				directories.push_back(entry.path());
			}
		}
		return directories;
	}

	inline std::vector<std::filesystem::path> GetFilesWithExtension(const std::filesystem::path& dirPath, const std::string& extension) {
		std::vector<std::filesystem::path> files;
		for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
			if (entry.is_regular_file() && entry.path().extension() == extension) {
				files.push_back(entry.path());
			}
		}
		return files;
	}
}

template<typename T>
	requires std::same_as<T, Image> || std::same_as<T, Texture2D> || std::same_as<T, RenderTexture2D> ||
		std::same_as<T, Font> || std::same_as<T, Mesh> || std::same_as<T, Model> ||
		std::same_as<T, Shader> || std::same_as<T, Wave> || std::same_as<T, Sound> ||
		std::same_as<T, Music> || std::same_as<T, AudioStream>
struct Managed {
	Managed(const T& v) : value(v), loaded(true) {}
	Managed& operator=(const Managed& other) {
		if (this != &other) {
			if (loaded)
				Unload();
			value = other.value;
			loaded = other.loaded;
		}
		return *this;
	}

	// --- Load constructors ---
	Managed(const char* fileName) requires std::same_as<T, Image> { value = LoadImage(fileName); loaded = true; }

	Managed(const char* fileName) requires std::same_as<T, Texture2D> { value = LoadTexture(fileName); loaded = true; }
	Managed(const Image& img) requires std::same_as<T, Texture2D> { value = LoadTextureFromImage(img); loaded = true; }

	Managed(int w, int h) requires std::same_as<T, RenderTexture2D> { value = LoadRenderTexture(w, h); loaded = true; }

	Managed(const char* fileName) requires std::same_as<T, Font> { value = LoadFont(fileName); loaded = true; }
	Managed(const char* fileName, int size, int* cps, int count) requires std::same_as<T, Font> { value = LoadFontEx(fileName, size, cps, count); loaded = true; }

	Managed() requires std::same_as<T, Mesh> { value = GenMeshCube(1.0f, 1.0f, 1.0f); loaded = true; }
	Managed(float radius, int rings, int slices) requires std::same_as<T, Mesh> { value = GenMeshSphere(radius, rings, slices); loaded = true; }
	Managed(float width, float length, int resX, int resZ) requires std::same_as<T, Mesh> { value = GenMeshPlane(width, length, resX, resZ); loaded = true; }
	// Use distinct signatures so MSVC does not collide
	Managed(float base, float height, int slices, bool cone = false) requires std::same_as<T, Mesh> {
		value = cone ? GenMeshCone(base, height, slices) : GenMeshCylinder(base, height, slices);
		loaded = true;
	}
	Managed(int sides, float radius) requires std::same_as<T, Mesh> { value = GenMeshPoly(sides, radius); loaded = true; }
	Managed(const char* fileName) requires std::same_as<T, Model> { value = LoadModel(fileName); loaded = true; }
	Managed(const Mesh& mesh) requires std::same_as<T, Model> { value = LoadModelFromMesh(mesh); loaded = true; }

	Managed(const char* vs, const char* fs) requires std::same_as<T, Shader> { value = LoadShader(vs, fs); loaded = true; }

	Managed(const char* fileName) requires std::same_as<T, Wave> { value = LoadWave(fileName); loaded = true; }
	Managed(const char* fileType, const unsigned char* data, int size) requires std::same_as<T, Wave> { value = LoadWaveFromMemory(fileType, data, size); loaded = true; }

	Managed(const char* fileName) requires std::same_as<T, Sound> { value = LoadSound(fileName); loaded = true; }
	Managed(const Wave& wave) requires std::same_as<T, Sound> { value = LoadSoundFromWave(wave); loaded = true; }

	Managed(const char* fileName) requires std::same_as<T, Music> { value = LoadMusicStream(fileName); loaded = true; }
	Managed(const char* type, const unsigned char* data, int size) requires std::same_as<T, Music> { value = LoadMusicStreamFromMemory(type, data, size); loaded = true; }

	Managed(unsigned int sr, unsigned int ss, unsigned int ch) requires std::same_as<T, AudioStream> { value = LoadAudioStream(sr, ss, ch); loaded = true; }

	// --- Unload handling ---
	void Unload() {
		if (!loaded) return;
		if constexpr (std::same_as<T, Image>) UnloadImage(value);
		else if constexpr (std::same_as<T, Texture2D>) UnloadTexture(value);
		else if constexpr (std::same_as<T, RenderTexture2D>) UnloadRenderTexture(value);
		else if constexpr (std::same_as<T, Font>) UnloadFont(value);
		else if constexpr (std::same_as<T, Mesh>) UnloadMesh(value);
		else if constexpr (std::same_as<T, Model>) UnloadModel(value);
		else if constexpr (std::same_as<T, Shader>) UnloadShader(value);
		else if constexpr (std::same_as<T, Wave>) UnloadWave(value);
		else if constexpr (std::same_as<T, Sound>) UnloadSound(value);
		else if constexpr (std::same_as<T, Music>) UnloadMusicStream(value);
		else if constexpr (std::same_as<T, AudioStream>) UnloadAudioStream(value);
		loaded = false;
	}

	// --- Access operators ---
	T* operator->() { return &value; }
	const T* operator->() const { return &value; }

	T& operator*() { return value; }
	const T& operator*() const { return value; }

	// --- Implicit conversions ---
	operator T& () { return value; }
	operator const T& () const { return value; }

	~Managed() { Unload(); }
protected:
	T value{};
	bool loaded = false;
};


inline void BeginUpscaleRender(const RenderTexture2D& target, float scale = 1.0f)
{
	BeginTextureMode(target);
	BeginMode2D(Camera2D{ .zoom = scale });
}

inline void EndUpscaleRender(const RenderTexture2D& target, Color background = BLACK)
{
	EndMode2D();
	EndTextureMode();
	float windowW = (float)GetScreenWidth();
	float windowH = (float)GetScreenHeight();
	float scale = std::min(windowW / target.texture.width, windowH / target.texture.height);
	float renderW = target.texture.width * scale;
	float renderH = target.texture.height * scale;
	float offsetX = (windowW - renderW) * 0.5f;
	float offsetY = (windowH - renderH) * 0.5f;
	BeginDrawing();
	ClearBackground(background);
	DrawTexturePro(
		target.texture,
		{ 0, 0, (float)target.texture.width, -(float)target.texture.height },
		{ offsetX, offsetY, renderW, renderH },
		{ 0, 0 },
		0,
		WHITE
	);
	EndDrawing();
}