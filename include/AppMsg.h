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
	bool isTerminationRequested = false;
};

class AppMsg{
public:
    AppMsg():
		displayMessenger(new InterThreadMessenger<DispMsg>),
		zlcMessenger(new InterThreadMessenger<ZLCMsg>)
	{};
	InterThreadMessenger<DispMsg>* displayMessenger;
	InterThreadMessenger<ZLCMsg>* zlcMessenger;

    void close(){
        displayMessenger->close();
		zlcMessenger->close();
    };
};

using AppMsgPtr = std::shared_ptr<AppMsg>;

#endif //ISLAY_APPMSG_H
