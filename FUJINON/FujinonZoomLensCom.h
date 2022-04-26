//
// Created by Masahiro Hirano <masahiro.dll@gmail.com>
//

#ifndef FUJINON_ZOOM_LENS_COM_H
#define FUJINON_ZOOM_LENS_COM_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "FujinonZoomLens.h"
#include "AppMsg.h"

class FujinonZoomLensClient: public FujinonZoomLensClientTemplate {
	AppMsgPtr appMsg;
public:
	FujinonZoomLensClient(AppMsgPtr _appMsg) : appMsg(_appMsg) {};

	void send(FujinonZoomLensCommand cmd) override {
		/* Implement here */
		auto md = appMsg->zlcRequestMessenger->prepareMsg();
		md->code = cmd.code;
		md->data = cmd.data;
		appMsg->zlcRequestMessenger->send();
		std::cout << "sendinf from FujinonZoomLensClient" << std::endl;
	}
};



class FujinonZoomLensServer {
	boost::asio::io_service io;
	boost::asio::serial_port port;
	boost::array<uchar, 32> receive_api_frame;
	size_t length;


public:
	FujinonZoomLensServer(const char *PORT = "COM1") : port(boost::asio::serial_port(io, PORT))
	{
		initialize();
	}

	void initialize() {
		port.set_option(boost::asio::serial_port_base::baud_rate(38400));
		port.set_option(boost::asio::serial_port_base::character_size(8));
		port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
		port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
		port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

		/*
		 * Make sure to use "video iris mode"
		 * Initialize zoom lens
		 * - Set filter to "Filter Clear" (not cut visible light)
		 * - Set to remote iris (to enable iris control)
		 */
		port.write_some(boost::asio::buffer(FujinonZoomLensControllerUtil::encodeCommand(0x40, { 0xE0 }))); // set filter to "Filter Clear"
		receive_api_frame.assign(0xFF); // Assign 0xFF to all entries
		length = port.read_some(boost::asio::buffer(receive_api_frame));
		FujinonZoomLensControllerUtil::decodeCommand(receive_api_frame);

		port.write_some(boost::asio::buffer(FujinonZoomLensControllerUtil::encodeCommand(0x42, { 0xDC }))); // set to remote iris
		receive_api_frame.assign(0xFF); // Assign 0xFF to all entries
		length = port.read_some(boost::asio::buffer(receive_api_frame));
		FujinonZoomLensControllerUtil::decodeCommand(receive_api_frame);

		port.write_some(boost::asio::buffer(FujinonZoomLensControllerUtil::encodeCommand(0x21, { 0x00, 0x00 }))); // set zoom to wide end
		receive_api_frame.assign(0xFF); // Assign 0xFF to all entries
		length = port.read_some(boost::asio::buffer(receive_api_frame));
		FujinonZoomLensControllerUtil::decodeCommand(receive_api_frame);

		port.write_some(boost::asio::buffer(FujinonZoomLensControllerUtil::encodeCommand(0x20, { 0xFF, 0xFF }))); // set iris to open
		receive_api_frame.assign(0xFF); // Assign 0xFF to all entries
		length = port.read_some(boost::asio::buffer(receive_api_frame));
		FujinonZoomLensControllerUtil::decodeCommand(receive_api_frame);
	}

	void runCommand(FujinonZoomLensCommand cmd) {
		uchar code = cmd.code;
		std::vector<uchar> data = cmd.data;

		/* SANITY CHECK */
		FujinonZoomLensControllerUtil::sanityCheck(code, data);

		/* SEND COMMAND */
		auto send_api_frame = FujinonZoomLensControllerUtil::encodeCommand(code, data);
		for (auto i : send_api_frame) { std::cout << std::hex << (uint)i << " "; }
		std::cout << std::dec << std::endl;
		port.write_some(boost::asio::buffer(send_api_frame));

		/* RECEIVE COMMAND */
		receive_api_frame.assign(0xFF); // Assign 0xFF to all entries
		length = port.read_some(boost::asio::buffer(receive_api_frame));
		FujinonZoomLensControllerUtil::decodeCommand(receive_api_frame);
	}

};

#endif //FUJINON_ZOOM_LENS_COM_H
