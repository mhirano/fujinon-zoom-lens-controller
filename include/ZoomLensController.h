//
// Created by Masahiro Hirano <masahiro.dll@gmail.com>
//

#ifndef ZOOM_LENS_CONTROLLER_H
#define ZOOM_LENS_CONTROLLER_H

//#define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING

#include "AppMsg.h"

#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include "ZoomLensCommand.h"

class ZoomLensController {

	AppMsgPtr appMsg;

public:

	ZoomLensController(AppMsgPtr _appMsg):appMsg(_appMsg) {};

	bool run() {
		const char *PORT = "COM1";
		boost::asio::io_service io;
		boost::asio::serial_port port(io, PORT);
		port.set_option(boost::asio::serial_port_base::baud_rate(38400));
		port.set_option(boost::asio::serial_port_base::character_size(8));
		port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
		port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
		port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

		while (true) {
			auto md = appMsg->zlcMessenger->receive();
			if (md != nullptr) {
				for (int i = 0; i < 2; i++) {
					std::cout << std::hex << (unsigned int)md->command.zoom[i] << " ";
				}
				if (md->command.zoom[0] == 0x00 && md->command.zoom[1] == 0x00) {
					boost::array<unsigned char, 5> send_api_frame = { 0x02, 0x21, 0x00, 0x00, 0xDD }; // ワイド端
					port.write_some(boost::asio::buffer(send_api_frame));
				}
				else if (md->command.zoom[0] == 0xFF && md->command.zoom[1] == 0xFF) {
					boost::array<unsigned char, 5> send_api_frame = { 0x02, 0x21, 0xFF, 0xFF, 0xDF }; // テレ端
					port.write_some(boost::asio::buffer(send_api_frame));
				}
				else {
					std::cout << "currently not supported command" << std::endl;
				}


				if (md->isTerminationRequested) {
					printf("\n## termination requested ##\n");
					break;
				}
			}
		}
		return true;
	};
};

#endif //ZOOM_LENS_CONTROLLER_H
