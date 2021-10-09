// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

#ifndef PCH_H
#define PCH_H

// TODO: add headers that you want to pre-compile here
#include <cstdint>
#include <cassert>

#ifdef _MSC_VER
	#define FINLINE __forceinline //still isn't forced, just prevents the cost/benifit analysis
#else
// I believe it's __attribute__((always_inline)) for gcc?
	#error	Need the Compiler's __forceinline alternative
#endif

//#define ROMSIZE 0x8000 //To be altered depending on the rom
//#define INSTANCES 1 //Amount of parallel instances
//#define UI //Whether to use a UI or not

#endif //PCH_H
