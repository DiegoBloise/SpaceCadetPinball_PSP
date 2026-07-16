#ifndef PCH_H
#define PCH_H

#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <type_traits>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <string>
#include <thread>
#include <map>
#include <unordered_map>
#include <initializer_list>

#define SDL_MAIN_HANDLED
#include "SDL.h"
#include <SDL_mixer.h>

constexpr int MIX_INIT_MID_Proxy = MIX_INIT_MID;

typedef char* LPSTR;
typedef const char* LPCSTR;

constexpr char PathSeparator = '/';

#define assertm(exp, msg) assert(((void)msg, exp))

inline size_t pgm_save(int width, int height, char* data, FILE* outfile)
{
	size_t n = 0;
	n += fprintf(outfile, "P5\n%d %d\n%d\n", width, height, 0xFF);
	n += fwrite(data, 1, width * height, outfile);
	return n;
}

inline float RandFloat()
{
	return static_cast<float>(std::rand() / static_cast<double>(RAND_MAX));
}

template <typename T>
constexpr int Sign(T val)
{
	return (T(0) < val) - (val < T(0));
}

template <typename T>
const T& Clamp(const T& n, const T& lower, const T& upper)
{
	return std::max(lower, std::min(n, upper));
}

inline FILE* fopenu(const char* path, const char* opt)
{
	return fopen(path, opt);
}

constexpr const char* PlatformDataPaths[2] =
{
	"ms0:/PSP/GAME/SpaceCadetPinball/",
	"ms0:/PSP/GAME/SpaceCadetPinball/"
};

constexpr float Pi = 3.14159265358979323846f;

#endif //PCH_H
