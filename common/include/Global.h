#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef _WIN32
	#ifdef DLL_EXPORT
		#define FLAW_API __declspec(dllexport)
	#else
		#define FLAW_API __declspec(dllimport)
	#endif
#else
	#ifdef DLL_EXPORT
		#define FLAW_API __attribute__((visibility("default")))
	#else
		#define FLAW_API
	#endif
#endif

#endif