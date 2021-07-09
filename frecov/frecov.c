#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define bool uint8_t
#define false 0
#define true 1

// #define DEBUG

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



#define  FAT_COPIES     2
#define  BPB_SIZE       512
#define  DIR_SIZE       32
#define  MAX_CLU_NR     32768   // 硬编码的数据， 由于磁盘最多128MiB， 每个cluster的size为 8 * 512 B = 4 KiB

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
struct FAT_HEADER {
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

struct FSINFO {
  uint32_t FSI_LeadSig;        
  uint8_t  FSI_Reserved1[480]; 
  uint32_t FSI_StrucSig;       
  uint32_t FSI_Free_Count;     
  uint32_t FSI_Nxt_Free;       
  uint8_t  FSI_Reserved2[12];  
  uint32_t FSI_TrailSig;       
} __attribute__((packed));

struct FAT_TABLE {
  uint32_t next;
} __attribute__((packed));

struct FAT {
  void *fat_head;
  struct FAT_HEADER *bpb;
  struct FSINFO *fsinfo;
  struct FAT_TABLE *fat[FAT_COPIES];
  void *data;
};

struct FAT_DIR {
  uint8_t DIR_Name[11]; //short name (limited to 8 characters)
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

struct FAT_LDIR {
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
struct BMP_HEADER {
  uint16_t Signature; // 'BM'
  uint32_t FileSize; // File size in bytes
  uint32_t reserved; //0
  uint32_t DataOffset;
} __attribute__((packed));

struct BMPINFO_HEADER {
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



static struct FAT *disk;
static int label[MAX_CLU_NR] = {}; 
enum {UNLABEL = 0, DIRECT, BMPHEAD, BMPDATA, UNUSED};

void Usage() {
  printf("Invalid usage\n");
  print(FONT_RED, "Usage: frecov file");
}

unsigned char ChkSum (unsigned char *pFcbName) {
  short FcbNameLen;
  unsigned char Sum;

  Sum = 0;
  for (FcbNameLen = 11; FcbNameLen != 0; --FcbNameLen) {
    // NOTE: The operation is an unsigned char rotate right
    Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *pFcbName++;
  }
  return Sum;
}


int main(int argc, char *argv[]) {
    Assert(sizeof(struct FAT_HEADER) == BPB_SIZE, "bad header!");
    Log("%d", (int)sizeof(struct FAT_HEADER));
    
    if(argc < 2) {
      Usage();
      exit(EXIT_FAILURE);
    }

    char *img = argv[1];
    int fd = open(img, O_RDONLY);
    Assert(fd != -1, "open img file failed!");
    struct stat st;
    Assert(fstat(fd, &st) != -1, "Read img stat failed!");
    
// ====================================================================================
//                                 Initialize Disk
//=====================================================================================
    
    disk = (struct FAT*)malloc(sizeof(struct FAT));
    disk->fat_head = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);  
    
    disk->bpb =  (struct FAT_HEADER *)disk->fat_head;
    Log("Bytes per sector is %d,the number of sectors per cluster is %d", (int)disk->bpb->BPB_BytsPerSec, (int)disk->bpb->BPB_SecPerClus);
    Log("the number of FAT is %d", (int)disk->bpb->BPB_NumFATs);
    Assert(disk->bpb->Signature_word == 0xaa55, "not a valid BPB");
    
    disk->fsinfo = (struct FSINFO*)(disk->fat_head + BPB_SIZE);
    Assert(disk->fsinfo->FSI_TrailSig == 0xaa550000, "not a valid FSINFO");
    
    size_t ft_off = (size_t)(disk->bpb->BPB_BytsPerSec * disk->bpb->BPB_RsvdSecCnt); // FAT_TABLE offset
    size_t ft_sz  = (size_t)(disk->bpb->BPB_BytsPerSec * disk->bpb->BPB_FATSz32);
    disk->fat[0] = (struct FAT_TABLE*)(disk->fat_head + ft_off);
    // It's recommanded that the number of FAT table should be 2
    if(disk->bpb->BPB_NumFATs == 2)
      disk->fat[1] = (struct FAT_TABLE*)((void *)disk->fat[0] + ft_sz); // Since FAT32
    else
      disk->fat[1] = disk->fat[0];
    
    // the value of rootclus is recommended to be 2 or the first usable cluster
    size_t data_off = (ft_off + 
                      (size_t)(ft_sz * disk->bpb->BPB_NumFATs) + 
                      (size_t)((disk->bpb->BPB_RootClus - 2) * disk->bpb->BPB_SecPerClus * disk->bpb->BPB_BytsPerSec));
    disk->data = (void *)(disk->fat_head + data_off);


// ====================================================================================
//                                 Scan Cluster
//=====================================================================================

