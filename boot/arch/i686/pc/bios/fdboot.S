    .set    MAGIC,          0xAA55
    .set    STACK_INIT,     0x7C00
    .set    LOAD_SEGMENT,   0x07E0
    .set    ENTRY_ADDR,     0x7E00

    .org    0

    .section .text.startup
    .code16
    .globl  _start
_start:
    jmp     .start1                     # EB 3C
    nop                                 # 90

    .globl  .bpb_oem_name
.bpb_oem_name:
    .ascii  "EMOS    "

.bpb_bytes_per_sector:
    .word   0x0000

.bpb_sectors_per_cluster:
    .byte   0x00

.bpb_reserved_sectors:
    .word   0x0000

.bpb_fat_count:
    .byte   0x00

.bpb_root_entries:
    .word   0x0000

.bpb_total_sectors_16:
    .word   0x0000

.bpb_media_type:
    .byte   0x00

.bpb_sectors_per_fat:
    .word   0x0000

.bpb_sectors_per_track:
    .word   0x0000

.bpb_head_count:
    .word   0x0000

.bpb_hidden_sectors:
    .long   0x00000000

.bpb_total_sectors_32:
    .long   0x00000000

.bpb_drive_number:
    .byte   0x00

.bpb_reserved1:
    .byte   0x00

.bpb_boot_signature:
    .byte   0x00

.bpb_volume_id:
    .long   0x00000000

.bpb_volume_label:
    .ascii  "           "

.bpb_file_system_type:
    .ascii  "FAT12   "
    
.start1:
    xor     %ax, %ax                    # clear segment registers
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %ss
    mov     $STACK_INIT, %sp            # setup stack pointer
    mov     %sp, %bp

.get_drive_geometry:
    push    %dx
    mov     %dl, .bpb_drive_number

    mov     $0x08, %ah                  # BIOS Int 13h, AH=0x08: Get Drive Geometry
    int     $0x13                       # do it!
    jc      print_error

    xor     %ax, %ax
    mov     %ax, %es

    mov     %dh, %dl
    xor     %dh, %dh
    inc     %dx
    mov     %dx, .bpb_head_count

    and     $0x3F, %cx
    mov     %cx, .bpb_sectors_per_track

    pop     %dx

    # read all reserved sectors to 0x7E00
    mov     $LOAD_SEGMENT, %ax
    mov     %ax, %es
    xor     %ecx, %ecx

.load_next:
    inc     %ecx
    cmp     %cx, .bpb_reserved_sectors
    jle     .jump_next

    mov     $1, %ax
    xor     %bx, %bx
    push    %es
    push    %ecx
    call    read_disk
    pop     %ecx

    pop     %ax
    add     $0x20, %ax
    mov     %ax, %es

    jmp     .load_next


.jump_next:
    xor     %dx, %dx
    mov     .bpb_drive_number, %dl

    jmp     ENTRY_ADDR


read_disk:
    # Read the disk using the BIOS interrupt call
    # Parameters:
    #   AL: number of sectors to read
    #   ECX: LBA (relative)
    #   DL: drive number
    #   ES:BX: pointer to buffer
    pusha

    push    %ax
    push    %bx

    add     .bpb_hidden_sectors, %ecx

    mov     %ecx, %eax
    and     $0x0000FFFF, %eax
    mov     %ecx, %edx
    shr     $16, %edx

    divw    .bpb_sectors_per_track       # temp = LBA / sectors_per_track
    inc     %dx
    mov     %dx, %bx                     # sector = (LBA % sectors_per_track) + 1

    xor     %dx, %dx
    divw    .bpb_head_count              # head = temp % head_count
                                         # cylinder = temp / head_count

    xchg    %al, %ah
    shl     $6, %al
    or      %bl, %al

    mov     %ax, %cx

    mov     %dl, %dh
    mov     .bpb_drive_number, %dl

    pop     %bx
    pop     %ax
    mov     $0x02, %ah                  # BIOS Int 13h, AH=0x02: Read
    int     $0x13

    mov     $str_diskerror, %si
    jc      print_error

    popa
    ret

print_error:
    # Print error and halt
    # Parameters:
    #   DS:SI: error string
    mov     $0x0E, %ah
    xor     %bx, %bx                    # to page 0

print_error.1:
    lodsb
    or      %al, %al
    jz      halt                        # until the char is NULL

    int     $0x10                       # do it!
    jmp     print_error.1               # print next character

halt:
    hlt
    jmp     halt                        # halt

str_diskerror: 
    .asciz  "disk error\r\n"

    .org    0x1FE                       # 512 - 2
    .word   MAGIC                       # magic number
