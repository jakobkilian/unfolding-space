// TODO Draft meiner ProtectedData Klasse
#include "monitoring.hpp"

#include <iostream>
#include <mutex>
#include <string>

// public function to return an instance of this protecting wrapper
ProtectedData::ProtectingWrapper ProtectedData::access() {
  return ProtectingWrapper(_data, _mutex);
}

ProtectedData pData;