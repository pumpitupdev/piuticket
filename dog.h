#pragma once
#include <stdio.h>
//Defines
#define DEBUG 0
#define MD_DOGCHECK      0x01
#define MD_READDOG       0x02
#define MD_WRITEDOG      0x03
#define MD_CONVERT       0x04
#define MD_SETPASSWORD   0x07
#define MD_SETSHARE      0x08
#define MD_GETLOCKNO     0x0B
#define MD_LOGIN         0x14
#define MD_SETDOGCASCADE 0x15
#define MD_MAGIC         0x484D
#define MD_RTADD         0x539
#define MD_SETDOGSERIAL  0x53A
#define MD_SETMFGSERIAL  0x53B
#define MD_SETVID        0x53C
#define MD_RLRT          0x53D
#define MD_ERR_PW        0x2745
#define MD_XACT          0x6B00
#define GOLD_SALT        0x646C6F47

//Custom Datatypes
typedef struct Request{
	unsigned short magic;
	unsigned short opcode;
	unsigned int   dog_serial;
	unsigned int   mask_key;
	unsigned short dog_addr;
	unsigned short dog_bytes;
	unsigned char  dog_data[256];
	unsigned int   dog_password;
	unsigned char  b_hostid;
}Request;

typedef struct Response{
	unsigned int dog_serial;
	unsigned int return_code;
	unsigned char dog_data[256];
}Response;

typedef struct DogHeader{
	unsigned int  dog_serial;
	unsigned int  dog_password;
	unsigned char vendor_id[8];
	unsigned int  mfg_serial;
	unsigned char dog_flashmem[200];
	unsigned int  num_keys;
}Dog;

typedef struct DogKey{
	unsigned long response;
	unsigned long algorithm;
	unsigned long req_len;
	unsigned char request[64];
}RTEntry;

//Because we need to pass along other ioctls.
static void *io;
static void * dog_table;


// Loads the dogfile, sets up the virtual dongle.
void init_microdoge(const char* doge_file_path);
int process_microdog_xact(int fd, int request, unsigned long* data);

