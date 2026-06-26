#ifndef API_ID_H
#define API_ID_H
#include "CCSystem.h"

int GetID();
void ReleaseID(int id);
void ReleaseAllID();
CCVector<bool>  LookAllID();
bool CameraOnline(int id);
bool IsIDAllocated(int id);

#endif //API_ID_H