    // scan every cluster
    size_t clu_sz = disk->bpb->BPB_BytsPerSec * disk->bpb->BPB_SecPerClus;
    int dir_nr = clu_sz / DIR_SIZE;
    int tot = 0;
    for (void *clu = disk->data; clu < disk->fat_head + st.st_size; clu += clu_sz) {
      int c = UNLABEL;
      for (int i = 0; i < dir_nr; ++i) {
        // scan every dir in a cluster to judge types of the cluster
        struct FAT_DIR *dir = (struct FAT_DIR*)(clu + i * DIR_SIZE);
        if (dir->DIR_Name[0] == 0x00) break; // all dir entry following this are also free
        if (dir->DIR_Name[0] == 0xe5) continue; // empty dir
        if(i == 0 && dir->DIR_Name[0] == 'B' && dir->DIR_Name[1] == 'M') {
            c = BMPHEAD;
            break;
        }
        if(dir->DIR_Name[8] == 'B' && dir->DIR_Name[9] == 'M' && dir->DIR_Name[10] == 'P') {
            c = DIRECT;
            break;
        }
      }
      if (c == UNLABEL) c = BMPDATA; // UNUSED is regarded as BMPDATA
      label[tot++] = c;
    }
          
// ====================================================================================
//                                 Recover files
//=====================================================================================

    for (int i = 0; i < tot; ++i) {
      if(label[i] == DIRECT) {
        void *clu_addr = disk->data + clu_sz * i;
        for (int j = 0; j < dir_nr; ++j) {
          struct FAT_DIR *dir = (struct FAT_DIR*)(clu_addr + j * DIR_SIZE);
          if(j > 0 && dir->DIR_Name[8] == 'B' && dir->DIR_Name[9] == 'M' && dir->DIR_Name[10] == 'P') {
            //judge valid bmphead
            uint16_t clu_idx = (dir->DIR_FstClusHI << 16) | dir->DIR_FstClusLO;
            
            if(clu_idx < 2 || clu_idx > MAX_CLU_NR) {
              Log("invalid clu idx: %d", clu_idx);
              continue;
            }

            struct BMP_HEADER *bmphead = (struct BMP_HEADER*) (disk->data + (clu_idx - 2) * clu_sz);
            
            if(bmphead->Signature != 0x4d42){
              Log("invalid bmphead");
              continue;
            }
            if(label[clu_idx - 2] != BMPHEAD) {
              Log("Not a valid bmphead");
              continue;
            }

            uint8_t chksum = ChkSum((unsigned char*)dir->DIR_Name);
            struct FAT_LDIR *ldir = (struct FAT_LDIR *)((void *)dir - DIR_SIZE);
            if(ldir->LDIR_Attr != ATTR_LONG_NAME) continue;
            if(ldir->LDIR_Chksum != chksum) continue;

            //compute sha1sum
            uint16_t FileSize = bmphead->FileSize;
            char tmpfile[] = "/tmp/tmp_XXXXXX";
            int fd;
            char str[50], buf[50];
            

            if((size_t)(bmphead + FileSize) > (size_t)disk->fat_head + st.st_size) continue;
            Assert((fd = mkstemp(tmpfile)) != -1, "create tmp_file failed!");
            write(fd, (void *)bmphead, FileSize); // continuous storage condition!!
            close(fd);
            sprintf(str, "sha1sum %s", tmpfile);
            FILE *fp = popen(str, "r");
            Assert(fp != NULL, "popen");
            fscanf(fp, "%s", buf); // Get it!
            Log("the len of buf is %d", (int)strlen(buf));
            printf("%s ", buf); //sha1sum!
            pclose(fp);

            // find filename
            uint16_t name[1024];
            int len = 0;
            while ((size_t)ldir >= (size_t)clu_addr) {
              if ((ldir->LDIR_Ord & 0x40)) break;
              if (ldir->LDIR_Chksum == ChkSum) {
                for (int i = 0; i < 5; ++i) name[len++] = ldir->LDIR_Name1[i << 1];
                for (int i = 0; i < 6; ++i) name[len++] = ldir->LDIR_Name2[i << 1];
                for (int i = 0; i < 2; ++i) name[len++] = ldir->LDIR_Name3[i << 1];
              }
              ldir--;
            }
            for (int i = 0; i < len; ++i) {
              if(name[i] == 0x00) {
                printf(".bmp\n");
                break;
              } else {
                printf("%c", name[i]);
              }
            }
            fflush(stdout);
          }
        }
      }
    }
    munmap(disk->fat_head, st.st_size);
    close(fd);
    
    return 0;
}