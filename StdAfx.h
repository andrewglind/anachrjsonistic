#ifndef STDAFX_JSON_H
#define STDAFX_JSON_H

#if __cplusplus < 201103L
	#if defined(_MSC_VER)
		typedef unsigned __int8 uint8_t;
		typedef unsigned __int16 uint16_t;
		typedef unsigned __int32 uint32_t;
	#else
		typedef unsigned char uint8_t;
		typedef unsigned short uint16_t;
		typedef unsigned int uint32_t;
	#endif
#else
	#include <stdint.h>
#endif

#ifdef _MSC_VER
	#pragma warning(disable: 4018)
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include <string>
#include <vector>
#include <map>

#if defined(_MSC_VER)
	#define HATCHET_EXPORT extern "C" __declspec(dllexport)
	#define HATCHET_CALL   __cdecl
	#define HATCHET_CLASS  __declspec(dllexport)
#elif defined(__GNUC__)
	#define HATCHET_EXPORT extern "C" __attribute__((visibility("default")))
	#define HATCHET_CALL
	#define HATCHET_CLASS  __attribute__((visibility("default")))
#else
	#define HATCHET_EXPORT extern "C"
	#define HATCHET_CALL
	#define HATCHET_CLASS
#endif

#endif
