#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define bool uint8_t
#define false 0
#define true 1

#define DEBUG

#define  FONT_BLACK          "\033[1;30m"
#define  FONT_RED            "\033[1;31m"
#define  FONT_GREEN          "\033[1;32m"
#define  FONT_YELLOW         "\033[1;33m"
#define  FONT_BLUE           "\033[1;34m"
#define  FONT_PURPLE         "\033[1;35m"
#define  FONT_CYAN           "\033[1;36m"
#define  FONT_WHITE          "\033[1;37m"

#define  FONT_RESET          "\033[0m"

#define print(FONT_COLOR, format, ...) \
    printf(FONT_COLOR format FONT_RESET "\n", \
        ## __VA_ARGS__)

#ifdef  DEBUG 
#define Log(format, ...) \
    printf(FONT_BLUE "[%s,%d,%s] " format FONT_RESET"\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#define Log(format, ...)
#endif

#ifdef  DEBUG
  #define Assert(cond, format, ...) \
    do { \
      if (!(cond)) { \
        Log(format, ## __VA_ARGS__); \
        exit(1); \
      } \
    } while (0)
#else
#define Assert(cond, format, ...)\
    do { \
      if (!(cond)) { \
        exit(1); \
      } \
    } while (0)
#endif


#define  ATTR_READ_ONLY 0x01
#define  ATTR_HIDDEN    0x02
#define  ATTR_SYSTEM    0x04
#define  ATTR_VOLUME_ID 0x08
#define  ATTR_DIRECTORY 0x10
#define  ATTR_ARCHIVE   0x20

#define ATTR_LONG_NAME \
 (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

#define ATTR_LONG_NAME_MASK \
    (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | \
        ATTR_DIRECTORY | ATTR_ARCHIVE)

// bios paremeter block(BPB)
// the first sector of the volume
struct fat_header {
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    // Extended BPB structure for FAT32 volumes
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo; // sector number of FSINFO structure
    uint16_t BPB_BkBootSec; //BPB的备份所在的sector FAT32中一般是6
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VollD;
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8];
    uint8_t  padding[420];
    uint16_t Signature_word;
} __attribute__((packed));

struct fat_dir {
  uint8_t DIR_Name[11]; //short name (limited to 11 characters)
  uint8_t DIR_Attr;
  uint8_t DIR_NTRes;
  uint8_t DIR_CrtTimeTenth;
  uint16_t DIR_CrtTime;
  uint16_t DIR_CrtDate;
  uint16_t DIR_LstAccDate;
  uint16_t DIR_FstClusHI;
  uint16_t DIR_WrtTime;
  uint16_t DIR_WrtDate;
  uint16_t DIR_FstClusLO;
  uint32_t DIR_FileSize;
 } __attribute__((packed));

struct fat_ldir {
  uint8_t LDIR_Ord;
  uint8_t LDIR_Name1[10];
  uint8_t LDIR_Attr;
  uint8_t LDIR_Type;
  uint8_t LDIR_Chksum;
  uint8_t LDIR_Name2[12];
  uint16_t LDIR_FstClusLO;
  uint8_t LDIR_Name3[4];
} __attribute__((packed));


// http://www.ece.ualberta.ca/~elliott/ee552/studentAppNotes/2003_w/misc/bmp_file_format/bmp_file_format.htm
struct bmp_header {
  uint16_t Signature; // 'BM'
  uint32_t FileSize; // File size in bytes
  uint32_t reserved; //0
  uint32_t DataOffset;
} __attribute__((packed));

struct bmp_infoheader {
  uint32_t Size;
  int32_t  Width;  
  int32_t  Height; 
  uint16_t Planes;
  uint16_t BitsPerPixel;
  uint32_t Compression;
  uint32_t ImageSize;
  int32_t  XpixelsPerM;
  int32_t  YpixelsPerM;
  uint32_t ColorsUsed;
  uint32_t ImportantColors;
} __attribute__((packed));

void Usage() {
  printf("Invalid usage\n");
  print(FONT_RED, "Usage: frecov file");
}


int main(int argc, char *argv[]) {
    Assert(sizeof(struct fat_header) == 512, "bad header!");
    Log("%d", (int)sizeof(struct fat_header));
    
    if(argc < 2) {
      Usage();
      exit(EXIT_FAILURE);
    }

    char *img = argv[1];
    int fd = open(img, O_RDONLY);
    Assert(fd != -1, "open img file failed!");
    struct stat st;
    Assert(fstat(fd, &st) != -1, "Read img stat failed!");
    
    void *FAT_HEAD = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);  
    
    struct fat_header* BPB =  (struct fat_header *)FAT_HEAD;
    Log("Bytes per sector is %d,the number of sectors per cluster is %d", (int)BPB->BPB_BytsPerSec, (int)BPB->BPB_SecPerClus);
    Log("the number of FAT is %d", (int)BPB->BPB_NumFATs);
    Assert(BPB->Signature_word == 0xaa55, "not a valid fat");
    munmap(FAT_HEAD, st.st_size);
    close(fd);
    
    return 0;
}
