#define EXPORT __declspec(dllimport)
#include "windisp.h"

int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved);
EXPORT BOOL CALLBACK VT100Rx(WindowDisplay* windisp, char value);
