//
// misc.h
//

void	DoAlert(const char* format, ...);
POMME_NORETURN void DoFatalAlert(const char* format, ...);
void	Wait(long);
POMME_NORETURN void CleanQuit(void);
void SetMyRandomSeed(unsigned long seed);
unsigned long MyRandomLong(void);
Handle	AllocHandle(long size);
void *AllocPtr(long size);
void *AllocPtrClear(long size);
float RandomFloat(void);
uint16_t	RandomRange(uint16_t min, uint16_t max);
void CalcFramesPerSecond(void);
Boolean IsPowerOf2(int num);
float RandomFloat2(void);
void SafeDisposePtr(void *ptr);
void MyFlushEvents(void);
