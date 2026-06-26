#include "basic/CC.h"

std::mutex CC::M_mutex_Print;
int CC::LogoTimeMS = 1;
std::function<bool()> CC::CloseFun;
bool CC::Exe_Run = true;
std::function<void(int)> CCAppInit_::Signal;
CCAppInit_ CCAppInit;