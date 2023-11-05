#ifndef _UEFI_H_
#define _UEFI_H_

#include <stdint.h>

typedef int8_t	 s8;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define LBA_SIZE 512

typedef struct guid_s {
	u32 data1;
	u16 data2;
	u16 data3;
	u8  data4[8];
} guid;

typedef struct mbr_part_s {
	u8  indicator;
	u8  start_chs[3];
	u8  os_type;
	u8  end_chs[3];
	u32 start_lba;
	u32 size_lba;
} __attribute__((packed)) mbr_part;

typedef struct mbr_s {
	u8		boot_code[440];
	u32		mbr_signature;
	u16		uknown;
	mbr_part	partitions[4];
	u16		boot_signature;
} __attribute__((packed)) mbr;

typedef struct gpt_hdr_blk_s {
	s8   signature[8];
	u32  revision;
	u32  size;
	u32  crc32;
	u32  reserved;
	u64  my_lba;
	u64  alt_lba;
	u64  use_start_lba;
	u64  use_last_lba;
	guid disk_guid;
	u64  entry_lba;
	u32  entry_cnt;
	u32  entry_len;
	u32  entry_crc32;
} __attribute__((packed)) gpt_hdr_blk;

typedef struct gpt_hdr_s {
	gpt_hdr_blk	blk;
	u8		reserved[LBA_SIZE - sizeof (gpt_hdr_blk)];
} __attribute__((packed)) gpt_hdr;

typedef struct gpt_ent_blk_s {
	guid type_guid;
	guid part_guid;
	u64  start_lba;
	u64  end_lba;
	u64  attrs;
	s8   name[72];
} __attribute__((packed)) gpt_ent_blk;

typedef struct gpt_ent_s {
	gpt_ent_blk	blk;
	u8		reserved[LBA_SIZE - sizeof (gpt_ent_blk)];
} __attribute__((packed)) gpt_ent;

#endif
