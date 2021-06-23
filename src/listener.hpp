#pragma once
#include "q.h"
#include <royale.hpp>

class Listener : public royale::IDepthDataListener {
public:
  Listener(Q<royale::DepthData *> * _q) {
	  this->q = _q;
  }
  void onNewData(const royale::DepthData *data) override;

private:
  Q<royale::DepthData *> * q;
};