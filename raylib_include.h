#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdarg>
#include <stdexcept>
#include <typeinfo>
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#define NOGDI
	#define CloseWindow	CloseWindowWin32
	#define ShowCursor	ShowCursorWin32
	#define LoadImageA	LoadImageAWin32
	#include "Windows.h"
	#undef RGB
	#undef LoadImage
	#undef CloseWindow
	#undef ShowCursor
	#undef DrawText
	#undef DrawTextEx
#endif
#define Rectangle rlRectangle
extern "C" {
    #include "raylib.h"
    #include "rlgl.h"
}
#include "raymath.h"
#undef Rectangle
#include <filesystem>
#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <utility>
#include <tuple>

template<typename K, typename V>
class ordered_map {
public:
	using key_type = K;
	using mapped_type = V;
	using value_type = std::pair<const K, V>;
	using size_type = size_t;

	using container_type = std::vector<std::pair<K, V>>;
	using iterator = typename container_type::iterator;
	using const_iterator = typename container_type::const_iterator;

	ordered_map() = default;
	ordered_map(const ordered_map&) = default;
	ordered_map& operator=(const ordered_map&) = default;
	ordered_map(ordered_map&&) noexcept = default;
	ordered_map& operator=(ordered_map&&) noexcept = default;

	// element access
	V& operator[](const K& key) {
		auto it = m_index.find(key);
		if (it == m_index.end()) {
			return push_back(key, V{})->second;
		}
		return m_data[it->second].second;
	}

	V& operator[](K&& key) {
		auto it = m_index.find(key);
		if (it == m_index.end()) {
			return push_back(std::move(key), V{})->second;
		}
		return m_data[it->second].second;
	}

	V& at(const K& key) { return m_data.at(m_index.at(key)).second; }
	const V& at(const K& key) const { return m_data.at(m_index.at(key)).second; }

	// modifiers
	std::pair<iterator, bool> insert(iterator pos, const value_type& value) {
		auto it = m_index.find(value.first);
		if (it != m_index.end())
			return { m_data.begin() + it->second, false };

		auto insertPos = m_data.insert(pos, { value.first, value.second });
		rebuild_index();
		return { insertPos, true };
	}

	std::pair<iterator, bool> insert(iterator pos, value_type&& value) {
		auto it = m_index.find(value.first);
		if (it != m_index.end())
			return { m_data.begin() + it->second, false };

		auto insertPos = m_data.insert(pos, std::move(value));
		rebuild_index();
		return { insertPos, true };
	}

	iterator push_back(const K& key, V&& value) {
		auto it = m_index.find(key);
		if (it != m_index.end())
			return m_data.begin() + it->second;

		m_data.emplace_back(key, std::move(value));
		m_index[key] = m_data.size() - 1;
		return std::prev(m_data.end());
	}

	iterator push_back(K&& key, V&& value) {
		auto it = m_index.find(key);
		if (it != m_index.end())
			return m_data.begin() + it->second;

		m_data.emplace_back(std::move(key), std::move(value));
		m_index[m_data.back().first] = m_data.size() - 1;
		return std::prev(m_data.end());
	}

	template<typename... Args>
	iterator emplace_back(const K& key, Args&&... args) {
		auto it = m_index.find(key);
		if (it != m_index.end())
			return m_data.begin() + it->second;

		m_data.emplace_back(key, V(std::forward<Args>(args)...));
		m_index[key] = m_data.size() - 1;
		return std::prev(m_data.end());
	}

	template<typename... Args>
	std::pair<iterator, bool> emplace(const K& key, Args&&... args) {
		auto it = m_index.find(key);
		if (it != m_index.end())
			return { m_data.begin() + it->second, false };

		m_data.emplace_back(key, V(std::forward<Args>(args)...));
		m_index[key] = m_data.size() - 1;
		return { std::prev(m_data.end()), true };
	}

	void erase(const K& key) {
		auto it = m_index.find(key);
		if (it == m_index.end()) return;

		size_t idx = it->second;
		m_data.erase(m_data.begin() + idx);
		m_index.erase(it);
		rebuild_index();
	}

