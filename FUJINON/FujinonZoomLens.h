//
// Created by Masahiro Hirano <masahiro.dll@gmail.com>
//

#ifndef FUJINON_ZOOM_LENS_H
#define FUJINON_ZOOM_LENS_H

#include <iostream>

/*
 * Helper class to use FujinonZoomLensController
 */
namespace FujinonZoomLensControllerUtil {
	inline std::vector<std::pair<float, float>> ZOOM_LUT = {
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

	inline std::vector<std::pair<float, float>> FOCUS_LUT = {
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

	enum class ZOOM_LENS_FILTER { VISIBLE_LIGHT_CUT_FILTER = 0, FILTER_CLEAR = 1 };
	enum class ZOOM_LENS_IRIS { AUTO = 0, REMOTE = 1 };
	enum class ZOOM_LENS_F { CLOSE = 0, F16 = 1, F11 = 2, F8 = 3, F5_6 = 4, F4 = 5, OPEN = 6 };

	/*
	 * Compute checksum
	 */
	inline uchar checksum(std::vector<uchar> vec) {
		uchar sum = 0x00;
		for (int i : vec) sum += i;
		return 0x100 - sum;
	}

	/*
	 * Generate command in C10 protocol
	 */
	inline std::vector<uchar> encodeCommand(uchar code, std::vector<uchar> data) {
		std::vector<uchar> api_frame;
		api_frame.push_back(data.size()); // append data length
		api_frame.push_back(code); // append function code
		for (auto i : data) { api_frame.push_back(i); } // append function data
		api_frame.push_back(checksum(api_frame)); // append checksum

		return api_frame;
	}

	/*
	 * Decode command in C10 protocol
	 */
	inline bool decodeCommand(const boost::array<uchar, 32> api_frame) {
		// print frame
//		for (size_t i = 0; i < api_frame.size() - 1; ++i) {
//			std::cout << std::hex << static_cast<uint>(api_frame[i]) << " ";
//		}
//		std::cout << std::dec << std::endl;

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
	inline void sanityCheck(uchar code, std::vector<uchar> data) {
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
}

/*
 * Command format for the zoom lens
 */
struct FujinonZoomLensCommand {
	uchar code;
	std::vector<uchar> data;
};

/*
 * Inherit this class to implement client
 */
class FujinonZoomLensClientTemplate {
public:
	virtual void send(FujinonZoomLensCommand cmd) = 0;
};

/*
 * Controller for fujinon zoom lens
 */
class FujinonZoomLensController {
public:

	/*
	 * Constructor
	 *
	 * 1. Register client to the controller
	 */
	FujinonZoomLensController(std::shared_ptr<FujinonZoomLensClientTemplate> _client):client(_client) {};

	/**
	 * Setter
	 */

	/* iris control - select F number from ZOOM_LENS_F */
	void setF(FujinonZoomLensControllerUtil::ZOOM_LENS_F F) {
		uchar data1, data2;
		switch (F) {
		case FujinonZoomLensControllerUtil::ZOOM_LENS_F::CLOSE:
			data1 = 0x00; data2 = 0x00;
		break; case FujinonZoomLensControllerUtil::ZOOM_LENS_F::F16:
			data1 = 0x34; data2 = 0x00;
			break;
		case FujinonZoomLensControllerUtil::ZOOM_LENS_F::F11:
			data1 = 0x46; data2 = 0x00;
			break;
		case FujinonZoomLensControllerUtil::ZOOM_LENS_F::F8:
			data1 = 0x5E; data2 = 0x00;
			break;
		case FujinonZoomLensControllerUtil::ZOOM_LENS_F::F5_6:
			data1 = 0x8E; data2 = 0x00;
			break;
		case FujinonZoomLensControllerUtil::ZOOM_LENS_F::F4:
			data1 = 0xEE; data2 = 0x00;
			break;
		case FujinonZoomLensControllerUtil::ZOOM_LENS_F::OPEN:
			data1 = 0xFF; data2 = 0xFF;
			break;
		default:
			data1 = 0xFF; data2 = 0xFF; // open
			break;
		}
		command(0x20, { data1, data2 });
	}

	/* Zoom by ratio (1x: wide end <--> 32x: tele end) */
	void setZoomRatio(float ratio) {
		ratio = std::clamp(ratio, 1.0f, 32.0f);

		std::vector<float> x, y;
		for (auto i : FujinonZoomLensControllerUtil::ZOOM_LUT) { x.push_back(i.first); y.push_back(i.second); }

		uint data = interp(ratio, x, y);
		uchar data1 = static_cast<uchar>(data / 256); // C10 protocol uses big endian
		uchar data2 = static_cast<uchar>(data % 256);

		command(0x21, { data1, data2 });
	}


	/* focus by meter (3m (Minimum object distance) <--> 500m (Infinity)) */
	void setFocus(float meter) {
		meter = std::clamp(meter, 3.0f, 500.0f);

		std::vector<float> x, y;
		for (auto i : FujinonZoomLensControllerUtil::FOCUS_LUT) { x.push_back(i.first); y.push_back(i.second); }

		uint data = interp(meter, x, y);
		uchar data1 = static_cast<uchar>(data / 256); // C10 protocol uses big endian
		uchar data2 = static_cast<uchar>(data % 256);

		command(0x22, { data1, data2 });
	}

	/* switch filter */
	void setFilter(FujinonZoomLensControllerUtil::ZOOM_LENS_FILTER filter) {
		switch (filter) {
		case FujinonZoomLensControllerUtil::ZOOM_LENS_FILTER::VISIBLE_LIGHT_CUT_FILTER:
			command(0x40, { 0xF0 });
			break;
		case FujinonZoomLensControllerUtil::ZOOM_LENS_FILTER::FILTER_CLEAR:
			command(0x40, { 0xE0 });
			break;
		default:
			command(0x40, { 0xE0 });
			break;
		}
	}

	/* iris mode */
	void setIrisMode(FujinonZoomLensControllerUtil::ZOOM_LENS_IRIS irisMode) {
		switch (irisMode) {
		case FujinonZoomLensControllerUtil::ZOOM_LENS_IRIS::AUTO:
			command(0x42, { 0xCC });
			break;
		case FujinonZoomLensControllerUtil::ZOOM_LENS_IRIS::REMOTE:
			command(0x42, { 0xDC });
			break;
		default:
			command(0x42, { 0xDC });
			break;
		}
	}

	/*
	 * Getter
	 */
	void getNameFirst() { command(0x11, {}); }
	void getNameSecond() { command(0x12, {}); }
	void getSerialNumber() { command(0x17, {}); }
	void getZoomPosition() { command(0x31, {}); }
	void getFocusPosition() { command(0x32, {}); }

	/* Send command via registered sender */
	void command(uchar code, std::vector<uchar> data) {
		FujinonZoomLensControllerUtil::sanityCheck(code, data);

		FujinonZoomLensCommand cmd;
		cmd.code = code;
		cmd.data = data;
		client->send(cmd);
	}

	friend class FujinonZoomLensControllerTest;

private:

	std::shared_ptr<FujinonZoomLensClientTemplate> client;

	/*
	 * Linearly interpolation to get f(q) using a LUT of y=f(x)
	 * (x,y are assumed to be ascendant.)
	 */
	float interp(float q, std::vector<float> x, std::vector<float> y) {
		float r; // r = f(q)
		q = std::clamp(q, x.front(), x.back()); // clamp to valid range
		auto next = std::find_if(x.begin(), x.end(), [&q](float v) { return q <= v; });
		if (next == x.begin()) {
			r = y.front();
		}
		else {
			auto prev = next - 1;
			std::size_t index = std::distance(x.begin(), prev);
			float prevV = y[index];
			float nextV = y[index + 1];
			r = (prevV * (*next - q) + nextV * (q - *prev)) / (*next - *prev);
		}
		return r;
	}

};





/*
 * Test for FujinonZoomLensController
 */
class FujinonZoomLensControllerTest {
public:

    FujinonZoomLensControllerTest(AppMsgPtr _appMsg): appMsg(_appMsg){ }

	/*
	 * Check if command is generated correctly.
	 */
    bool run() {
		class FujinonZoomLensClientTest : public FujinonZoomLensClientTemplate {
			void send(FujinonZoomLensCommand cmd){}
		};

		auto client = std::make_shared<FujinonZoomLensClientTest>();

		FujinonZoomLensController zlc(std::static_pointer_cast<FujinonZoomLensClientTemplate>(client));
        /*
         * interp
         */
        std::vector<float> x{1.0, 3.0, 6.0, 10.0};
        std::vector<float> y{2.0, 4.0, 9.0, 15.0};
        std::cout
                << zlc.interp(0.0, x, y) << " -- 2.0 (Expected output)" << std::endl
                << zlc.interp(1.0, x, y) << " -- 2.0 (Expected output)" << std::endl
                << zlc.interp(2.0, x, y) << " -- 3.0 (Expected output)" << std::endl
                << zlc.interp(3.0, x, y) << " -- 4.0 (Expected output)" << std::endl
                << zlc.interp(4.0, x, y) << " -- 5.6666 (Expected output)" << std::endl
                << zlc.interp(6.0, x, y) << " -- 9.0(Expected output)" << std::endl
                << zlc.interp(9.0, x, y) << " -- 13.5 (Expected output)" << std::endl
                << zlc.interp(11.0, x, y) << " -- 15.0 (Expected output)" << std::endl;


        /*
         * iris
         */
		zlc.setF(FujinonZoomLensControllerUtil::ZOOM_LENS_F::CLOSE); // close
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 20 0 0 de (Expected output)" << std::endl;

		zlc.setF(FujinonZoomLensControllerUtil::ZOOM_LENS_F::OPEN); // open
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 20 ff ff e0 (Expected output)" << std::endl;

        /*
         * setZoomRatio
         */
		zlc.command(0x21, {0x00, 0x00}); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 0 0 dd (Expected output)" << std::endl;

		zlc.command(0x21, {0xFF, 0xFF}); // tele end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 ff ff df (Expected output)" << std::endl;

		zlc.setZoomRatio(1.0); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 0 0 dd (Expected output)" << std::endl;

		zlc.setZoomRatio(2.0); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 54 00 89 (Expected output)" << std::endl;

		zlc.setZoomRatio(3.0); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 78 00 65 (Expected output)" << std::endl;

		zlc.setZoomRatio(10.0); // wide end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 C2 00 1B (Expected output)" << std::endl;

		zlc.setZoomRatio(32.0); // tele end
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 21 ff ff df (Expected output)" << std::endl;


        /*
         * focus
         */
		zlc.setFocus(3.0); // focus at 3m
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 22 0C 00 D0 (Expected output)" << std::endl;

		zlc.setFocus(5.0); // focus at 50m
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 22 6E 00 6E (Expected output)" << std::endl;

		zlc.setFocus(100.0); // focus at 100m
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 22 F7 63 82 (Expected output)" << std::endl;

		zlc.setFocus(500.0); // focus at infinity
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "2 22 FF FF DE (Expected output)" << std::endl;

        return true;
    }

private:

    AppMsgPtr appMsg;

};

#endif //FUJINON_ZOOM_LENS_H
