#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <getopt.h>

extern unsigned int crc32(const unsigned char *buf, unsigned int len);

typedef struct {
    uint8_t bytes[16];
} __attribute__((packed)) afs_uuid_t;

struct first_sector {
    uint8_t bootcode[504];
    char filesystem_signature[4];
    uint16_t reserved_sectors;
    uint16_t vbr_signature;
} __attribute__((packed));

struct rdb {
    uint64_t total_sector_count;
    uint64_t total_block_count;
    uint8_t sectors_per_block;
    uint8_t rdb_copy_count;
    uint16_t bytes_per_sector;
    uint32_t flags;
    uint64_t root_mdb_pointer;
    uint64_t udb_pointer;
    uint64_t jbb_pointer;
    uint64_t rbb_pointer;
    uint64_t group0_gbb_pointer;
    afs_uuid_t volume_uuid;
    char formatted_os[16];
    uint16_t filesystem_version;
} __attribute__((packed));

struct mdb {
    uint64_t parent_mdb_pointer;
    uint64_t next_mdb_pointer;
    uint32_t flags;
    uint32_t attribute_entry_presence_bitmap;
    uint64_t file_size;
    uint64_t dhb_ddb_pointer;
    uint8_t reserved[8];
} __attribute__((packed));

struct mdb_attribute_entry_header {
    uint32_t entry_length;
    uint8_t entry_type;
    uint8_t reserved[3];
} __attribute__((packed));

enum mdb_entry_type {
    MDB_ENTRY_FAE = 1,
    MDB_ENTRY_TAE,
    MDB_ENTRY_PAE,
    MDB_ENTRY_QAE,
    MDB_ENTRY_EAE,
    MDB_ENTRY_CAE,
};

struct mdb_fae {
    struct mdb_attribute_entry_header header;
    uint32_t filename_hash;
    uint8_t reserved[4];
    uint8_t filename_length;
    char filename[255];
} __attribute__((packed));

struct mdb_tae {
    struct mdb_attribute_entry_header header;
    uint64_t create_time;
    uint64_t write_time;
    uint64_t read_time;
} __attribute__((packed));

struct mdb_pae {
    struct mdb_attribute_entry_header header;
    uint64_t acb_pointer;
} __attribute__((packed));

struct mdb_qae {
    struct mdb_attribute_entry_header header;
    uint64_t qcb_pointer;
} __attribute__((packed));

struct mdb_eae {
    struct mdb_attribute_entry_header header;
    uint8_t encryption_algorithm;
    uint8_t reserved[15];
    uint8_t decription_test_data[];
} __attribute__((packed));

struct mdb_cae {
    struct mdb_attribute_entry_header header;
    uint8_t compression_algorithm;
    uint8_t reserved[15];
} __attribute__((packed));

struct acb_entry {
    afs_uuid_t uuid;
    uint16_t permission_bitmap;
    uint16_t flags;
    uint8_t reserved[12];
} __attribute__((packed));

struct acb {
    uint64_t parent_acb_head_pointer;
    uint64_t next_acb_pointer;
    uint16_t entry_count;
    uint8_t reserved[6];
    struct acb_entry entries[];
} __attribute__((packed));

struct qcb_entry {
    afs_uuid_t uuid;
    uint64_t current_file_count;
    uint64_t file_count_limit;
    uint64_t block_count_soft_limit;
    uint64_t block_count_hard_limit;
    uint64_t current_block_count;
    uint64_t last_limit_alarm_time;
    uint32_t flags;
    uint8_t reserved[4];
} __attribute__((packed));

struct qcb {
    uint64_t next_qcb_pointer;
    uint16_t entry_count;
    uint8_t reserved[14];
    struct qcb_entry entries[];
} __attribute__((packed));

struct dhb_entry {
    uint64_t mdb_pointer;
    uint32_t filename_hash;
    uint8_t reserved[6];
    uint16_t left_child_offset;
    uint16_t right_child_offset;
    uint16_t next_child_offset;
    uint64_t left_child_block;
    uint64_t right_child_block;
    uint64_t next_child_block;
} __attribute__((packed));

struct dhb {
    uint16_t entry_count;
    uint8_t reserved[14];
    struct dhb_entry entries[];
} __attribute__((packed));