	iterator erase(iterator pos) {
		if (pos == m_data.end()) return m_data.end();

		const K& key = pos->first;
		m_index.erase(key);
		auto next = m_data.erase(pos);
		rebuild_index();
		return next;
	}

	void clear() noexcept {
		m_data.clear();
		m_index.clear();
	}

	// lookup
	iterator find(const K& key) {
		auto it = m_index.find(key);
		if (it == m_index.end()) return m_data.end();
		return m_data.begin() + it->second;
	}

	const_iterator find(const K& key) const {
		auto it = m_index.find(key);
		if (it == m_index.end()) return m_data.end();
		return m_data.begin() + it->second;
	}

	bool contains(const K& key) const noexcept { return m_index.find(key) != m_index.end(); }
	size_type count(const K& key) const noexcept { return m_index.count(key); }

	// iteration
	iterator begin() noexcept { return m_data.begin(); }
	iterator end() noexcept { return m_data.end(); }
	const_iterator begin() const noexcept { return m_data.begin(); }
	const_iterator end() const noexcept { return m_data.end(); }
	const_iterator cbegin() const noexcept { return m_data.cbegin(); }
	const_iterator cend() const noexcept { return m_data.cend(); }

	// capacity
	bool empty() const noexcept { return m_data.empty(); }
	size_type size() const noexcept { return m_data.size(); }

private:
	void rebuild_index() {
		m_index.clear();
		for (size_t i = 0; i < m_data.size(); ++i)
			m_index[m_data[i].first] = i;
	}

	container_type m_data;
	std::unordered_map<K, size_t> m_index;
};

namespace rlx {
	struct RGB {
		RGB(uint8_t r, uint8_t g, uint8_t b) {
			rgb8 = (uint8_t)(((r & 0xE0)) | ((g >> 3) & 0x1C) | (b >> 6));
			rgb16 = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
			rgb32 = (uint32_t)(((uint8_t)(r) | ((uint16_t)((uint8_t)(g)) << 8)) | (((uint32_t)(uint8_t)(b)) << 16));
		}

		constexpr operator uint8_t() const { return rgb8; }
		constexpr operator uint16_t() const { return rgb16; }
		constexpr operator uint32_t() const { return rgb32; }
		constexpr operator Color() const {
			return { (unsigned char)(rgb32 & 0xFF), (unsigned char)((rgb32 >> 8) & 0xFF), (unsigned char)((rgb32 >> 16) & 0xFF), 255 };
		}
#ifdef _WIN32
		constexpr operator COLORREF() const {
			return (COLORREF)rgb32;
		}
#endif

	private:
		uint8_t rgb8;
		uint16_t rgb16;
		uint32_t rgb32;
	};

	struct RGBA {
		RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
			rgba8 = (uint8_t)(((r & 0xE0)) | ((g >> 3) & 0x1C) | (b >> 6)); // RGB 3-3-2
			rgba16 = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)); // RGB 5-6-5
			rgba32 = (uint32_t)(r | (g << 8) | (b << 16) | (a << 24)); // RGBA8888
		}

		constexpr operator uint8_t() const { return rgba8; }
		constexpr operator uint16_t() const { return rgba16; }
		constexpr operator uint32_t() const { return rgba32; }
		constexpr operator Color() const {
			return { (unsigned char)(rgba32 & 0xFF), (unsigned char)((rgba32 >> 8) & 0xFF), (unsigned char)((rgba32 >> 16) & 0xFF), (unsigned char)((rgba32 >> 24) & 0xFF) };
		}
#ifdef _WIN32
		constexpr operator COLORREF() const {
			return (COLORREF)(rgba32 & 0x00FFFFFF);
		}
