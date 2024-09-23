#ifndef AWH_GLOBAL_H
#define AWH_GLOBAL_H

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#define DECL_EXPORT __declspec(dllexport)
	#define DECL_IMPORT __declspec(dllimport)
#else
	#define DECL_EXPORT __attribute__((visibility("default")))
	#define DECL_IMPORT __attribute__((visibility("default")))
#endif

#if defined(AWH_SHARED_LIBRARY_EXPORT)
	#define AWHSHARED_EXPORT DECL_EXPORT
#elif defined(ACU_SHARED_LIBRARY_IMPORT)
	#define AWHSHARED_EXPORT DECL_IMPORT
#else
	#define AWHSHARED_EXPORT
#endif

#endif // AWH_GLOBAL_H
