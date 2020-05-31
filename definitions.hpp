#pragma once
// This files contains global definitions

/*__________Protected Data Struct_________*/
#include "monitoring.hpp"
//the global instance of the protected data class
extern ProtectedData pData;
// Shortcut to access data struct in this global instance by PDATA
//#define PDATA pData.access().data