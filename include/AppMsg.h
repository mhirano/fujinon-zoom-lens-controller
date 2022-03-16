//
// Created by Masahiro Hirano <masahiro.dll@gmail.com>
//

#ifndef ISLAY_APPMSG_H
#define ISLAY_APPMSG_H

#include <opencv2/opencv.hpp>
#include <map>
#include <memory>
#include "InterThreadMessenger.hpp"

#include "ZoomLensCommand.h"

struct DispMsg : public MsgData {
    std::map<std::string, cv::Mat> pool;
};

// Zoom lens controller message
struct ZLCMsg : public MsgData {
	ZLCCommand command;
};

class AppMsg{
public:
    AppMsg():
			displayMessenger(new InterThreadMessenger<DispMsg>),
			zlcRequestMessenger(new InterThreadMessenger<ZLCMsg>),
			zlcResponseMessenger(new InterThreadMessenger<ZLCMsg>){};

	InterThreadMessenger<DispMsg>* displayMessenger;
	InterThreadMessenger<ZLCMsg>* zlcRequestMessenger;
	InterThreadMessenger<ZLCMsg>* zlcResponseMessenger;

    void close(){
        displayMessenger->close();
		zlcRequestMessenger->close();
		zlcResponseMessenger->close();
    };
};

using AppMsgPtr = std::shared_ptr<AppMsg>;

#endif //ISLAY_APPMSG_H
