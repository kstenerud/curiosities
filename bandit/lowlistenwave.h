//lowlistenwave.h
//GROUP MEMBERS:
//       Rob Blasutig
//       Michael Gray
//       Cam Mitchner
//       Karl Stenerud
//       Chris Torrens
#ifndef _LOWLISTENWAVE_H
#define _LOWLISTENWAVE_H

bool TestOpenOutputDevice(HWND hWnd, WAVEFORMATEX wfx);
void StartPlayBackTest(HWND hWnd);
void AddInitialBuffersToQueue(HWND hWnd);
void StopPlayBackTest(HWND hWnd);
void AllocListenBuffers();
void CleanUpListenBuffers();

#endif