struct ddb {
    uint64_t previous_ddb_pointer;
    uint64_t next_ddb_pointer;
    uint64_t dsb_pointers[];
} __attribute__((packed));

static int decode_size(const char *str, uint64_t *value)
{
    uint64_t result = 0;

    for (int i = 0; str[i]; i++) {
        if (!isdigit(str[i])) {
            if (str[i + 1]) {
                return 1;
            }

            switch (str[i]) {
                case 'T':
                    result *= 1024;
                case 'G':
                    result *= 1024;
                case 'M':
                    result *= 1024;
                case 'k':
                    result *= 1024;
                    break;
                default:
                    return 1;
            }
            
            break;
        }

        result *= 10;
        result += str[i] - '0';
    }

    *value = result;
    return 0;
}

static int decode_uint(const char *str, uint64_t *value)
{
    uint64_t result = 0;

    if (str[0] != '0') {
        for (int i = 0; str[i]; i++) {
            if (!isdigit(str[i])) {
                return 1;
            }

            result *= 10;
            result += str[i] - '0';
        }

        *value = result;
        return 0;
    }

    if (str[1] == 'x') {
        for (int i = 2; str[i]; i++) {
            if (!isxdigit(str[i])) {
                return 1;
            }

            result <<= 4;
            result |= str[i] <= '9' ? str[i] - '0' : ((str[i] <= 'F' ? str[i] - 'A' : str[i] - 'a') + 10);
        }

        *value = result;
        return 0;
    }
    
    if (str[1] == 'o' || isdigit(str[1])) {
        for (int i = str[1] == 'o' ? 2 : 1; str[i]; i++) {
            if (!isdigit(str[i]) || str[i] >= '8') {
                return 1;
            }

            result <<= 3;
            result |= str[i] - '0';
        }

        *value = result;
        return 0;
    }
    
    if (str[1] == 'b') {
        for (int i = 2; str[i]; i++) {
            if (str[i] != '0' && str[i] != '1') {
                return 1;
            }

            result <<= 1;
            result |= str[i] - '0';
        }

        *value = result;
        return 0;
    }

    *value = result;
    return 0;
}

static int decode_uuid(const char *str, afs_uuid_t *value)
{
    memset(value, 0, sizeof(afs_uuid_t));

    int digit_idx = 0;
    for (int i = 0; str[i]; i++) {
        if (!isxdigit(str[i]) && str[i] != '-') {
            return 1;
        }

        if (str[i] == '-') {
            if (i != 8 && i != 13 && i != 18 && i != 23) {
                return 1;
            } else {
                continue;
            }
        }

        if (digit_idx >= 32) {
            return 1;
        }

        value->bytes[digit_idx >> 1] <<= 4;
        value->bytes[digit_idx >> 1] |= str[i] <= '9' ? str[i] - '0' : ((str[i] <= 'F' ? str[i] - 'A' : str[i] - 'a') + 10);
        digit_idx++;
    }

    return 0;
}

static const char *argv0;
static int verbose = 0, dry_run = 0, image = 0;

static long afs_fread(void *ptr, size_t size, size_t count, FILE *stream)
{
    return fread(ptr, size, count, stream);
}

static long afs_fwrite(const void *ptr, size_t size, size_t count, FILE *stream)
{
    if (dry_run) {
        return count;
    }

    return fwrite(ptr, size, count, stream);
}

