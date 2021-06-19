#pragma once
#include <royale.hpp>


class Listener : public royale::IDepthDataListener {
	public:
		int i;
		void onNewData(const royale::DepthData * data) override;

};