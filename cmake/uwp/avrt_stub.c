// TheSuperHackers @build githubawn 16/10/2026 avrt.dll (MMCSS thread priority) is not
// part of the UWP API set, so the Xbox loader would reject OpenAL32.dll if it imported
// it. OpenAL Soft only uses it to raise its mixer thread priority and carries on when
// the call fails, so report failure.

#include <windows.h>

static HANDLE WINAPI ggcAvSetMmThreadCharacteristicsW(LPCWSTR task, LPDWORD index)
{
	(void)task;
	(void)index;
	SetLastError(ERROR_NOT_SUPPORTED);
	return NULL;
}

static BOOL WINAPI ggcAvRevertMmThreadCharacteristics(HANDLE handle)
{
	(void)handle;
	return FALSE;
}

void *__imp_AvSetMmThreadCharacteristicsW = (void *)&ggcAvSetMmThreadCharacteristicsW;
void *__imp_AvRevertMmThreadCharacteristics = (void *)&ggcAvRevertMmThreadCharacteristics;
