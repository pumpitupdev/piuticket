

#include <string.h>
#include <stdio.h>
#include "hdd.h"
#include "util.h"

// Global ATA data block for HDD Identification.
unsigned char ATA_DATA[512];

// Handles ioctl HDD Identify Device with fake data.
int fake_ata_data(void* data){
    memcpy(data,ATA_DATA,512);
    return 0;
}

// Gets ATA data from HDD file.
void get_ata_data(){

    char hdd_path[4096] = {0x00};
    sprintf(hdd_path,"%s/%s",root_path,hdd_filename);

    FILE *fp = fopen(hdd_path,"rb");

    memset(ATA_DATA,0x00,512);
    fseek(fp,0x300,SEEK_SET);
    // Read in Model
    fread(ATA_DATA+0x14,20,1,fp);
    printf("HDD Model: %s\n",ATA_DATA+0x14);
    // Read in Firmware
    fread(ATA_DATA+0x2E,8,1,fp);
    printf("HDD Firmware: %s\n",ATA_DATA+0x2E);
    // Read in Serial
    fread(ATA_DATA+0x36,36,1,fp);
    printf("HDD Serial: %s\n",ATA_DATA+0x36);
    fclose(fp);
}
