#pragma once

#include "../vector_type.h"

VK2D_BEGIN

enum class Colors : uint32_t {
	Transparent          = 0x00000000, 
	WhiteTransparent     = 0x00ffffff, 
	Maroon               = 0xff000080,
	DarkRed              = 0xff00008b,
	Brown                = 0xff2a2aa5,
	Firebrick            = 0xff2222b2,
	Crimson              = 0xff3c14dc,
	Red                  = 0xff0000ff,
	Tomato               = 0xff4763ff,
	Coral                = 0xff507fff,
	IndianRed            = 0xff5c5ccd,
	LightCoral           = 0xff8080f0,
	DarkSalmon           = 0xff7a96e9,
	Salmon               = 0xff7280fa,
	LightSalmon          = 0xff7aa0ff,
	OrangeRed            = 0xff0045ff,
	DarkOrange           = 0xff008cff,
	Orange               = 0xff00a5ff,
	Gold                 = 0xff00d7ff,
	DarkGoldenRod        = 0xff0b86b8,
	GoldenRod            = 0xff20a5da,
	PaleGoldenRod        = 0xffaae8ee,
	DarkKhaki            = 0xff6bb7bd,
	Khaki                = 0xff8ce6f0,
	Olive                = 0xff008080,
	Yellow               = 0xff00ffff,
	YellowGreen          = 0xff32cd9a,
	DarkOliveGreen       = 0xff2f6b55,
	OliveDrab            = 0xff238e6b,
	LawnGreen            = 0xff00fc7c,
	ChartReuse           = 0xff00ff7f,
	GreenYellow          = 0xff2fffad,
	DarkGreen            = 0xff006400,
	Green                = 0xff008000,
	ForestGreen          = 0xff228b22,
	Lime                 = 0xff00ff00,
	LimeGreen            = 0xff32cd32,
	LightGreen           = 0xff90ee90,
	PaleGreen            = 0xff98fb98,
	DarkSeaGreen         = 0xff8fbc8f,
	MediumSpringGreen    = 0xff9afa00,
	SpringGreen          = 0xff7fff00,
	SeaGreen             = 0xff578b2e,
	MediumAquaMarine     = 0xffaacd66,
	MediumSeaGreen       = 0xff71b33c,
	LightSeaGreen        = 0xffaab220,
	DarkSlateGray        = 0xff4f4f2f,
	Teal                 = 0xff808000,
	DarkCyan             = 0xff8b8b00,
	Aqua                 = 0xffffff00,
	Cyan                 = 0xffffff00,
	LightCyan            = 0xffffffe0,
	DarkTurquoise        = 0xffd1ce00,
	Turquoise            = 0xffd0e040,
	MediumTurquoise      = 0xffccd148,
	PaleTurquoise        = 0xffeeeeaf,
	AquaMarine           = 0xffd4ff7f,
	PowderBlue           = 0xffe6e0b0,
	CadetBlue            = 0xffa09e5f,
	SteelBlue            = 0xffb48246,
	CornFlowerBlue       = 0xffed9564,
	DeepSkyBlue          = 0xffffbf00,
	DodgerBlue           = 0xffff901e,
	LightBlue            = 0xffe6d8ad,
	SkyBlue              = 0xffebce87,
	LightSkyBlue         = 0xffface87,
	MidnightBlue         = 0xff701919,
	Navy                 = 0xff800000,
	DarkBlue             = 0xff8b0000,
	MediumBlue           = 0xffcd0000,
	Blue                 = 0xffff0000,
	RoyalBlue            = 0xffe16941,
	BlueViolet           = 0xffe22b8a,
	Indigo               = 0xff82004b,
	DarkSlateBlue        = 0xff8b3d48,
	SlateBlue            = 0xffcd5a6a,
	MediumSlateBlue      = 0xffee687b,
	MediumPurple         = 0xffdb7093,
	DarkMagenta          = 0xff8b008b,
	DarkViolet           = 0xffd30094,
	DarkOrchid           = 0xffcc3299,
	MediumOrchid         = 0xffd355ba,
	Purple               = 0xff800080,
	Thistle              = 0xffd8bfd8,
	Plum                 = 0xffdda0dd,
	Violet               = 0xffee82ee,
	Magenta              = 0xffff00ff,
	Orchid               = 0xffd670da,
	MediumVioletRed      = 0xff8515c7,
	PaleCioletRed        = 0xff9370db,
	DeepPink             = 0xff9314ff,
	HotPink              = 0xffb469ff,
	LightPink            = 0xffc1b6ff,
	Pink                 = 0xffcbc0ff,
	AntiqueWhite         = 0xffd7ebfa,
	Beige                = 0xffdcf5f5,
	Bisque               = 0xffc4e4ff,
	BlanchedAlmond       = 0xffcdebff,
	Wheat                = 0xffb3def5,
	CornSilk             = 0xffdcf8ff,
	LemonChiffon         = 0xffcdfaff,
	LightGoldenRodYellow = 0xffd2fafa,
	LightYellow          = 0xffe0ffff,
	SaddleBrown          = 0xff13458b,
	Sienna               = 0xff2d52a0,
	Chocolate            = 0xff1e69d2,
	Peru                 = 0xff3f85cd,
	SandyBrown           = 0xff60a4f4,
	BurlyWood            = 0xff87b8de,
	Tan                  = 0xff8cb4d2,
	RosyBrown            = 0xff8f8fbc,
	Moccasin             = 0xffb5e4ff,
	NavajoWhite          = 0xffaddeff,
	PeachPuff            = 0xffb9daff,
	MistyRose            = 0xffe1e4ff,
	LavenderBlush        = 0xfff5f0ff,
	Linen                = 0xffe6f0fa,
	OldLace              = 0xffe6f5fd,
	PapayaWhip           = 0xffd5efff,
	SeaShell             = 0xffeef5ff,
	MintCream            = 0xfffafff5,
	SlateGray            = 0xff908070,
	LightSlateGray       = 0xff998877,
	LightSteelBlue       = 0xffdec4b0,
	Lavender             = 0xfffae6e6,
	FloralWhite          = 0xfff0faff,
	AliceBlue            = 0xfffff8f0,
	GhostWhite           = 0xfffff8f8,
	Honeydew             = 0xfff0fff0,
	Ivory                = 0xfff0ffff,
	Azure                = 0xfffffff0,
	Snow                 = 0xfffafaff,
	Black                = 0xff000000,
	DimGray              = 0xff696969,
	Gray                 = 0xff808080,
	DarkGray             = 0xffa9a9a9,
	Silver               = 0xffc0c0c0,
	LightGray            = 0xffd3d3d3,
	Gainsboro            = 0xffdcdcdc,
	WhiteSmoke           = 0xfff5f5f5,
	White                = 0xffffffff
};

