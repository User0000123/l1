#include <shellscalingapi.h>
#include "Model.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    SetProcessDPIAware();
    
    ModelStart(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}