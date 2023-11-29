#ifndef DLLUTILS_H
#define DLLUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#pragma once

// Disable warning messages about strcpy
#pragma warning(disable : 4996)
#pragma warning(disable : 4133)


//Directives for Windows PC and UNIX platforms
#ifdef _WIN32
   #include <windows.h>
   //#define STDCALL __stdcall // Remember to use this __stdcall in VC++ 6.0
   #define STDCALL __cdecl   // Remember to select this __cdecl in VS 2008
   // windows platforms has __int64 type - 64-bit integer
#else
   #include <dlfcn.h>
   #define STDCALL 
   // unix-family platforms doesn't support __int64, therefore need to define it here
   typedef long long __int64;
#endif


// Use 64-bit integer for fortran addresses
typedef __int64 fAddr;

#ifndef _WIN32
   // Macro for finding max, min
   //#define max(x, y) ((x) > (y) ? (x) : (y))
   //#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

static inline int min_i(int x, int y) { return x < y ? x : y; }
static inline int max_i(int x, int y) { return x > y ? x : y; }

static inline double min_d(double x, double y) { return x < y ? x : y; }
static inline double max_d(double x, double y) { return x > y ? x : y; }

// Generic function pointer for all Get functions
typedef void (STDCALL *fnPtrGetKey32Field)(int key, int fieldIdx, char strValue[512]);
typedef void (STDCALL *fnPtrGetKey64Field)(__int64 key, int fieldIdx, char strValue[512]);
typedef void (STDCALL *fnPtrGetField)(int fieldIdx, char strValue[512]);



#define PI        3.1415926535897932384
#define MPD       1440.0
#define MSPERDAY  86400000
#define TODEG     180.0 / PI
#define TORAD     PI / 180.0

// Command line arguments structure
typedef struct
{
   char  inFile[512];         // Input file name from command line
   char  outFile[512];        // Output file name from command line
   char  logFile[512];        // Log file name from command line
   char  bConst[7];           // Optional earth constant from command line   
} CommandLineArgs;


void   GetCommandLineArgs(int argc, char* argv[], const char* exeName, const char* appName, CommandLineArgs* cla);
void*  LoadLib(const char* dllName);
void*  GetFnPtr(void* hLib, char* fName);
char*  ToStrLower(char* inStr);
void   FreeLib(void* hLib, const char* dllName);
FILE*  FileOpen(char* fileName, char* mode);
void   Rtrim(char* fileName);
char*  GetOneJob(FILE* inputFilePtr, int* isEndAll);
void   PrintWarning(FILE* fp, const char* softwareName);
double DaysInWholeMillisec(double ds50);






#endif
