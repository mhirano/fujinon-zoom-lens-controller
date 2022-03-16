//
// Created by Masahiro Hirano <masahiro.dll@gmail.com>
//

#ifndef ZOOM_LENS_COMMAND_H
#define ZOOM_LENS_COMMAND_H

struct ZLCCommand {
public:
    unsigned char code;
    std::vector<unsigned char> data;
};

#endif //ZOOM_LENS_COMMAND_H