struct Color {
	VK2D_INLINE Color() VK2D_NOTHROW = default;

	VK2D_INLINE Color(Colors color) VK2D_NOTHROW {
		*reinterpret_cast<Colors*>(this) = color;
	}

	VK2D_INLINE explicit Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) VK2D_NOTHROW :
		r(r),
		g(g),
		b(b),
		a(a) {}

	VK2D_INLINE explicit Color(int32_t r, int32_t g, int32_t b, int32_t a = 255) VK2D_NOTHROW :
		r((uint8_t)r),
		g((uint8_t)g),
		b((uint8_t)b),
		a((uint8_t)a) {}

	VK2D_INLINE explicit Color(float r, float g, float b, float a = 1.f) VK2D_NOTHROW :
		r(static_cast<uint8_t>(255.99f * r)),
		g(static_cast<uint8_t>(255.99f * g)),
		b(static_cast<uint8_t>(255.99f * b)),
		a(static_cast<uint8_t>(255.99f * a)) {}

	VK2D_INLINE Color(const vec3& color) VK2D_NOTHROW :
		r(static_cast<uint8_t>(255.99f * color.r)),
		g(static_cast<uint8_t>(255.99f * color.g)),
		b(static_cast<uint8_t>(255.99f * color.b)),
		a(255) {}

	VK2D_INLINE Color(const vec4& color) VK2D_NOTHROW :
		r(static_cast<uint8_t>(255.99f * color.r)),
		g(static_cast<uint8_t>(255.99f * color.g)),
		b(static_cast<uint8_t>(255.99f * color.b)),
		a(static_cast<uint8_t>(255.99f * color.a)) {}

	VK2D_INLINE vec4 to_vec4() const VK2D_NOTHROW {
		return {
			static_cast<float>(r) / 255.f,
			static_cast<float>(g) / 255.f,
			static_cast<float>(b) / 255.f,
			static_cast<float>(a) / 255.f
		};
	}

	VK2D_INLINE vec3 to_vec3() const VK2D_NOTHROW {
		return {
			static_cast<float>(r) / 255.f,
			static_cast<float>(g) / 255.f,
			static_cast<float>(b) / 255.f
		};
	}

	VK2D_INLINE bool operator==(const Color& rhs) const VK2D_NOTHROW {
		return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
	}

	VK2D_INLINE bool operator!=(const Color& rhs) const VK2D_NOTHROW {
		return !(*this == rhs);
	}

	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

VK2D_END