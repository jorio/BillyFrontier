//
// misc.h
//

void	DoAlert(const char* format, ...);
POMME_NORETURN void DoFatalAlert(const char* format, ...);
void	Wait(long);
POMME_NORETURN void CleanQuit(void);
void SetMyRandomSeed(uint32_t seed);
uint32_t MyRandomLong(void);
Handle	AllocHandle(long size);
void *AllocPtr(long size);
void *AllocPtrClear(long size);
void *ReallocPtr(void* initialPtr, long newSize);
void SafeDisposePtr(void *ptr);
float RandomFloat(void);
uint16_t	RandomRange(uint16_t min, uint16_t max);
void CalcFramesPerSecond(void);
Boolean IsPowerOf2(int num);
float RandomFloat2(void);
void MyFlushEvents(void);
