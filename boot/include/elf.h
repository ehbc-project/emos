#ifndef __ELF_H__
#define __ELF_H__

#include <stdint.h>
#include <stdio.h>

#include <compiler.h>

#define ELF_MAGIC               "\x7FELF"

#define ELF_OSABI_NONE          0x00
#define ELF_OSABI_SYSV          0x00
#define ELF_OSABI_HPUX          0x01
#define ELF_OSABI_NETBSD        0x02
#define ELF_OSABI_LINUX         0x03
#define ELF_OSABI_HURD          0x04
#define ELF_OSABI_SOLARIS       0x06
#define ELF_OSABI_AIX           0x07
#define ELF_OSABI_IRIX          0x08
#define ELF_OSABI_FREEBSD       0x09
#define ELF_OSABI_TRU64         0x0A
#define ELF_OSABI_MODESTO       0x0B
#define ELF_OSABI_OPENBSD       0x0C
#define ELF_OSABI_OPENVMS       0x0D
#define ELF_OSABI_NONSTOP       0x0E
#define ELF_OSABI_AROS          0x0F
#define ELF_OSABI_FENIXOS       0x10
#define ELF_OSABI_CLOUDABI      0x11
#define ELF_OSABI_OPENVOS       0x12
#define ELF_OSABI_ARM           0x40
#define ELF_OSABI_AEABI         0x61
#define ELF_OSABI_STANDALONE    0xFF

#define ELF_TYPE_NONE           0x00
#define ELF_TYPE_REL            0x01
#define ELF_TYPE_EXEC           0x02
#define ELF_TYPE_DYN            0x03
#define ELF_TYPE_CORE           0x04
#define ELF_TYPE_LOOS           0xFE00
#define ELF_TYPE_HIOS           0xFEFF
#define ELF_TYPE_LOPROC         0xFF00
#define ELF_TYPE_HIPROC         0xFFFF

