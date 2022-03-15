//
// Created by Masahiro Hirano <masahiro.dll@gmail.com>
//

#ifndef ZOOM_LENS_COMMAND_H
#define ZOOM_LENS_COMMAND_H

class ZLCCommand {
public:
	unsigned char zoom[2] = {0x00, 0x00}; // 0000: wide ---- FFFF: tele
};

#endif //ZOOM_LENS_COMMAND_H