#endif

	private:
		uint8_t rgba8;
		uint16_t rgba16;
		uint32_t rgba32;
	};

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

	inline void DrawGrid2D(int cells, float cellSize, Color color)
	{
		float size = cells * cellSize;

		for (int i = 0; i <= cells; i++)
		{
			float offset = i * cellSize;
			DrawLine((int)offset, 0, (int)offset, (int)size, color);
			DrawLine(0, (int)offset, (int)size, (int)offset, color);
		}
	}

	inline void DrawGrid2DEx(rlRectangle area, int cells, float cellSize, Color color)
	{
		float x0 = area.x;
		float y0 = area.y;
		float width = area.width;
		float height = area.height;

		int cols = (int)(width / cellSize);
		int rows = (int)(height / cellSize);

		for (int i = 0; i <= cols; i++)
		{
			float x = x0 + i * cellSize;
			DrawLine((int)x, (int)y0, (int)x, (int)(y0 + height), color);
		}

		for (int j = 0; j <= rows; j++)
		{
			float y = y0 + j * cellSize;
			DrawLine((int)x0, (int)y, (int)(x0 + width), (int)y, color);
		}
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
		// --- Constructors ---
		Managed() : value({}), loaded(false) {}

		// Construct directly from an existing Raylib object (e.g., LoadRenderTexture())
		Managed(const T& v) noexcept { value = v; loaded = true; }
		Managed(T&& v) noexcept { value = std::move(v); loaded = true; }

		// Disable copying (Raylib handles are unique)
		Managed(const Managed&) = delete;
		Managed& operator=(const Managed&) = delete;

		// Move support
		Managed(Managed&& other) noexcept {
			value = other.value;
			loaded = other.loaded;
			other.loaded = false;
			other.value = {};
		}

		Managed& operator=(Managed&& other) noexcept {
			if (this != &other) {
				if (loaded)
					Unload();
				value = other.value;
				loaded = other.loaded;
				other.loaded = false;
				other.value = {};
			}
			return *this;
		}

		// --- Load constructors ---
		Managed(const char* fileName) requires std::same_as<T, Image> { value = LoadImage(fileName); loaded = true; }

		Managed(const char* fileName) requires std::same_as<T, Texture2D> { value = LoadTexture(fileName); loaded = true; }
		Managed(const Image& img)     requires std::same_as<T, Texture2D> { value = LoadTextureFromImage(img); loaded = true; }

		Managed(int w, int h)         requires std::same_as<T, RenderTexture2D> { value = LoadRenderTexture(w, h); loaded = true; }

		Managed(const char* fileName) requires std::same_as<T, Font> { value = LoadFont(fileName); loaded = true; }
		Managed(const char* fileName, int size, int* cps, int count) requires std::same_as<T, Font> {
			value = LoadFontEx(fileName, size, cps, count); loaded = true;
		}

		Managed() requires std::same_as<T, Mesh> { value = GenMeshCube(1.0f, 1.0f, 1.0f); loaded = true; }
		Managed(float radius, int rings, int slices) requires std::same_as<T, Mesh> {
			value = GenMeshSphere(radius, rings, slices); loaded = true;
		}
		Managed(float width, float length, int resX, int resZ) requires std::same_as<T, Mesh> {
			value = GenMeshPlane(width, length, resX, resZ); loaded = true;
		}
		Managed(float base, float height, int slices, bool cone = false) requires std::same_as<T, Mesh> {
			value = cone ? GenMeshCone(base, height, slices) : GenMeshCylinder(base, height, slices);
			loaded = true;
		}
		Managed(int sides, float radius) requires std::same_as<T, Mesh> { value = GenMeshPoly(sides, radius); loaded = true; }

		Managed(const char* fileName) requires std::same_as<T, Model> { value = LoadModel(fileName); loaded = true; }
		Managed(const Mesh& mesh)     requires std::same_as<T, Model> { value = LoadModelFromMesh(mesh); loaded = true; }

		Managed(const char* vs, const char* fs) requires std::same_as<T, Shader> { value = LoadShader(vs, fs); loaded = true; }

		Managed(const char* fileName) requires std::same_as<T, Wave> { value = LoadWave(fileName); loaded = true; }
		Managed(const char* fileType, const unsigned char* data, int size) requires std::same_as<T, Wave> {
			value = LoadWaveFromMemory(fileType, data, size); loaded = true;
		}

		Managed(const char* fileName) requires std::same_as<T, Sound> { value = LoadSound(fileName); loaded = true; }
		Managed(const Wave& wave)     requires std::same_as<T, Sound> { value = LoadSoundFromWave(wave); loaded = true; }

		Managed(const char* fileName) requires std::same_as<T, Music> { value = LoadMusicStream(fileName); loaded = true; }
		Managed(const char* type, const unsigned char* data, int size) requires std::same_as<T, Music> {
			value = LoadMusicStreamFromMemory(type, data, size); loaded = true;
		}

		Managed(unsigned int sr, unsigned int ss, unsigned int ch) requires std::same_as<T, AudioStream> {
			value = LoadAudioStream(sr, ss, ch); loaded = true;
		}

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

		// --- Status ---
		bool IsLoaded() const { return loaded; }

		~Managed() { Unload(); }

	protected:
		T value{};
		bool loaded = false;
	};

	inline void BeginUpscaleRender(RenderTexture2D target, float scale = 1.0f)
	{
		BeginTextureMode(target);
		BeginMode2D(Camera2D{ .zoom = scale });
	}

	inline std::tuple<float, float, float, float> GetUpscaledTargetArea(int width, int height)
	{
		float windowW = (float)GetScreenWidth();
		float windowH = (float)GetScreenHeight();
		float scale = std::min(windowW / width, windowH / height);
		float renderW = width * scale;
		float renderH = height * scale;
		float offsetX = (windowW - renderW) * 0.5f;
		float offsetY = (windowH - renderH) * 0.5f;
		return { offsetX, offsetY, renderW, renderH };
	}

	inline void EndUpscaleRender(RenderTexture2D target, Color background = BLACK, std::function<void()> before = nullptr, std::function<void()> after = nullptr)
	{
		EndMode2D();
		EndTextureMode();
		auto [offsetX, offsetY, renderW, renderH] = GetUpscaledTargetArea(target.texture.width, target.texture.height);
		BeginDrawing();
		ClearBackground(background);
		if (before)
			before();
		DrawTexturePro(
			target.texture,
			{ 0, 0, (float)target.texture.width, -(float)target.texture.height },
			{ offsetX, offsetY, renderW, renderH },
			{ 0, 0 },
			0,
			WHITE
		);
		if (after)
			after();
		EndDrawing();
	}
}
namespace Core
{
	class Layer {
	public:
		Layer() = default;
		virtual ~Layer() = default;
		virtual void OnShow() {}
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
		virtual void OnRender_Before_Unscaled() {}
		virtual void OnRender_After_Unscaled() {}

