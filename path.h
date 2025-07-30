#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER
#define VERSION "Version 0.0.1\n Archit Mishra 2025\nGPL-3.0"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

namespace Colors {
struct Reset {};          
struct Bold {};           
struct Dim {};            
struct Italic {};         
struct Underline {};      
struct Blink {};          
struct RapidBlink {};     
struct Reverse {};        
struct Conceal {};        
struct Crossed {};        
struct DoubleUnderline {};
struct Overline {};       
struct Framed {};         
struct Encircled {};      
struct Strikethrough {};  
struct Hidden {};         
struct SlowBlink {};      

// ─────────────────────────────────────────────────────────────────────────────
//  Color types
// ─────────────────────────────────────────────────────────────────────────────
//  True color (24-bit) templates
template <int R, int G, int B> struct FgRGB {};
template <int R, int G, int B> struct BgRGB {};

// ─────────────────────────────────────────────────────────────────────────────
//  Named 20 base colors (ANSI 8 + 12 custom via RGB) and bright variants
// ─────────────────────────────────────────────────────────────────────────────
enum class Color16 : int {
  Black = 30,
  Red = 31,
  Green = 32,
  Yellow = 33,
  Blue = 34,
  Magenta = 35,
  Cyan = 36,
  White = 37,
  BrightBlack = 90,
  BrightRed = 91,
  BrightGreen = 92,
  BrightYellow = 93,
  BrightBlue = 94,
  BrightMagenta = 95,
  BrightCyan = 96,
  BrightWhite = 97,
};

// ─────────────────────────────────────────────────────────────────────────────
//  Output operator overloads
// ─────────────────────────────────────────────────────────────────────────────
inline std::ostream &operator<<(std::ostream &os, Reset) {
  return os << "\033[0m";
}
inline std::ostream &operator<<(std::ostream &os, Bold) {
  return os << "\033[1m";
}
inline std::ostream &operator<<(std::ostream &os, Dim) {
  return os << "\033[2m";
}
inline std::ostream &operator<<(std::ostream &os, Italic) {
  return os << "\033[3m";
}
inline std::ostream &operator<<(std::ostream &os, Underline) {
  return os << "\033[4m";
}
inline std::ostream &operator<<(std::ostream &os, Blink) {
  return os << "\033[5m";
}
inline std::ostream &operator<<(std::ostream &os, RapidBlink) {
  return os << "\033[6m";
}
inline std::ostream &operator<<(std::ostream &os, Reverse) {
  return os << "\033[7m";
}
inline std::ostream &operator<<(std::ostream &os, Conceal) {
  return os << "\033[8m";
}
inline std::ostream &operator<<(std::ostream &os, Crossed) {
  return os << "\033[9m";
}
inline std::ostream &operator<<(std::ostream &os, DoubleUnderline) {
  return os << "\033[21m";
}
inline std::ostream &operator<<(std::ostream &os, Overline) {
  return os << "\033[53m";
}
inline std::ostream &operator<<(std::ostream &os, Framed) {
  return os << "\033[51m";
}
inline std::ostream &operator<<(std::ostream &os, Encircled) {
  return os << "\033[52m";
}
inline std::ostream &operator<<(std::ostream &os, Strikethrough) {
  return os << "\033[9m";
}
inline std::ostream &operator<<(std::ostream &os, Hidden) {
  return os << "\033[8m";
}
inline std::ostream &operator<<(std::ostream &os, SlowBlink) {
  return os << "\033[5m";
}

inline std::ostream &operator<<(std::ostream &os, Color16 fg) {
  return os << "\033[" << static_cast<int>(fg) << "m";
}

template <int R, int G, int B>
inline std::ostream &operator<<(std::ostream &os, FgRGB<R, G, B>) {
  return os << "\033[38;2;" << R << ";" << G << ";" << B << "m";
}

template <int R, int G, int B>
inline std::ostream &operator<<(std::ostream &os, BgRGB<R, G, B>) {
  return os << "\033[48;2;" << R << ";" << G << ";" << B << "m";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pre-instantiated manipulators for ease of use
// ─────────────────────────────────────────────────────────────────────────────
inline Reset reset;
inline Bold bold;
inline Dim dim;
inline Italic italic;
inline Underline underline;
inline Blink blink;
inline RapidBlink rapid_blink;
inline Reverse reverse;
inline Conceal conceal;
inline Crossed crossed;
inline DoubleUnderline double_underline;
inline Overline overline;
inline Framed framed;
inline Encircled encircled;
inline Strikethrough strikethrough;
inline Hidden hidden;
inline SlowBlink slow_blink;

// ─────────────────────────────────────────────────────────────────────────────
//  Named text colors
// ─────────────────────────────────────────────────────────────────────────────
namespace text {
inline Color16 black{Color16::Black};
inline Color16 red{Color16::Red};
inline Color16 green{Color16::Green};
inline Color16 yellow{Color16::Yellow};
inline Color16 blue{Color16::Blue};
inline Color16 magenta{Color16::Magenta};
inline Color16 cyan{Color16::Cyan};
inline Color16 white{Color16::White};
inline Color16 bright_black{Color16::BrightBlack};
inline Color16 bright_red{Color16::BrightRed};
inline Color16 bright_green{Color16::BrightGreen};
inline Color16 bright_yellow{Color16::BrightYellow};
inline Color16 bright_blue{Color16::BrightBlue};
inline Color16 bright_magenta{Color16::BrightMagenta};
inline Color16 bright_cyan{Color16::BrightCyan};
inline Color16 bright_white{Color16::BrightWhite};
// 12 custom RGB colors
template <int R, int G, int B> constexpr FgRGB<R, G, B> rgb() { return {}; }
inline auto orange = rgb<255, 165, 0>();
inline auto pink = rgb<255, 192, 203>();
inline auto purple = rgb<128, 0, 128>();
inline auto teal = rgb<0, 128, 128>();
inline auto brown = rgb<165, 42, 42>();
inline auto lime = rgb<0, 255, 0>();
inline auto navy = rgb<0, 0, 128>();
inline auto olive = rgb<128, 128, 0>();
inline auto maroon = rgb<128, 0, 0>();
inline auto aqua = rgb<0, 255, 255>();
inline auto silver = rgb<192, 192, 192>();
inline auto gold = rgb<255, 215, 0>();
inline auto turquoise = rgb<64,224,208>();
inline auto hotpink   = rgb<255,105,180>();
} // namespace text

// ─────────────────────────────────────────────────────────────────────────────
//  Named background colors
// ─────────────────────────────────────────────────────────────────────────────
namespace bg {
inline Color16 black{static_cast<Color16>(40)};
inline Color16 red{static_cast<Color16>(41)};
inline Color16 green{static_cast<Color16>(42)};
inline Color16 yellow{static_cast<Color16>(43)};
inline Color16 blue{static_cast<Color16>(44)};
inline Color16 magenta{static_cast<Color16>(45)};
inline Color16 cyan{static_cast<Color16>(46)};
inline Color16 white{static_cast<Color16>(47)};
inline Color16 bright_black{static_cast<Color16>(100)};
inline Color16 bright_red{static_cast<Color16>(101)};
inline Color16 bright_green{static_cast<Color16>(102)};
inline Color16 bright_yellow{static_cast<Color16>(103)};
inline Color16 bright_blue{static_cast<Color16>(104)};
inline Color16 bright_magenta{static_cast<Color16>(105)};
inline Color16 bright_cyan{static_cast<Color16>(106)};
inline Color16 bright_white{static_cast<Color16>(107)};
// 12 custom RGB backgrounds
template <int R, int G, int B> constexpr BgRGB<R, G, B> rgb() { return {}; }
inline auto bg_orange = rgb<255, 165, 0>();
inline auto bg_pink = rgb<255, 192, 203>();
inline auto bg_purple = rgb<128, 0, 128>();
inline auto bg_teal = rgb<0, 128, 128>();
inline auto bg_brown = rgb<165, 42, 42>();
inline auto bg_lime = rgb<0, 255, 0>();
inline auto bg_navy = rgb<0, 0, 128>();
inline auto bg_olive = rgb<128, 128, 0>();
inline auto bg_maroon = rgb<128, 0, 0>();
inline auto bg_aqua = rgb<0, 255, 255>();
inline auto bg_silver = rgb<192, 192, 192>();
inline auto bg_gold = rgb<255, 215, 0>();
} // namespace bg

} // namespace Colors