#define ELF_MACH_NONE           0x00
#define ELF_MACH_M32            0x01
#define ELF_MACH_SPARC          0x02
#define ELF_MACH_386            0x03
#define ELF_MACH_68K            0x04
#define ELF_MACH_88K            0x05
#define ELF_MACH_IAMCU          0x06
#define ELF_MACH_860            0x07
#define ELF_MACH_MIPS           0x08
#define ELF_MACH_S370           0x09
#define ELF_MACH_MIPS_RS3_LE    0x0A
#define ELF_MACH_PARISC         0x0F
#define ELF_MACH_VPP500         0x11
#define ELF_MACH_SPARC32PLUS    0x12
#define ELF_MACH_960            0x13
#define ELF_MACH_PPC            0x14
#define ELF_MACH_PPC64          0x15
#define ELF_MACH_S390           0x16
#define ELF_MACH_SPU            0x17
#define ELF_MACH_V800           0x24
#define ELF_MACH_FR20           0x25
#define ELF_MACH_RH32           0x26
#define ELF_MACH_RCE            0x27
#define ELF_MACH_ARM            0x28
#define ELF_MACH_FAKE_ALPHA     0x29
#define ELF_MACH_SH             0x2A
#define ELF_MACH_SPARCV9        0x2B
#define ELF_MACH_TRICORE        0x2C
#define ELF_MACH_ARC            0x2D
#define ELF_MACH_H8_300         0x2E
#define ELF_MACH_H8_300H        0x2F
#define ELF_MACH_H8S            0x30
#define ELF_MACH_H8_500         0x31
#define ELF_MACH_IA_64          0x32
#define ELF_MACH_MIPS_X         0x33
#define ELF_MACH_COLDFIRE       0x34
#define ELF_MACH_M68HC12        0x35
#define ELF_MACH_MMA            0x36
#define ELF_MACH_PCP            0x37
#define ELF_MACH_NCPU           0x38
#define ELF_MACH_NDR1           0x39
#define ELF_MACH_STARCORE       0x3A
#define ELF_MACH_ME16           0x3B
#define ELF_MACH_ST100          0x3C
#define ELF_MACH_TINYJ          0x3D
#define ELF_MACH_X86_64         0x3E
#define ELF_MACH_PDSP           0x3F
#define ELF_MACH_PDP10          0x40
#define ELF_MACH_PDP11          0x41
#define ELF_MACH_FX66           0x42
#define ELF_MACH_ST9PLUS        0x43
#define ELF_MACH_ST7            0x44
#define ELF_MACH_MC68HC16       0x45
#define ELF_MACH_MC68HC11       0x46
#define ELF_MACH_MC68HC08       0x47
#define ELF_MACH_MC68HC05       0x48
#define ELF_MACH_SVX            0x49
#define ELF_MACH_ST19           0x4A
#define ELF_MACH_VAX            0x4B
#define ELF_MACH_CRIS           0x4C
#define ELF_MACH_JAVELIN        0x4D
#define ELF_MACH_FIREPATH       0x4E
#define ELF_MACH_ZSP            0x4F
#define ELF_MACH_MMIX           0x50
#define ELF_MACH_HUANY          0x51
#define ELF_MACH_PRISM          0x52
#define ELF_MACH_AVR            0x53
#define ELF_MACH_FR30           0x54
#define ELF_MACH_D10V           0x55
#define ELF_MACH_D30V           0x56
#define ELF_MACH_V850           0x57
#define ELF_MACH_M32R           0x58
#define ELF_MACH_MN10300        0x59
#define ELF_MACH_MN10200        0x5A
#define ELF_MACH_PJ             0x5B
#define ELF_MACH_OPENRISC       0x5C
#define ELF_MACH_ARC_COMPACT    0x5D
#define ELF_MACH_XTENSA         0x5E
#define ELF_MACH_VIDEOCORE      0x5F
#define ELF_MACH_TMM_GPP        0x60
#define ELF_MACH_NS32K          0x61
#define ELF_MACH_TPC            0x62
#define ELF_MACH_SNP1K          0x63
#define ELF_MACH_ST200          0x64
#define ELF_MACH_IP2K           0x65
#define ELF_MACH_MAX            0x66
#define ELF_MACH_CR             0x67
#define ELF_MACH_F2MC16         0x68
#define ELF_MACH_MSP430         0x69
#define ELF_MACH_BLACKFIN       0x6A
#define ELF_MACH_SE_C33         0x6B
#define ELF_MACH_SEP            0x6C
#define ELF_MACH_ARCA           0x6D
#define ELF_MACH_UNICORE        0x6E
#define ELF_MACH_EXCESS         0x6F
#define ELF_MACH_DXP            0x70
#define ELF_MACH_ALTERA_NIOS2   0x71
#define ELF_MACH_CRX            0x72
#define ELF_MACH_XGATE          0x73
#define ELF_MACH_C166           0x74
#define ELF_MACH_M16C           0x75
#define ELF_MACH_DSPIC30F       0x76
#define ELF_MACH_CE             0x77
#define ELF_MACH_M32C           0x78
#define ELF_MACH_M32C           0x78
#define ELF_MACH_TSK3000        0x83
#define ELF_MACH_RS08           0x84
#define ELF_MACH_SHARC          0x85
#define ELF_MACH_ECOG2          0x86
#define ELF_MACH_SCORE7         0x87
#define ELF_MACH_DSP24          0x88
#define ELF_MACH_VIDEOCORE3     0x89
#define ELF_MACH_LATTICEMICO32  0x8A
#define ELF_MACH_SE_C17         0x8B
#define ELF_MACH_TI_C6000       0x8C
#define ELF_MACH_TI_C2000       0x8D
#define ELF_MACH_TI_C5500       0x8E
#define ELF_MACH_TI_ARP32       0x8F
#define ELF_MACH_TI_PRU         0x90
#define ELF_MACH_MMDSP_PLUS     0xA0
#define ELF_MACH_CYPRESS_M8C    0xA1
#define ELF_MACH_R32C           0xA2
#define ELF_MACH_TRIMEDIA       0xA3
#define ELF_MACH_QDSP6          0xA4
#define ELF_MACH_8051           0xA5
#define ELF_MACH_STXP7X         0xA6
#define ELF_MACH_NDS32          0xA7
#define ELF_MACH_ECOG1X         0xA8
#define ELF_MACH_MAXQ30         0xA9
#define ELF_MACH_XIMO16         0xAA
#define ELF_MACH_MANIK          0xAB
#define ELF_MACH_CRAYNV2        0xAC
#define ELF_MACH_RX             0xAD
#define ELF_MACH_METAG          0xAE
#define ELF_MACH_MCST_ELBRUS    0xAF
#define ELF_MACH_ECOG16         0xB0
#define ELF_MACH_CR16           0xB1
#define ELF_MACH_ETPU           0xB2
#define ELF_MACH_SLE9X          0xB3
#define ELF_MACH_L10M           0xB4
#define ELF_MACH_K10M           0xB5
#define ELF_MACH_AARCH64        0xB7
#define ELF_MACH_AVR32          0xB9
#define ELF_MACH_STM8           0xBA
#define ELF_MACH_TILE64         0xBB
#define ELF_MACH_TILEPRO        0xBC
#define ELF_MACH_MICROBLAZE     0xBD
#define ELF_MACH_CUDA           0xBE
#define ELF_MACH_TILEGX         0xBF
#define ELF_MACH_CLOUDSHIELD    0xC0
#define ELF_MACH_COREA_1ST      0xC1
#define ELF_MACH_COREA_2ND      0xC2
#define ELF_MACH_ARCV2          0xC3
#define ELF_MACH_OPEN8          0xC4
#define ELF_MACH_RL78           0xC5
#define ELF_MACH_VIDEOCORE5     0xC6
#define ELF_MACH_78KOR          0xC7
#define ELF_MACH_56800EX        0xC8
#define ELF_MACH_BA1            0xC9
#define ELF_MACH_BA2            0xCA
#define ELF_MACH_XCORE          0xCB
#define ELF_MACH_MCHP_PIC       0xCC
#define ELF_MACH_INTELGT        0xCD
#define ELF_MACH_KM32           0xD2
#define ELF_MACH_KMX32          0xD3
#define ELF_MACH_EMX16          0xD4
#define ELF_MACH_EMX8           0xD5
#define ELF_MACH_KVARC          0xD6
#define ELF_MACH_CDP            0xD7
#define ELF_MACH_CDGE           0xD8
#define ELF_MACH_COOL           0xD9
#define ELF_MACH_NORC           0xDA
#define ELF_MACH_CSR_KALIMBA    0xDB
#define ELF_MACH_Z80            0xDC
#define ELF_MACH_VISIUM         0xDD
#define ELF_MACH_FT32           0xDE
#define ELF_MACH_MOXIE          0xDF
#define ELF_MACH_AMDGPU         0xE0
#define ELF_MACH_RISCV          0xF3
#define ELF_MACH_BPF            0xF7
#define ELF_MACH_CSKY           0xFC
#define ELF_MACH_65C816         0x101
#define ELF_MACH_LOONGARCH      0x102
#define ELF_MACH_ALPHA          0x9026

