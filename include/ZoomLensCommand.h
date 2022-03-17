//
// Created by Masahiro Hirano <masahiro.dll@gmail.com>
//

#ifndef ZOOM_LENS_COMMAND_H
#define ZOOM_LENS_COMMAND_H

struct ZLCCommand {
public:
    uchar code;
    std::vector<uchar> data;
};

#endif //ZOOM_LENS_COMMAND_H