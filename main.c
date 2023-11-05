#include "uefi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>
#include <zlib.h>

int
main(void)
{
	FILE *file_disk;
	file_disk = fopen("disk", "wb");
	if (file_disk == NULL) {
		fprintf(stderr, "ՍԽԼ։ չհաջողվեց ստեղծել նոր ֆայլը\n");
		exit(EXIT_FAILURE);
	}

	mbr file_disk_mbr;
	gpt_hdr file_disk_gpt_hdr_main;
	gpt_ent file_disk_gpt_tbl[128];
	gpt_hdr file_disk_gpt_hdr_back;

	const size_t file_disk_esp_size = 32 * 1024 * 1024;
	const size_t file_disk_size = 
		sizeof (file_disk_mbr) + 
		sizeof (file_disk_gpt_hdr_main) + 
		sizeof (file_disk_gpt_tbl) +
		file_disk_esp_size +
		sizeof (file_disk_gpt_tbl) + 
		sizeof (file_disk_gpt_hdr_back);

	// MBR-ը լրացնենք
	{
		bzero(&file_disk_mbr, sizeof (file_disk_mbr));

		file_disk_mbr.boot_signature = 0xAA55;
		// սկզբի LBA-ն՝ 0x002000
		file_disk_mbr.partitions[0].start_chs[0] = 0x00;
		file_disk_mbr.partitions[0].start_chs[1] = 0x20;
		file_disk_mbr.partitions[0].start_chs[2] = 0x00;
		// ՕՀ֊ի տիպը` GPT Protective
		file_disk_mbr.partitions[0].os_type = 0xEE;
		// զբաղեցնելու է աբողջ սկավառակը
		file_disk_mbr.partitions[0].end_chs[0] = 0xFF;
		file_disk_mbr.partitions[0].end_chs[1] = 0xFF;
		file_disk_mbr.partitions[0].end_chs[2] = 0xFF;
		// առաջին LBA-ն` GPT Header-ը
		file_disk_mbr.partitions[0].start_lba = 1;
		// վերջին LBA-ն, ՈՒՂՂԵԼ
		file_disk_mbr.partitions[0].size_lba = file_disk_size / LBA_SIZE;

		fwrite(&file_disk_mbr, sizeof (file_disk_mbr), 1, file_disk);
	}

	// GPT-ն լրացնենք
	{
		bzero(&file_disk_gpt_hdr_main, sizeof (file_disk_gpt_hdr_main));

		static const char efi_part[] = "EFI PART";
		memcpy(file_disk_gpt_hdr_main.blk.signature, efi_part, 8);
		// v1.0
		file_disk_gpt_hdr_main.blk.revision = 0x00010000;
		file_disk_gpt_hdr_main.blk.size = sizeof (file_disk_gpt_hdr_main.blk);
		file_disk_gpt_hdr_main.blk.crc32 = 0;
		file_disk_gpt_hdr_main.blk.my_lba = 1;
		// կրկնվող GPT header-ի LBA-ն
		file_disk_gpt_hdr_main.blk.alt_lba = file_disk_size / LBA_SIZE - 1;
		// MBR + GPT header + GPT table
		file_disk_gpt_hdr_main.blk.use_start_lba = 1 + 1 + 32;
		// GPT header + GPT table
		file_disk_gpt_hdr_main.blk.use_last_lba = (file_disk_size / LBA_SIZE - 1) - (sizeof (file_disk_gpt_tbl) + 1);
		uuid_generate((u8*)&file_disk_gpt_hdr_main.blk.disk_guid);
		// GPT table LBA` GPT header + MBR
		file_disk_gpt_hdr_main.blk.entry_lba = 2;
		file_disk_gpt_hdr_main.blk.entry_cnt = 128;
		file_disk_gpt_hdr_main.blk.entry_len = 128;
		file_disk_gpt_hdr_main.blk.entry_crc32 = 0;

		bzero(file_disk_gpt_tbl, sizeof (file_disk_gpt_tbl));

		UUID_DEFINE(type_esp_uuid, 
			0xC1, 0x2A, 0x73, 0x28, 0xF8, 0x1F, 0x11, 0xD2, 
			0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B);

		guid type_esp_guid;
		memcpy(&type_esp_guid, type_esp_uuid, sizeof (type_esp_guid));
		file_disk_gpt_tbl[0].blk.type_guid = type_esp_guid;

		uuid_generate((u8*)&file_disk_gpt_tbl[0].blk.part_guid);

		// առաջին ազատ տեղը
		file_disk_gpt_tbl[0].blk.start_lba = file_disk_gpt_hdr_main.blk.use_start_lba;
		file_disk_gpt_tbl[0].blk.end_lba = 32 * 1024 * 1024 / LBA_SIZE;

		static const char esp_name[] = "EFI SYSTEM PARTITION";
		memcpy(file_disk_gpt_tbl[0].blk.name, esp_name, sizeof (esp_name));

		u32 crc;
		crc = crc32(0, Z_NULL, 0);
		file_disk_gpt_hdr_main.blk.entry_crc32 = crc32(crc, (u8*)&file_disk_gpt_tbl, sizeof (file_disk_gpt_tbl));

		crc = crc32(0, Z_NULL, 0);
		file_disk_gpt_hdr_main.blk.crc32 = crc32(crc, (u8*)&file_disk_gpt_hdr_main, sizeof (file_disk_gpt_hdr_main));

		fwrite(&file_disk_gpt_hdr_main, sizeof (file_disk_gpt_hdr_main), 1, file_disk);
		fwrite(&file_disk_gpt_tbl, sizeof (file_disk_gpt_tbl), 1, file_disk);

		// backup-ը գրենք
		file_disk_gpt_hdr_back = file_disk_gpt_hdr_main;

		file_disk_gpt_hdr_back.blk.crc32 = 0;
		file_disk_gpt_hdr_back.blk.entry_crc32 = 0;

		file_disk_gpt_hdr_back.blk.my_lba = file_disk_gpt_hdr_main.blk.alt_lba;
		file_disk_gpt_hdr_back.blk.alt_lba = file_disk_gpt_hdr_main.blk.my_lba;
		file_disk_gpt_hdr_back.blk.entry_lba = file_disk_gpt_hdr_main.blk.use_last_lba + 1;

		crc = crc32(0, Z_NULL, 0);
		file_disk_gpt_hdr_back.blk.entry_crc32 = crc32(crc, (u8*)&file_disk_gpt_tbl, sizeof (file_disk_gpt_tbl));

		crc = crc32(0, Z_NULL, 0);
		file_disk_gpt_hdr_back.blk.crc32 = crc32(crc, (u8*)&file_disk_gpt_hdr_back, sizeof (file_disk_gpt_hdr_back));

		fwrite(&file_disk_gpt_tbl, sizeof (file_disk_gpt_tbl), 1, file_disk);
		fwrite(&file_disk_gpt_hdr_back, sizeof (file_disk_gpt_hdr_back), 1, file_disk);
	}

	fclose(file_disk);

	return EXIT_SUCCESS;
}