struct elf_header {
    char magic[4];
    uint8_t class;
    uint8_t endianness;
    uint8_t header_version;
    uint8_t osabi;
    uint8_t abi_version;
    uint8_t pad[7];
    uint16_t type;
    uint16_t machine;
    uint32_t version;

    union {
        struct {
            uint32_t entry;
            uint32_t phoff;
            uint32_t shoff;
            uint32_t flags;
            uint16_t ehsize;
            uint16_t phentsize;
            uint16_t phnum;
            uint16_t shentsize;
            uint16_t shnum;
            uint16_t shstrndx;
        } __packed elf32;

        struct {
            uint64_t entry;
            uint64_t phoff;
            uint64_t shoff;
            uint32_t flags;
            uint16_t ehsize;
            uint16_t phentsize;
            uint16_t phnum;
            uint16_t shentsize;
            uint16_t shnum;
            uint16_t shstrndx;
        } __packed elf64;
    } __packed;
} __packed;

#define PHDR_TYPE_NULL          0            /* Program header table entry unused */
#define PHDR_TYPE_LOAD          1            /* Loadable program segment */
#define PHDR_TYPE_DYNAMIC       2            /* Dynamic linking information */
#define PHDR_TYPE_INTERP        3            /* Program interpreter */
#define PHDR_TYPE_NOTE          4            /* Auxiliary information */
#define PHDR_TYPE_SHLIB         5            /* Reserved */
#define PHDR_TYPE_PHDR          6            /* Entry for header table itself */
#define PHDR_TYPE_TLS           7            /* Thread-local storage segment */
#define PHDR_TYPE_NUM           8            /* Number of defined types */
#define PHDR_TYPE_LOOS          0x60000000    /* Start of OS-specific */
#define PHDR_TYPE_GNU_EH_FRAME  0x6474e550    /* GCC .eh_frame_hdr segment */
#define PHDR_TYPE_GNU_STACK     0x6474e551    /* Indicates stack executability */
#define PHDR_TYPE_GNU_RELRO     0x6474e552    /* Read-only after relocation */
#define PHDR_TYPE_LOSUNW        0x6ffffffa
#define PHDR_TYPE_SUNWBSS       0x6ffffffa    /* Sun Specific segment */
#define PHDR_TYPE_SUNWSTACK     0x6ffffffb    /* Stack segment */
#define PHDR_TYPE_HISUNW        0x6fffffff
#define PHDR_TYPE_HIOS          0x6fffffff    /* End of OS-specific */
#define PHDR_TYPE_LOPROC        0x70000000    /* Start of processor-specific */
#define PHDR_TYPE_HIPROC        0x7fffffff    /* End of processor-specific */