static int make_fs(const char *file, const char *boot_code_file, const char *os_name, const char *label, uint8_t rdb_count, uint8_t sectors_per_block, uint16_t reserved_sectors, uint64_t journal_size, afs_uuid_t uuid)
{
    uint16_t bytes_per_sector = 512;
    uint64_t total_sector_count;
    uint64_t total_block_count;
    uint32_t bytes_per_block;
    uint32_t blocks_per_block_group;
    uint64_t total_block_group_count;
    uint64_t rbb_count;
    off_t file_len;
    int label_len;
    
    FILE *image_fp = NULL;
    FILE *boot_code_fp = NULL;

    void *boot_code, *buf;

    image_fp = fopen(file, "rb+");
    if (!image_fp) {
        fprintf(stderr, "%s: cannot open file: %s\n", argv0, file);
        return 1;
    }
    
    fseek(image_fp, 0, SEEK_END);
    file_len = ftello(image_fp);
    if (file_len % bytes_per_sector) {
        fprintf(stderr, "%s: file size is not a multiple of sector size\n", argv0);
        fclose(image_fp);
        return 1;
    }
    total_sector_count = file_len / bytes_per_sector;

    total_block_count = (total_sector_count - reserved_sectors) / sectors_per_block;
    bytes_per_block = bytes_per_sector * sectors_per_block;
    blocks_per_block_group = bytes_per_block * 8 - 32;
    total_block_group_count = total_block_count / blocks_per_block_group;
    rbb_count = (total_block_group_count + blocks_per_block_group - 1) / blocks_per_block_group;

    if (!total_block_count || !bytes_per_block || !blocks_per_block_group || !total_block_group_count || !rbb_count) {
        fprintf(stderr, "%s: file size is too small for this configuration\n", argv0);
        fclose(image_fp);
        return 1;
    }

    printf("Creating AFS Volume (%llu sectors, %llu blocks, %llu block groups)\n", total_sector_count, total_block_count, total_block_group_count);
    
    if (boot_code_file) {
        boot_code_fp = fopen(boot_code_file, "rb");
        if (!boot_code_fp) {
            fprintf(stderr, "%s: cannot open file: %s\n", argv0, boot_code_file);
            return 1;
        }

        fseek(boot_code_fp, 0, SEEK_END);
        file_len = ftello(boot_code_fp);
        if (file_len > reserved_sectors * bytes_per_sector) {
            fprintf(stderr, "%s: boot code is larger than reserved sectors area\n", argv0);
            fclose(boot_code_fp);
            fclose(image_fp);
            return 1;
        }
        
        boot_code = malloc(file_len);

        fseek(boot_code_fp, 0, SEEK_SET);
        afs_fread(boot_code, file_len, 1, boot_code_fp);

        fseek(image_fp, 0, SEEK_SET);
        afs_fwrite(boot_code, file_len, 1, image_fp);
        fflush(image_fp);

        free(boot_code);
        fclose(boot_code_fp);
    }
    
    buf = malloc(sizeof(struct first_sector));
    
    /* write sector 0 */
    fseek(image_fp, 0, SEEK_SET);
    afs_fread(buf, sizeof(struct first_sector), 1, image_fp);
    ((struct first_sector *)buf)->filesystem_signature[0] = 'A';
    ((struct first_sector *)buf)->filesystem_signature[1] = 'F';
    ((struct first_sector *)buf)->filesystem_signature[2] = 'S';
    ((struct first_sector *)buf)->filesystem_signature[3] = '\0';
    ((struct first_sector *)buf)->reserved_sectors = reserved_sectors;
    ((struct first_sector *)buf)->vbr_signature = 0xAA55;
    fseek(image_fp, 0, SEEK_SET);
    afs_fwrite(buf, sizeof(struct first_sector), 1, image_fp);
    fflush(image_fp);

    buf = realloc(buf, bytes_per_block);
    
    /* write RDB */
    for (int i = 0; i < rdb_count; i++) {
        fseek(image_fp, bytes_per_sector * reserved_sectors + bytes_per_block * i, SEEK_SET);
        memset(buf, 0, bytes_per_block);
        ((struct rdb *)buf)->total_sector_count = total_sector_count;
        ((struct rdb *)buf)->total_block_count = total_block_count;
        ((struct rdb *)buf)->sectors_per_block = sectors_per_block;
        ((struct rdb *)buf)->rdb_copy_count = rdb_count;
        ((struct rdb *)buf)->bytes_per_sector = bytes_per_sector;
        ((struct rdb *)buf)->flags = 0;
        ((struct rdb *)buf)->root_mdb_pointer = 2 + rbb_count;
        ((struct rdb *)buf)->udb_pointer = 0;
        ((struct rdb *)buf)->jbb_pointer = 0;
        ((struct rdb *)buf)->rbb_pointer = 1;
        ((struct rdb *)buf)->group0_gbb_pointer = 1 + rbb_count;
        ((struct rdb *)buf)->volume_uuid = uuid;
        strncpy(((struct rdb *)buf)->formatted_os, os_name, sizeof(((struct rdb *)buf)->formatted_os));
        ((struct rdb *)buf)->filesystem_version = 0x0001;
        *(uint32_t *)((uint8_t *)buf + bytes_per_block - 4) = crc32(buf, bytes_per_block - 4);
        afs_fwrite(buf, bytes_per_block, 1, image_fp);
        fflush(image_fp);
    }

    /* write RBB */
    fseek(image_fp, bytes_per_sector * reserved_sectors + bytes_per_block * rdb_count, SEEK_SET);
    memset(buf, 0, bytes_per_block);
    for (uint64_t i = 1; i < total_block_group_count; i = (i & ~0x07) + 8) {
        uint64_t groups_left = total_block_group_count - i;
        ((uint8_t *)buf)[i >> 3] = (groups_left < 8 ? ((1 << groups_left) - 1) : 0xFF) & (0xFF << (i & 0x07));
    }
    *(uint32_t *)((uint8_t *)buf + bytes_per_block - 4) = crc32(buf, bytes_per_block - 4);
    afs_fwrite(buf, bytes_per_block, 1, image_fp);
    fflush(image_fp);

    /* write Group 0 GBB */
    fseek(image_fp, bytes_per_sector * reserved_sectors + bytes_per_block * (rdb_count + rbb_count), SEEK_SET);
    memset(buf, 0, bytes_per_block);
    for (uint64_t i = rdb_count + 2 + rbb_count; i < blocks_per_block_group; i = (i & ~0x07) + 8) {
        uint64_t blocks_left = blocks_per_block_group - i;
        ((uint8_t *)buf)[i >> 3] = (blocks_left < 8 ? ((1 << blocks_left) - 1) : 0xFF) & (0xFF << (i & 0x07));
    }
    *(uint32_t *)((uint8_t *)buf + bytes_per_block - 4) = crc32(buf, bytes_per_block - 4);
    afs_fwrite(buf, bytes_per_block, 1, image_fp);
    fflush(image_fp);

    /* write root MDB */
    if (!label) label = "NO_NAME";
    label_len = strnlen(label, 255);
    fseek(image_fp, bytes_per_sector * reserved_sectors + bytes_per_block * (rdb_count + 1 + rbb_count), SEEK_SET);
    memset(buf, 0, bytes_per_block);
    ((struct mdb *)buf)->parent_mdb_pointer = 0;
    ((struct mdb *)buf)->next_mdb_pointer = 0;
    ((struct mdb *)buf)->flags = 1;  /* directory */
    ((struct mdb *)buf)->attribute_entry_presence_bitmap = (1 << (MDB_ENTRY_FAE - 1)) | (1 << (MDB_ENTRY_TAE - 1));
    ((struct mdb *)buf)->file_size = 0;
    ((struct mdb *)buf)->dhb_ddb_pointer = 3 + rbb_count;

    /* FAE */
    ((struct mdb_fae *)((uint8_t *)buf + sizeof(struct mdb)))->header.entry_length = sizeof(struct mdb_fae);
    ((struct mdb_fae *)((uint8_t *)buf + sizeof(struct mdb)))->header.entry_type = MDB_ENTRY_FAE;
    ((struct mdb_fae *)((uint8_t *)buf + sizeof(struct mdb)))->filename_length = label_len;
    ((struct mdb_fae *)((uint8_t *)buf + sizeof(struct mdb)))->filename_hash = crc32((void *)label, label_len);
    strncpy(((struct mdb_fae *)((uint8_t *)buf + sizeof(struct mdb)))->filename, label, label_len);

    /* TAE */
    ((struct mdb_tae *)((uint8_t *)buf + sizeof(struct mdb) + sizeof(struct mdb_fae)))->header.entry_length = sizeof(struct mdb_tae);
    ((struct mdb_tae *)((uint8_t *)buf + sizeof(struct mdb) + sizeof(struct mdb_fae)))->header.entry_type = MDB_ENTRY_TAE;
    ((struct mdb_tae *)((uint8_t *)buf + sizeof(struct mdb) + sizeof(struct mdb_fae)))->create_time = time(NULL);
    ((struct mdb_tae *)((uint8_t *)buf + sizeof(struct mdb) + sizeof(struct mdb_fae)))->write_time = time(NULL);
    ((struct mdb_tae *)((uint8_t *)buf + sizeof(struct mdb) + sizeof(struct mdb_fae)))->read_time = time(NULL);

    *(uint32_t *)((uint8_t *)buf + bytes_per_block - 4) = crc32(buf, bytes_per_block - 4);
    afs_fwrite(buf, bytes_per_block, 1, image_fp);
    fflush(image_fp);

    /* write root dhb */
    fseek(image_fp, bytes_per_sector * reserved_sectors + bytes_per_block * (rdb_count + 2 + rbb_count), SEEK_SET);
    memset(buf, 0, bytes_per_block);
    *(uint32_t *)((uint8_t *)buf + bytes_per_block - 4) = crc32(buf, bytes_per_block - 4);
    afs_fwrite(buf, bytes_per_block, 1, image_fp);
    fflush(image_fp);

    free(buf);
    fclose(image_fp);

    printf("Filesystem used %llu blocks out of %llu blocks (%.2f%%)\n", rdb_count + 2 + rbb_count, total_block_count, (float)5 / total_block_count);

    return 0;
}

