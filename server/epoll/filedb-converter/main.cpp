#include "FiledbConverter.hpp"

#include <cstring>
#include <iostream>
#include <string>
#include <sys/stat.h>

static void usage(const char *argv0) {
    std::cerr
        << "Usage: " << argv0 << " <command> <path>\n\n"
        << "Commands:\n"
        << "  b2x <path>   Convert binary -> XML. <path> may be a file or a directory.\n"
        << "               For files the XML is written at <path>.xml.\n"
        << "               For directories every recognised binary under <path> is\n"
        << "               converted in place.\n"
        << "  x2b <path>   Reverse: XML -> binary. For a file <path> must end in .xml;\n"
        << "               the binary is written at <path> with the .xml suffix removed.\n"
        << "               For directories every *.xml under <path> is converted back.\n"
        << "  help         Show this help\n\n"
        << "The recognised layout mirrors catchchallenger-server-cli-epoll's\n"
        << "database/ directory (see the server's database-filedb.md)."
        << std::endl;
}

static bool isDir(const std::string &p) {
    struct stat sb{};
    return ::stat(p.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }
    const std::string cmd = argv[1];
    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        usage(argv[0]);
        return 0;
    }
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
    const std::string path = argv[2];

    FiledbConverter::Direction dir;
    if (cmd == "b2x") dir = FiledbConverter::Direction::BinToXml;
    else if (cmd == "x2b") dir = FiledbConverter::Direction::XmlToBin;
    else {
        usage(argv[0]);
        return 1;
    }

    if (isDir(path))
        return FiledbConverter::convertDirectory(path, dir);
    return FiledbConverter::convertFile(path, dir);
}
