#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

void readPayload(){
	int offset = 5 + buffer[4];
	if (pid == 0x0000){
		if (buffer[offset] != 0x00){
			printf("Something's wrong!! It should be PAT\n");
		}
		uint16_t const transport_stream_id = ((buffer[offset + 3] << 8) & 0xFF00) | (buffer[offset + 4] & 0xFF);
		uint8_t section_number = buffer[offset + 6];
		uint8_t const last_section_number = buffer[offset + 7];
		printf("Table id : %x\n", buffer[offset] & 0xFF);
		printf("Section syntax indicator : %x\n", (buffer[offset + 1] & 0x80) >> 7);
		printf("Section length : %d\n", ((buffer[offset + 1] << 8) & 0x1F00) | (buffer[offset + 2] & 0xFF));
		printf("Transport stream id : %d\n", transport_stream_id);
		printf("Version number : %d\n", buffer[offset + 5] & 0x3E);
		printf("Current next indicator : %d\n", buffer[offset + 5] & 0x1);
		printf("Section number : %d\n", section_number);
		printf("Last section number : %d\n", last_section_number);

		for (; section_number <= last_section_number; section_number++){
			unsigned const program_map_pid = ((buffer[offset + 10 + 4 * section_number] << 8 ) & 0x1F00 ) | (buffer[offset + 11 + 4 * section_number] & 0xFF);
			printf("program number %d : %d -> %d\n",
				section_number, (buffer[offset + 8 + 4 * section_number] << 8 | buffer[offset + 9 + 4 * section_number]),
				program_map_pid);
		}

		printf("CRC 32 : %x\n", (buffer[offset + 12 + 4 * last_section_number] << 24) | ((buffer[offset + 13 + 4 * last_section_number] << 16) & 0xFF0000) |
			((buffer[offset + 14 + 4 * last_section_number] << 8) & 0xFF00) | (buffer[offset + 15 + 4 * last_section_number] & 0xFF));
	} else if (pid == 0x0001){
		/* CAT */
	} else if (pid < 0x0010){
		/* Reserved */
	} else if (pid < 0x1FFF){
		if (buffer[offset] == 0x42){
			printf("service_description_section\n");
		} else{
			if (buffer[offset] == 0x0 && buffer[offset + 1] == 0x0 && buffer[offset + 2] == 0x1){
				printf("Stream id : %d\n", buffer[offset + 3] & 0xFF);
				printf("PES packet length : %d\n", ((buffer[offset + 4] << 8) & 0xFF00) | (buffer[offset + 5] & 0xFF));
				if ((buffer[offset + 6] & 0xC0) >> 6 != 0x2){
					printf("PES_scrambling_control : %x\n", (buffer[offset + 6] & 0x3F) >> 4);
					printf("PES priority : %x\n", (buffer[offset + 6] & 0x8) >> 3);
					printf("Data alignment indicator : %x\n", (buffer[offset + 6] & 0x4) >> 2);
					printf("Copyright : %x\n", (buffer[offset + 6] & 0x2) >> 1);
					printf("Original or copy : %x\n", buffer[offset + 6] & 0x1);
					printf("PTS_DTS_flag", (buffer[offset + 7] & 0xC0) >> 6);
					printf("ESCR_flag", (buffer[offset + 7] & 0x20) >> 5);
					printf("ES_rate_flag", (buffer[offset + 7] & 0x10) >> 4);
					printf("DSM_trick_flag", (buffer[offset + 7] & 0x8) >> 3);
					printf("additional_copy_into_flag", (buffer[offset + 7] & 0x4) >> 2);
					printf("PES_CRC_flag", (buffer[offset + 7] & 0x2) >> 1);
					printf("PES_extension_flag", buffer[offset + 7] & 0x1);
				}
			} else{
				
			}
		}
	} else{
		printf("It's Null packet\n");
	}
}

