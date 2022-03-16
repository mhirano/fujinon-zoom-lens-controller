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


class ZoomLensController {
public:

	ZoomLensController(AppMsgPtr _appMsg):appMsg(_appMsg) {};

	bool run() {
//		const char *PORT = "COM1";
//		boost::asio::io_service io;
//		boost::asio::serial_port port(io, PORT);
//		port.set_option(boost::asio::serial_port_base::baud_rate(38400));
//		port.set_option(boost::asio::serial_port_base::character_size(8));
//		port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
//		port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
//		port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

        /*
         * Make sure to use "video iris mode"
         * Initialize zoom lens
         * - Set filter to "Filter Clear" (not cut visible light)
         * - Set to remote iris (to enable iris control)
         */

        // set filter to "Filter Clear"
//        port.write_some(boost::asio::buffer(generateCommand(0x40, {0xE0})));

        // set to remote iris
//        port.write_some(boost::asio::buffer(generateCommand(0x42, {0xDC})));


        while (true) {
			auto commandMsg = appMsg->zlcRequestMessenger->receive();
			if (commandMsg != nullptr) {
                unsigned char code = commandMsg->command.code;
                std::vector<unsigned char> data = commandMsg->command.data;

                /* SANITY CHECK */
                sanityCheck(code, data);

                /* SEND COMMAND */
                auto send_api_frame = generateCommand(code, data);
                for (auto i: send_api_frame) { std::cout << std::hex << (unsigned int)i << " "; }
                std::cout << std::dec << std::endl;
//                port.write_some(boost::asio::buffer(send_api_frame));

                /* RECEIVE COMMAND */
//                boost::array<unsigned char, 32> receive_api_frame;
//                receive_api_frame.assign(0xFF); // Assign 0xFF to all entries
//                size_t length = port.read_some( boost::asio::buffer(receive_api_frame) );
//
//                for (size_t i = 0; i < receive_api_frame.size() - 1; ++i) {
//                    std::cout << std::hex << static_cast<unsigned int>(receive_api_frame[i]) << " ";
//                }
//                std::cout << std::endl;
			}

            if (appMsg->zlcRequestMessenger->isClosed()) {
                printf("\n## termination requested ##\n");
                break;

            }
		}

		return true;
	};

	/*
	 * Generate command in C10 protocol
	 */
    static std::vector<unsigned char> generateCommand(unsigned char code, std::vector<unsigned char> data) {
        std::vector<unsigned char> api_frame;
        api_frame.push_back(data.size()); // append data length
        api_frame.push_back(code); // append function code
        for (auto i:data) { api_frame.push_back(i); } // append function data
        api_frame.push_back(checksum(api_frame)); // append checksum

        return api_frame;
    }

    /*
     * Sanity check
     */
    static void sanityCheck(unsigned char code, std::vector<unsigned char> data){
        switch (code) {
            case 0x20: /* Iris control (Position) */
                assert(data.size() == 2 && "Bad command to zoom lens");
                break;
            case 0x21: /* Zoom control (Position) */
                assert(data.size() == 2 && "Bad command to zoom lens");
                break;
            case 0x22: /* Focus control (Position) */
                assert(data.size() == 2 && "Bad command to zoom lens");
                break;
            case 0x40: /* filter control (VisCut/Clear) */
                assert(data.size() == 1 && "Bad command to zoom lens");
                break;
            case 0x42: /* Iris control (Auto/Remote) */
                assert(data.size() == 1 && "Bad command to zoom lens");
                break;
            default:
                assert(!"Not supported command.");
                break;
        }
    }

    /*
     * Compute checksum
     */
    static unsigned char checksum(std::vector<unsigned char> vec){
        unsigned char sum = 0x00;
        for(int i: vec) sum += i;
        return 0x100 - sum;
    }

private:

    AppMsgPtr appMsg;

};


/*
 * Helper class to use ZoomLensController
 */
class ZoomLensControllerUtil {
public:
    enum class ZOOM_LENS_FILTER{VISIBLE_LIGHT_CUT_FILTER=0, FILTER_CLEAR=1};
    enum class ZOOM_LENS_IRIS{AUTO=0, REMOTE=1};

    ZoomLensControllerUtil(AppMsgPtr _appMsg): appMsg(_appMsg){};