#define PHDR_FLAG_X             (1 << 0)    /* Segment is executable */
#define PHDR_FLAG_W             (1 << 1)    /* Segment is writable */
#define PHDR_FLAG_R             (1 << 2)    /* Segment is readable */
#define PHDR_FLAG_MASKOS        0x0ff00000    /* OS-specific */
#define PHDR_FLAG_MASKPROC      0xf0000000    /* Processor-specific */

struct elf_program_header {
    uint32_t type;

    union {
        struct {
            uint32_t offset;
            uint32_t vaddr;
            uint32_t paddr;
            uint32_t filesz;
            uint32_t memsz;
            uint32_t flags;
            uint32_t addralign;
        } __packed elf32;

        struct {
            uint32_t flags;
            uint64_t offset;
            uint64_t vaddr;
            uint64_t paddr;
            uint64_t filesz;
            uint64_t memsz;
            uint64_t addralign;
        } __packed elf64;
    } __packed;
} __packed;

struct elf_section_header {
    uint32_t name;
    uint32_t type;

    union {
        struct {
            uint32_t flags;
            uint32_t address;
            uint32_t offset;
            uint32_t size;
            uint32_t link;
            uint32_t info;
            uint32_t addralign;
            uint32_t entry_size;
        } __packed elf32;

        struct {
            uint64_t flags;
            uint64_t address;
            uint64_t offset;
            uint64_t size;
            uint32_t link;
            uint32_t info;
            uint64_t addralign;
            uint64_t entry_size;
        } __packed elf64;
    } __packed;
} __packed;

struct elf_file {
    FILE *fp;
    struct elf_header ehdr;
    uint32_t phdr_idx;
};

void elf_close(struct elf_file *elf);

__malloc_like(elf_close)
struct elf_file *elf_open(const char *path);

void elf_reset_current_program_header(struct elf_file *elf);
int elf_get_current_program_header(struct elf_file *__restrict elf, struct elf_program_header *__restrict phdr);
int elf_load_current_program_header(struct elf_file *__restrict elf);
int elf_advance_program_header(struct elf_file *elf);



#endif // __ELF_H__
