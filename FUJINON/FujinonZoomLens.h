//
// Created by Masahiro Hirano <masahiro.dll@gmail.com>
//

#ifndef ZOOM_LENS_CONTROLLER_H
#define ZOOM_LENS_CONTROLLER_H


#include "AppMsg.h"

#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>

class ZoomLensController {
public:

	ZoomLensController(AppMsgPtr _appMsg) :appMsg(_appMsg) {};

	bool run() {
		const char *PORT = "COM1";
		boost::asio::io_service io;
		boost::asio::serial_port port(io, PORT);
		port.set_option(boost::asio::serial_port_base::baud_rate(38400));
		port.set_option(boost::asio::serial_port_base::character_size(8));
		port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
		port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
		port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

		// frames
		boost::array<uchar, 32> receive_api_frame;
		size_t length;

        /*
         * Make sure to use "video iris mode"
         * Initialize zoom lens
         * - Set filter to "Filter Clear" (not cut visible light)
         * - Set to remote iris (to enable iris control)
         */
	    port.write_some(boost::asio::buffer(encodeCommand(0x40, {0xE0}))); // set filter to "Filter Clear"
		receive_api_frame.assign(0xFF); // Assign 0xFF to all entries
		length = port.read_some(boost::asio::buffer(receive_api_frame));
		decodeCommand(receive_api_frame);

		port.write_some(boost::asio::buffer(encodeCommand(0x42, {0xDC}))); // set to remote iris
		receive_api_frame.assign(0xFF); // Assign 0xFF to all entries
		length = port.read_some(boost::asio::buffer(receive_api_frame));
		decodeCommand(receive_api_frame);

		port.write_some(boost::asio::buffer(encodeCommand(0x21, { 0x00, 0x00}))); // set zoom to wide end
		receive_api_frame.assign(0xFF); // Assign 0xFF to all entries
		length = port.read_some(boost::asio::buffer(receive_api_frame));
		decodeCommand(receive_api_frame);

		port.write_some(boost::asio::buffer(encodeCommand(0x20, { 0xFF, 0xFF }))); // set iris to open
		receive_api_frame.assign(0xFF); // Assign 0xFF to all entries
		length = port.read_some(boost::asio::buffer(receive_api_frame));
		decodeCommand(receive_api_frame);

        /*
         * Main loop
         */
        while (true) {
			auto commandMsg = appMsg->zlcRequestMessenger->receive();
			if (commandMsg != nullptr) {
                uchar code = commandMsg->code;
                std::vector<uchar> data = commandMsg->data;

                /* SANITY CHECK */
                sanityCheck(code, data);

                /* SEND COMMAND */
                auto send_api_frame = encodeCommand(code, data);
                for (auto i: send_api_frame) { std::cout << std::hex << (uint)i << " "; }
                std::cout << std::dec << std::endl;
                port.write_some(boost::asio::buffer(send_api_frame));

                /* RECEIVE COMMAND */
                receive_api_frame.assign(0xFF); // Assign 0xFF to all entries
                length = port.read_some( boost::asio::buffer(receive_api_frame) );
				decodeCommand(receive_api_frame);
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
    static std::vector<uchar> encodeCommand(uchar code, std::vector<uchar> data) {
        std::vector<uchar> api_frame;
        api_frame.push_back(data.size()); // append data length
        api_frame.push_back(code); // append function code
        for (auto i:data) { api_frame.push_back(i); } // append function data
        api_frame.push_back(checksum(api_frame)); // append checksum
	
        return api_frame;
    }

	/*
	 * Decode command in C10 protocol
	 */
	bool decodeCommand(const boost::array<uchar, 32> api_frame) {
		// print frame
		for (size_t i = 0; i < api_frame.size() - 1; ++i) {
			std::cout << std::hex << static_cast<uint>(api_frame[i]) << " ";
		}
		std::cout << std::dec << std::endl;

		// retrieve data part
		size_t length = static_cast<uint>(api_frame[0]);
		uchar code = api_frame[1];
		std::vector<uchar> data;
		for (size_t i = 1; i <= length; i++) {
			data.push_back(api_frame[1 + i]);
		}
		uchar checksum_received = api_frame[2 + length];

		// checksum
		std::vector<uchar> data_for_checksum = data;
		data_for_checksum.push_back(api_frame[0]);
		data_for_checksum.push_back(api_frame[1]);
		if (!(checksum_received == checksum(data_for_checksum))) {
			std::cout << "Checksum failed" << std::endl;
			return false;
		}

		// decode data
		if (code == 0x11) { // Get second half of name
			std::cout << "Name (first half): ";
			for (int i = 0; i < length; i++) {
				std::cout << static_cast<char>(data[i]);
			}
			std::cout << std::endl;
		}
		else if (code == 0x12) { // Get first half of name
			std::cout << "Name (second half): "; 
			for (int i = 0; i < length; i++) {
				std::cout << static_cast<char>(data[i]);
			}
			std::cout << std::endl;
		}
		else if (code == 0x17) { // Get serial number
			std::cout << "Serial number: ";
			for (int i = 0; i < length; i++) {
				std::cout << static_cast<char>(data[i]);
			}
			std::cout << std::endl;
		}
		else if (code == 0x31) { // Get zoom position
			std::cout << "Zoom position: " << std::hex << (uint)data[0] << " " << (uint)data[1] << std::endl;
		}
		else if (code == 0x32) { // Get focus position
			std::cout << "Focus position: " << std::hex << (uint)data[0] << " " << (uint)data[1] << std::endl;
		}
		return true;
	}

    /*
     * Sanity check
     */
    static void sanityCheck(uchar code, std::vector<uchar> data){
        switch (code) {
            case 0x20: /* Iris control (Position) */
                assert(data.size() == 2 && "Wrong data size");
                break;
            case 0x21: /* Zoom control (Position) */
                assert(data.size() == 2 && "Wrong data size");
                break;
            case 0x22: /* Focus control (Position) */
                assert(data.size() == 2 && "Wrong data size");
                break;
            case 0x40: /* Filter control (VisCut/Clear) */
                assert(data.size() == 1 && "Wrong data size");
                break;
            case 0x42: /* Iris control (Auto/Remote) */
                assert(data.size() == 1 && "Wrong data size");
                break;
			case 0x17: /* Serial number */
				assert(data.size() == 0 && "Wrong data size");
				break;
			case 0x11: /* Name(first half) */
				assert(data.size() == 0 && "Wrong data size");
				break;
			case 0x12: /* Name(second half) */
				assert(data.size() == 0 && "Wrong data size");
				break;
			case 0x31: /* Get zoom position */
				assert(data.size() == 0 && "Wrong data size");
				break;
			case 0x32: /* Get focus position */
				assert(data.size() == 0 && "Wrong data size");
				break;
			default:
                assert(!"Unsupported command for now.");
                break;
        }
    }

    /*
     * Compute checksum
     */
    static uchar checksum(std::vector<uchar> vec){
        uchar sum = 0x00;
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
    std::vector<std::pair<float, float>> ZOOM_LUT = {
            {1.0, 0x0000},
            {1.1, 0x0800},
            {1.2, 0x1400},
            {1.3, 0x2000},
            {1.4, 0x2800},
            {1.5, 0x3400},
            {1.6, 0x3C00},
            {1.7, 0x4000},
            {1.8, 0x4800},
            {1.9, 0x4C00},
            {2.0, 0x5400},
            {2.1, 0x5800},
            {2.2, 0x5C00},
            {2.3, 0x6000},
            {2.4, 0x6400},
            {2.5, 0x6800},
            {2.6, 0x6C00},
            {2.7, 0x7000},
            {2.9, 0x7400},
            {3.0, 0x7800},
            {3.1, 0x7C00},
            {3.3, 0x8000},
            {3.5, 0x8400},
            {3.7, 0x8800},
            {3.9, 0x8C00},
            {4.1, 0x9000},
            {4.4, 0x9400},
            {4.7, 0x9800},
            {5.0, 0x9C00},
            {5.3, 0xA000},
            {5.7, 0xA400},
            {6.1, 0xA800},
            {6.6, 0xAC00},
            {7.1, 0xB000},
            {7.6, 0xB400},
            {8.2, 0xB800},
            {8.9, 0xBC00},
            {9.6, 0xC000},
            {10.4, 0xC400},
            {11.2, 0xC800},
            {12.1, 0xCC00},
            {13.0, 0xD000},
            {14.0, 0xD400},
            {15.1, 0xD800},
            {16.3, 0xDC00},
            {17.6, 0xE000},
            {19.0, 0xE400},
            {20.5, 0xE800},
            {22.2, 0xEC00},
            {23.9, 0xF000},
            {25.8, 0xF400},
            {27.8, 0xF800},
            {29.9, 0xFC00},
            {32.0, 0xFFFF}
    };

    std::vector<std::pair<float, float>> FOCUS_LUT = {
            {3.0, 0x0C00},
            {3.1, 0x1000},
            {3.2, 0x1800},
            {3.3, 0x2000},
            {3.4, 0x2800},
            {3.5, 0x2C00},
            {3.6, 0x3400},
            {3.7, 0x3800},
            {3.8, 0x4000},
            {3.9, 0x4400},
            {4.0, 0x4800},
            {4.1, 0x4C00},
            {4.2, 0x5400},
            {4.3, 0x5800},
            {4.4, 0x5C00},
            {4.6, 0x6000},
            {4.7, 0x6400},
            {4.8, 0x6800},
            {4.9, 0x6C00},
            {5.1, 0x7000},
            {5.2, 0x7400},
            {5.3, 0x7800},
            {5.5, 0x7C00},
            {5.7, 0x8000},
            {5.9, 0x8400},
            {6.0, 0x8800},
            {6.3, 0x8C00},
            {6.5, 0x9000},
            {6.7, 0x9400},
            {7.0, 0x9800},
            {7.2, 0x9C00},
            {7.5, 0xA000},
            {7.9, 0xA400},
            {8.2, 0xA800},
            {8.6, 0xAC00},
            {9.1, 0xB000},
            {9.5, 0xB400},
            {10.1, 0xB800},
            {10.7, 0xBC00},
            {11.3, 0xC000},
            {12.1, 0xC400},
            {13.0, 0xC800},
            {14.0, 0xCC00},
            {15.2, 0xD000},
            {16.6, 0xD400},
            {18.3, 0xD800},
            {20.4, 0xDC00},
            {23.1, 0xE000},
            {26.5, 0xE400},
            {31.2, 0xE800},
            {37.8, 0xEC00},
            {48.2, 0xF000},
            {66.2, 0xF400},
            {106.1, 0xF800},
            {267.7, 0xFC00},
            {500, 0xFFFF} // considered infinity
    };
public:
    friend class ZoomLensControllerTest;

    enum class ZOOM_LENS_FILTER{VISIBLE_LIGHT_CUT_FILTER=0, FILTER_CLEAR=1};
    enum class ZOOM_LENS_IRIS{AUTO=0, REMOTE=1};
    enum class ZOOM_LENS_F{CLOSE=0, F16=1, F11=2, F8=3, F5_6=4, F4=5, OPEN=6};

    ZoomLensControllerUtil(AppMsgPtr _appMsg): appMsg(_appMsg){};


	/**
	 * Setter
	 */

    /* iris control - select F number from ZOOM_LENS_F */
    void setF(ZOOM_LENS_F F){
        uchar data1, data2;
        switch(F){
            case ZOOM_LENS_F::CLOSE:
                data1 = 0x00; data2 = 0x00;
                break;case ZOOM_LENS_F::F16:
                data1 = 0x34; data2 = 0x00;
                break;
            case ZOOM_LENS_F::F11:
                data1 = 0x46; data2 = 0x00;
                break;
            case ZOOM_LENS_F::F8:
                data1 = 0x5E; data2 = 0x00;
                break;
            case ZOOM_LENS_F::F5_6:
                data1 = 0x8E; data2 = 0x00;
                break;
            case ZOOM_LENS_F::F4:
                data1 = 0xEE; data2 = 0x00;
                break;
            case ZOOM_LENS_F::OPEN:
                data1 = 0xFF; data2 = 0xFF;
                break;
            default:
                data1 = 0xFF; data2 = 0xFF; // open
                break;
        }
		command(0x20, { data1, data2 });
    }

    /* Zoom by ratio (1x: wide end <--> 32x: tele end) */
    void setZoomRatio(float ratio){
        ratio = std::clamp(ratio, 1.0f, 32.0f);

        std::vector<float> x,y;
        for (auto i:ZOOM_LUT) { x.push_back(i.first); y.push_back(i.second); }

        uint data = interp(ratio, x, y);
        uchar data1 = static_cast<uchar>(data/256); // C10 protocol uses big endian
        uchar data2 = static_cast<uchar>(data%256);

		command(0x21, { data1, data2 });
    }


    /* focus by meter (3m (Minimum object distance) <--> 500m (Infinity)) */
    void setFocus(float meter){
        meter = std::clamp(meter, 3.0f, 500.0f);

        std::vector<float> x,y;
        for (auto i:FOCUS_LUT) { x.push_back(i.first); y.push_back(i.second); }

        uint data = interp(meter, x, y);
        uchar data1 = static_cast<uchar>(data/256); // C10 protocol uses big endian
        uchar data2 = static_cast<uchar>(data%256);

		command(0x22, { data1, data2 });
    }

    /* switch filter */
    void setFilter(ZOOM_LENS_FILTER filter){
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
    void setIrisMode(ZOOM_LENS_IRIS irisMode){
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

	/*
	 * Getter
	 */

	void getNameFirst() {
		command(0x11, {});
	}
	void getNameSecond() {
		command(0x12, {});
	}

	void getSerialNumber() {
		command(0x17, {});
	}

	void getZoomPosition() {
		command(0x31, {});
	}

	void getFocusPosition() {
		command(0x32, {});
	}


	/* direct command */
	void command(uchar code, std::vector<uchar> data) {
		ZoomLensController::sanityCheck(code, data);

		auto md = appMsg->zlcRequestMessenger->prepareMsg();
		md->code = code;
		md->data = data;
		appMsg->zlcRequestMessenger->send();
	}

private:

    /*
     * Linearly interpolation to get f(q) using a LUT of y=f(x)
     * (x,y are assumed to be ascendant.)
     */
    float interp(float q, std::vector<float> x, std::vector<float> y){
        float r; // r = f(q)
        q = std::clamp(q, x.front(), x.back()); // clamp to valid range
        auto next = std::find_if(x.begin(), x.end(), [&q](float v) { return q <= v; });
        if(next == x.begin()){
            r = y.front();
        } else {
            auto prev = next-1;
            std::size_t index = std::distance(x.begin(), prev);
            float prevV = y[index];
            float nextV = y[index + 1];
            r = (prevV * (*next - q) + nextV * (q - *prev))/(*next-*prev);
        }
        return r;
    }

    AppMsgPtr appMsg;
};

using ZLC = ZoomLensController;
using ZLCUtil = ZoomLensControllerUtil;

/*
 * Test for ZoomLensController and ZoomLensControllerUtil
 */
class ZoomLensControllerTest {
public:

    ZoomLensControllerTest(AppMsgPtr _appMsg): appMsg(_appMsg){ }

	/*
	 * Check if command is generated correctly.
	 */
    bool run() {
        ZLCUtil zlcUtil(appMsg);

        /*
         * interp
         */
        std::vector<float> x{1.0, 3.0, 6.0, 10.0};
        std::vector<float> y{2.0, 4.0, 9.0, 15.0};
        std::cout
                << zlcUtil.interp(0.0, x, y) << " -- 2.0 (Expected output)" << std::endl
                << zlcUtil.interp(1.0, x, y) << " -- 2.0 (Expected output)" << std::endl
                << zlcUtil.interp(2.0, x, y) << " -- 3.0 (Expected output)" << std::endl
                << zlcUtil.interp(3.0, x, y) << " -- 4.0 (Expected output)" << std::endl
                << zlcUtil.interp(4.0, x, y) << " -- 5.6666 (Expected output)" << std::endl
                << zlcUtil.interp(6.0, x, y) << " -- 9.0(Expected output)" << std::endl
                << zlcUtil.interp(9.0, x, y) << " -- 13.5 (Expected output)" << std::endl
                << zlcUtil.interp(11.0, x, y) << " -- 15.0 (Expected output)" << std::endl;


        /*
         * iris
         */
        zlcUtil.setF(ZoomLensControllerUtil::ZOOM_LENS_F::CLOSE); // close
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 20 0 0 de (Expected output)" << std::endl;

        zlcUtil.setF(ZoomLensControllerUtil::ZOOM_LENS_F::OPEN); // open
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 20 ff ff e0 (Expected output)" << std::endl;

        /*
         * setZoomRatio
         */
        zlcUtil.command(0x21, {0x00, 0x00}); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 0 0 dd (Expected output)" << std::endl;

        zlcUtil.command(0x21, {0xFF, 0xFF}); // tele end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 ff ff df (Expected output)" << std::endl;

        zlcUtil.setZoomRatio(1.0); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 0 0 dd (Expected output)" << std::endl;

        zlcUtil.setZoomRatio(2.0); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 54 00 89 (Expected output)" << std::endl;

        zlcUtil.setZoomRatio(3.0); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 78 00 65 (Expected output)" << std::endl;

        zlcUtil.setZoomRatio(10.0); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 C2 00 1B (Expected output)" << std::endl;

        zlcUtil.setZoomRatio(32.0); // tele end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 ff ff df (Expected output)" << std::endl;


        /*
         * focus
         */
        zlcUtil.setFocus(3.0); // focus at 3m
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 22 0C 00 D0 (Expected output)" << std::endl;

        zlcUtil.setFocus(5.0); // focus at 50m
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 22 6E 00 6E (Expected output)" << std::endl;

        zlcUtil.setFocus(100.0); // focus at 100m
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 22 F7 63 82 (Expected output)" << std::endl;

        zlcUtil.setFocus(500.0); // focus at infinity
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 22 FF FF DE (Expected output)" << std::endl;

        return true;
    }

private:

    AppMsgPtr appMsg;

};

#endif //ZOOM_LENS_CONTROLLER_H