static const struct option options[] = {
    { "boot-code", 1, NULL, 'b' },
    { "dry-run", 0, NULL, 'd' },
    { "image", 0, NULL, 'i'},
    { "journal-size", 1, NULL, 'j' },
    { "label", 1, NULL, 'l' },
    { "rdb-count", 1, NULL, 'N' },
    { "os-name", 1, NULL, 'o' },
    { "reserve", 1, NULL, 'r' },
    { "sectors-per-block", 1, NULL, 's' },
    { "uuid", 1, NULL, 'u'},
    { "verbose", 0, NULL, 'v' },
    { NULL, 0, NULL, 0 },
};

static void print_usage(void)
{
    fprintf(stderr, "Usage: %s [-b boot-code] [-j journal-size] [-l label]\n", argv0);
    fprintf(stderr, "\t[-N rdb-count] [-o os-name]\n");
    fprintf(stderr, "\t[-r reserve] [-s sectors-per-block] [-u uuid]\n");
    fprintf(stderr, "\t[-div] device/file\n");
}

int main(int argc, char **argv)
{
    int next_option;

    uint64_t tmp;
    char *boot_code_file = NULL, *os_name = NULL, *label = NULL, *file = NULL;
    uint8_t rdb_count = 1, sectors_per_block = 8;
    uint16_t reserved_sectors = 16;
    uint64_t journal_size = 1024 * 1024;
    afs_uuid_t uuid;

    argv0 = argv[0];

    do {
        next_option = getopt_long(argc, argv, "b:dij:l:N:o:r:s:u:v", options, NULL);

        switch (next_option) {
            case 'b':
                boot_code_file = optarg;
                break;
            case 'd':
                dry_run = 1;
                break;
            case 'i':
                image = 1;
                break;
            case 'j':
                if (decode_size(optarg, &tmp)) {
                    fprintf(stderr, "%s: journal-size: invalid size\n", argv0);
                    exit(1);
                }
                journal_size = tmp;
                break;
            case 'l':
                label = optarg;
                break;
            case 'N':
                if (decode_uint(optarg, &tmp) || tmp >= 256) {
                    fprintf(stderr, "%s: mdb-count: invalid numeric value\n", argv0);
                    exit(1);
                }
                rdb_count = tmp;
                break;
            case 'o':
                os_name = optarg;
                break;
            case 'r':
                if (decode_uint(optarg, &tmp) || tmp >= 65536) {
                    fprintf(stderr, "%s: reserve: invalid numeric value\n", argv0);
                    exit(1);
                }
                reserved_sectors = tmp;
                break;
            case 's':
                if (decode_uint(optarg, &tmp) || tmp >= 256) {
                    fprintf(stderr, "%s: sectors-per-block: invalid numeric value\n", argv0);
                    exit(1);
                }
                sectors_per_block = tmp;
                break;
            case 'u':
                if (decode_uuid(optarg, &uuid)) {
                    fprintf(stderr, "%s: uuid: invalid UUID value\n", argv0);
                    exit(1);
                }
                break;
            case 'v':
                verbose = 1;
                break;
            case '?':
                print_usage();
                exit(1);
            case -1:
                break;
            default:
                abort();
        }
    } while (next_option != -1);

    for (int i = optind; i < argc; i++) {
        file = argv[i];
        break;
    }

    return make_fs(file, boot_code_file, os_name, label, rdb_count, sectors_per_block, reserved_sectors, journal_size, uuid);
}