    /* iris by ratio (0: close end <--> 1: open end) */
    void iris(float ratio){
        ratio = std::clamp(ratio, 0.0f, 1.0f);
        int dataDec = std::ceil(ratio * 65535);
        unsigned char data1 = static_cast<unsigned char>(dataDec/256); // C10 protocol uses big endian
        unsigned char data2 = static_cast<unsigned char>(dataDec%256);

        auto md = appMsg->zlcRequestMessenger->prepareMsg();
        md->command.code = 0x20;
        md->command.data = {data1, data2};
        appMsg->zlcRequestMessenger->send();
    }

    /* Zoom by ratio (0: wide end <--> 1: tele end) */
    void zoom(float ratio){
        ratio = std::clamp(ratio, 0.0f, 1.0f);
        int dataDec = std::ceil(ratio * 65535);
        unsigned char data1 = static_cast<unsigned char>(dataDec/256); // C10 protocol uses big endian
        unsigned char data2 = static_cast<unsigned char>(dataDec%256);

        auto md = appMsg->zlcRequestMessenger->prepareMsg();
        md->command.code = 0x21;
        md->command.data = {data1, data2};
        appMsg->zlcRequestMessenger->send();
    }

    /* focus by ratio (0: MOD(minimum object distance) <--> 1: Inf) */
    void focus(float ratio){
        ratio = std::clamp(ratio, 0.0f, 1.0f);
        int dataDec = std::ceil(ratio * 65535);
        unsigned char data1 = static_cast<unsigned char>(dataDec/256); // C10 protocol uses big endian
        unsigned char data2 = static_cast<unsigned char>(dataDec%256);

        auto md = appMsg->zlcRequestMessenger->prepareMsg();
        md->command.code = 0x22;
        md->command.data = {data1, data2};
        appMsg->zlcRequestMessenger->send();
    }

    /* switch filter */
    void filter(ZOOM_LENS_FILTER filter){
        switch (filter) {
            case ZOOM_LENS_FILTER::VISIBLE_LIGHT_CUT_FILTER:
                command(0x40, {0xF0});
                break;
            case ZOOM_LENS_FILTER::FILTER_CLEAR:
                command(0x40, {0xE0});
                break;
            default:
                break;
        }
    }

    /* iris mode */
    void irisMode(ZOOM_LENS_IRIS irisMode){
        switch (irisMode) {
            case ZOOM_LENS_IRIS::AUTO:
                command(0x42, {0xCC});
                break;
            case ZOOM_LENS_IRIS::REMOTE:
                command(0x42, {0xDC});
                break;
            default:
                break;
        }

    }

    /* direct command */
    void command(uchar code, std::vector<uchar> data){
        ZoomLensController::sanityCheck(code, data);
        auto md = appMsg->zlcRequestMessenger->prepareMsg();
        md->command.code = code;
        md->command.data = data;
        appMsg->zlcRequestMessenger->send();
    }
private:


    AppMsgPtr appMsg;
};

using ZLC = ZoomLensController;
using ZLCUtil = ZoomLensControllerUtil;

/*
 * Test for ZoomLensController and ZoomLensControllerUtil
 */
class ZoomLensControllerTest {

public:

    ZoomLensControllerTest(AppMsgPtr _appMsg): appMsg(_appMsg){

    }

    bool run() {
        ZLCUtil zlcUtil(appMsg);
        /*
         * iris
         */
        zlcUtil.iris(0); // close end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 20 0 0 de (Expected output)" << std::endl;

        zlcUtil.iris(1); // open end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 20 ff ff e0 (Expected output)" << std::endl;

        /*
         * zoom
         */
        zlcUtil.command(0x21, {0x00, 0x00}); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 0 0 dd (Expected output)" << std::endl;

        zlcUtil.command(0x21, {0xFF, 0xFF}); // tele end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 ff ff df (Expected output)" << std::endl;

        zlcUtil.zoom(0); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 0 0 dd (Expected output)" << std::endl;

        zlcUtil.zoom(1); // tele end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 ff ff df (Expected output)" << std::endl;


        /*
         * focus
         */


        return true;
    }

private:

    AppMsgPtr appMsg;

};

#endif //ZOOM_LENS_CONTROLLER_H
