#include "xinput.h"
#include <string>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#ifndef PAGESIZE
#define PAGESIZE 4096
#endif
using namespace std;

struct __attribute__((packed)) Sjmp
{
	unsigned char op;
	void* value;

	Sjmp(void* value):
		op(0xE9), value((void*)((long)value-(long)&op-5))
	{
		/*
		 This JITs a X86 jmp instruction
		 */
	}
};


extern "C" void *wine_dll_load( const char *filename, char *error, int errorsize, int *file_exists )
{
	/*
	 This is a wine intern function,
	 we get control of this function via LD_PRELOAD.

	 We check the filenames and hook some functions ;)
	*/

	//call original function:
	void* result = ((decltype(&wine_dll_load))dlsym(RTLD_NEXT, "wine_dll_load"))(filename, error, errorsize, file_exists);

	//check for dlls
	if (string("xinput1_3.dll") == filename)
	{
		long addr = 0;
		pair<string, void*> list[] =
		{
			{"XInputEnable"                    , (void*)&XInputEnable},
			{"XInputGetAudioDeviceIds"         , (void*)&XInputGetAudioDeviceIds},
			{"XInputGetBatteryInformation"     , (void*)&XInputGetBatteryInformation},
			{"XInputGetCapabilities"           , (void*)&XInputGetCapabilities},
			{"XInputGetDSoundAudioDeviceGuids" , (void*)&XInputGetDSoundAudioDeviceGuids},
			{"XInputGetKeystroke"              , (void*)&XInputGetKeystroke},
			{"XInputGetState"                  , (void*)&XInputGetState},
			{"XInputSetState"                  , (void*)&XInputSetState}
		};
		//hook functions
		for(int i = 0; i < 7; ++i)
		{
			addr = long(dlsym(result, list[i].first.c_str()));
			if (addr != 0)
			{
				long addr_start = (addr - PAGESIZE-1) & ~(PAGESIZE-1);
				long addr_end   = (addr + PAGESIZE-1) & ~(PAGESIZE-1);
				mprotect((void*)addr_start, addr_end-addr_start, PROT_READ|PROT_WRITE|PROT_EXEC);
				new ((void*)addr) Sjmp(list[i].second);
			}
		}
	}
}