		std::string Identifier = "";
	};

	class Window {
	public:
		~Window() {
			if (IsWindowReady())
				CloseWindow();
		}

		inline static std::unique_ptr<Window> Create(
			int width = 800,
			int height = 600,
			const std::string& title = "Raylib Window")
		{
			return std::make_unique<Window>(width, height, title);
		}

		void AddFlag(unsigned int flag) { flags |= flag; }
		void ClearFlags() { flags = 0; }

		void Show() {
			if (!IsWindowReady())
			{
#ifdef _WIN32
				SetProcessDPIAware();
#endif
				ApplyFlags();
				InitWindow(width, height, title.c_str());
			}
		}

		bool IsReady() const { return IsWindowReady(); }
		bool ShouldClose() const { return WindowShouldClose(); }

		void SetTitle(const std::string& newTitle) { SetWindowTitle(newTitle.c_str()); }
		void SetSize(int w, int h) { SetWindowSize(w, h); }
		void SetPosition(int x, int y) { SetWindowPosition(x, y); }
		void Focus() { SetWindowFocused(); }
		void Maximize() { MaximizeWindow(); }
		void Minimize() { MinimizeWindow(); }
		void Restore() { RestoreWindow(); }
		void ToggleFullscreen() { ::ToggleFullscreen(); }

		Vector2 GetPosition() const { return GetWindowPosition(); }
		Vector2 GetSize() const { return { (float)GetScreenWidth(), (float)GetScreenHeight() }; }

#ifdef _WIN32
		HWND GetHandle() const { return (HWND)GetWindowHandle(); }
#endif