void readAdaptationField(){
	uint8_t const discontinuity_indicator = buffer[5] & 0x80;
	uint8_t const pcrFlag = (buffer[5] & 0x10) >> 4;
	uint8_t const opcrFlag = (buffer[5] & 0x8) >> 3;
	uint8_t const spFlag = (buffer[5] & 0x4) >> 2;
	uint8_t const tpdFlag = (buffer[5] & 0x2) >> 1;
	uint8_t const afeFlag = buffer[5] & 0x1;
	int offset = 0;

	printf("Adaptation field length : %d\n", buffer[4] & 0xFF);								/* Number of bytes in the adaptation field following this byte */
	printf("Discontinuity indicator : %x\n", (buffer[5] & 0x80) >> 7);						/* If it is 1, discontinuity state is true */
	printf("Random access indicator : %x\n", (buffer[5] & 0x40) >> 6);						/* If it is 1, TS contains some information to aid random access */
	printf("Elementary stream priority indicator : %x\n", (buffer[5] & 0x20) >> 5);			/* If it is 1, the payload has a higher priority than others */
	printf("PCR_flag  : %d\n", pcrFlag);													/* If it is 1, optional fields exist */
	printf("OPCR_flag  : %d\n", opcrFlag);
	printf("splicing_point_flag  : %d\n", spFlag);
	printf("transport_private_data_flag  : %d\n", tpdFlag);
	printf("adaptation_field_extension_flag  : %d\n", afeFlag);
	if (pcrFlag){
		uint32_t const pcrBase = (buffer[6] << 24) | ((buffer[7] << 16) & 0xFF0000)| ((buffer[8] << 8) & 0xFF00) | (buffer[9] & 0xFF) ; 
		unsigned short const pcrExt = ((buffer[10] & 0x01) << 8) | (buffer[11] & 0xFF);
		printf("Program clock reference base : %d\n", pcrBase);
		printf("Program clock reference extension : %d\n", pcrExt);
	}
	if (opcrFlag){
		uint32_t const org_pcrBase = (buffer[12] << 24) | ((buffer[13] << 16) & 0xFF0000) | ((buffer[14] << 8) & 0xFF00) | (buffer[15] & 0xFF);
		unsigned short const org_pcrExt = ((buffer[16] & 0x01) << 8) | (buffer[17] & 0xFF);
		printf("Original program clock reference base : %d\n", org_pcrBase);
		printf("Original program clock reference extension : %d\n", org_pcrExt);
	}
	if (spFlag){
		printf("Splice countdown : %d\n", buffer[18] & 0xFF);
	}
	if (tpdFlag){
		uint8_t const transport_private_data_length = offset = buffer[19] & 0xFF;
		printf("Transport private data length : %d\n", transport_private_data_length);
		int i;
		for (i = 0; i < transport_private_data_length; i++){
			printf("private data : %x\n", buffer[20 + i] & 0xFF);
		}
	}
	if (afeFlag){
		uint8_t const adaptation_field_extension_length = buffer[20 + offset] & 0xFF;
		uint8_t const itw_flag = (buffer[21 + offset] & 0x80) >> 7;
		uint8_t const piecewise_rate_flag = (buffer[21 + offset] & 0x40) >> 6;
		uint8_t const seamless_splice_flag = (buffer[21 + offset] & 0x20) >> 5;
		printf("Adaptation field extension length : %d\n", adaptation_field_extension_length);
		printf("itw_flag : %d\n", itw_flag);
		printf("piecewise_rate_flag : %d\n", piecewise_rate_flag);
		printf("seamless_splice_flag : %d\n", seamless_splice_flag);
		if (itw_flag){
			printf("Itw_valid_flag : %d\n", (buffer[22 + offset] & 0x80) >> 7);
			printf("itw_offset : %d\n", (buffer[22 + offset] & 0x7F) | buffer[23 + offset]);
		}
		if (piecewise_rate_flag){
			printf("Piecewise_rate : %d\n", 
				(buffer[24 + offset] & 0x3F) << 24) | (buffer[25 + offset] << 16) | (buffer[26 + offset] << 8) | (buffer[27 + offset]);
		}
		if (seamless_splice_flag){
			unsigned const DTS = (((buffer[28 + offset] & 0xD) << 5) | ((buffer[29 + offset] & 0xF8) >> 3) << 24) | 
				((buffer[29 + offset] & 0x7) << 5) | ((buffer[30 + offset] & 0xF8) >> 3) | 
				((buffer[30 + offset] & 0x6) << 6) | ((buffer[31 + offset] & 0xFC) >> 2) | 
				((buffer[31 + offset] & 0x3) << 6) | ((buffer[32 + offset] & 0xFC) >> 2);
			printf("Splice type : %d\n", buffer[28 + offset] & 0xF0);
			printf("DTS_next_au : %x\n", DTS);
		}
	}
	
}

void readTsHeader(){
	if (buffer[0] != TRANSPORT_SYNC_BYTE){
		printf("Missing Sync byte here.\n");
	}
	if ((buffer[1] & 0x80) != 0x0){
		printf("Transport error indicator should be zero.\n");
	}
	uint8_t adaptation_field_control = (buffer[3] & 0x30) >> 4;
	printf("Sync byte : %x\n", buffer[0]);
	printf("Transport error indicator : %x\n", (buffer[1] & 0x80) >> 7);
	printf("Payload unit start indicator : %x\n", (buffer[1] & 0x40) >> 6);	
	/* Payload unit start indicator : if it is 1, it is the first part of PES or PSI packet */
	printf("Transport priority : %x\n", (buffer[1] & 0x20) >> 5);										
	/* Transport priority : if it is 1, it has to be prior */
	printf("PID : %d\n", (pid = ((buffer[1] << 8) & 0x1F00) | (buffer[2] & 0xFF)));
	printf("Transport Scrambling control : %x\n", (buffer[3] & 0xC0) >> 6);								/* If it is 0, not scrambled */
	printf("Adaptation field control : %d\n", adaptation_field_control);
	printf("Continuity counter : %x\n", (buffer[3] & 0x0F));
	if (adaptation_field_control == 0x2 || adaptation_field_control == 0x3)
		readAdaptationField();
	if (adaptation_field_control == 0x1 || adaptation_field_control == 0x3)
		readPayload();
}

