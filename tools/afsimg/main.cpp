#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "image.hpp"
#include "afs.hpp"

int main(int argc, char **argv)
{
    if (argc < 1) {
        fprintf(stderr, "usage: %s image[@@offset] subcommand [...]\n", argv[0]);
        return 1;
    }

    lba_t offset = 0;
    char *sep_pos = strstr(argv[1], "@@");
    if (sep_pos) {
        *sep_pos = '\0';
        offset = strtoll(sep_pos + 2, NULL, 10);
    }

    std::string image_offset(argv[1]);

    Image image(argv[1], true);

    Afs afs(image, offset);

    printf("total sector count: %llu\n", afs.getTotalSectorCount());
    printf("total block count: %llu\n", afs.getTotalBlockCount());
    printf("rdb copy count: %u\n", afs.getRdbCopyCount());
    printf("bytes per sector: %u\n", afs.getBytesPerSector());
    printf("sectors per block: %u\n", afs.getSectorsPerBlock());

    std::unique_ptr<Afs::Directory> root_dir = std::unique_ptr<Afs::Directory>(afs.openRootDirectory());

    printf("volume name: %s\n", root_dir->getName().c_str());

    return 0;
}