		Window(int w, int h, const std::string& t)
			: width(w), height(h), title(t), flags(0) {
		}

	private:
		int width;
		int height;
		std::string title;
		unsigned int flags;

		void ApplyFlags() {
			if (flags != 0)
				SetConfigFlags(flags);
		}
	};

	class Application {
	public:
		bool UpscaleEnabled = false;
		uint8_t UpscaleFactor = 1;
		rlx::Managed<RenderTexture2D> UpscaleTexture{};
		Color ClearBackgroundColor = BLACK;
	public:
		static void InitializeComponents(
			int width = 800,
			int height = 600,
			const std::string& title = "Raylib Application")
		{
			auto& app = Instance();
			if (!app.window)
				app.window = Window::Create(width, height, title);
		}

		static Window* GetWindow() {
			return Instance().window.get();
		}

		static void Run(std::function<void()> loop = nullptr) {
			auto& app = Instance();

			if (!app.window)
				throw std::runtime_error("Application window does not exist.");

			if (!app.window->IsReady()) {
				app.window->Show();
				for (auto& [_, layer] : app.m_Layers)
					layer->OnShow();
			}

			while (app.window && !app.window->ShouldClose()) {
				if (loop) loop();
				else {
					for (auto& [_, layer] : app.m_Layers)
						layer->OnUpdate();

					if (app.UpscaleEnabled && app.UpscaleTexture.IsLoaded()) {
						rlx::BeginUpscaleRender(app.UpscaleTexture, (float)app.UpscaleFactor);
						ClearBackground(app.ClearBackgroundColor);
						for (auto& [_, layer] : app.m_Layers)
							layer->OnRender();
						rlx::EndUpscaleRender(app.UpscaleTexture, app.ClearBackgroundColor, [&]() {
								for (auto& [_, layer] : app.m_Layers)
									layer->OnRender_Before_Unscaled();
							},
							[&]() {
								for (auto& [_, layer] : app.m_Layers)
									layer->OnRender_After_Unscaled();
							});
					}
					else {
						BeginDrawing();
						ClearBackground(app.ClearBackgroundColor);
						for (auto& [_, layer] : app.m_Layers)
							layer->OnRender();
						EndDrawing();
					}
				}
			}
		}


		template<typename TLayer, typename... Args>
		static void Add(Args&&... args)
		{
			static_assert(std::is_base_of_v<Core::Layer, TLayer>,
				"TLayer must derive from Core::Layer");

			auto& app = Instance();
			uint32_t id = static_cast<uint32_t>(app.m_Layers.size());

			auto layer = std::make_unique<TLayer>(std::forward<Args>(args)...);
			if (layer->Identifier.empty())
				throw std::invalid_argument("Layer Identifier was not set.");

			for (auto& [key, existing] : app.m_Layers)
			{
				if (existing && existing->Identifier == layer->Identifier)
					throw std::invalid_argument("Duplicate layer identifier: " + layer->Identifier);
			}

			app.m_Layers.emplace(id, std::move(layer));
		}

		template<typename TLayer>
			requires(std::is_base_of_v<Core::Layer, TLayer>)
		static void Remove()
		{
			auto& app = Instance();

			for (auto it = app.m_Layers.begin(); it != app.m_Layers.end(); ++it)
			{
				if (dynamic_cast<TLayer*>(it->second.get()))
				{
					app.m_Layers.erase(it);
					break;
				}
			}
		}

		static rlRectangle GetUpscaledRenderArea()
		{
			auto& app = Instance();
			if (!app.UpscaleEnabled)
				return {0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight() };
			auto [offsetX, offsetY, renderW, renderH] = rlx::GetUpscaledTargetArea(app.UpscaleTexture->texture.width, app.UpscaleTexture->texture.height);
			return { offsetX, offsetY, renderW, renderH };
		}

		static Application& Instance() {
			static Application instance;
			return instance;
		}
	private:
		// Only allows single window for now
		std::unique_ptr<Window> window;

		ordered_map<uint32_t, std::unique_ptr<Layer>> m_Layers;

		Application() = default;
		~Application() = default;

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;
	};
}