#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dog.h"


unsigned char b_share = 0;

FILE * dogfile;
Dog curr_dog;

//Loads dongle metadata from our dongle file.
void load_dogfile(){
	fseek(dogfile,0,SEEK_SET);
	fread(&curr_dog,224,1,dogfile);
	free(dog_table);
	dog_table = (unsigned char*) malloc(sizeof(RTEntry) * curr_dog.num_keys);
	fread(dog_table,(sizeof(RTEntry) * curr_dog.num_keys),1,dogfile);
	
}

void init_microdoge(const char* doge_file_path){
	//Set our endpoint.
    dogfile = fopen("/dev/usbdog","wb");
    fclose(dogfile);
    
	//Setting up the virtual dongle
    dogfile = fopen(doge_file_path,"rb+");
    if(!dogfile){
     printf("Dog Key File Not Found!\n");
     exit(1);
    }
	load_dogfile();
}  
//Set our manufacturer serial number.
void set_mfg_serial(unsigned char* dog_data){
  memcpy(&curr_dog.mfg_serial,dog_data,4);
  fseek(dogfile,0,SEEK_SET);
  fwrite(&curr_dog,sizeof(Dog),1,dogfile);
}
//Set our dog serial.
void set_dog_serial(unsigned char* dog_data){

  curr_dog.dog_serial = *(unsigned long *)dog_data;
  fseek(dogfile,0,SEEK_SET);
  fwrite(&curr_dog,sizeof(Dog),1,dogfile);
}
//Set our VendorID.
void set_vendor_id(unsigned char* dog_data){
  memcpy(&curr_dog.vendor_id,dog_data,8);
  fseek(dogfile,0,SEEK_SET);
  fwrite(&curr_dog,sizeof(Dog),1,dogfile);
}

//
unsigned long convert(unsigned short dog_bytes,unsigned char * dog_data){
	unsigned long dog_response = 0;
	RTEntry *rt;
	int i;
	unsigned long curr_algo;
	memcpy(&curr_algo,curr_dog.dog_flashmem+196,4);
	for(i = 0 ; i < curr_dog.num_keys;i++){
		rt = dog_table + (i * sizeof(RTEntry));
		if(rt->algorithm == curr_algo){
			if(rt->req_len == dog_bytes){
				if(memcmp(rt->request,dog_data,dog_bytes) == 0){
					dog_response = rt->response;
					int j;
					//for(j=0;j<dog_bytes;j++){
					//printf("%02x",dog_data[j]);
					//}
					//printf(" -> 0x%04x\n",dog_response);
				}
			}
		}
	}
	return dog_response;
}

//Add a new key to our rainbow table. 
//NOTE: This will not auto-reload the table for use, you have to call 
//the ReloadRainbowTable() function from your program.
void add_rtkey(unsigned char * dog_data,unsigned short dogbytes,unsigned long dog_response){
  //Increment our key count.
  curr_dog.num_keys++;
  //Write the new header.
  fseek(dogfile,0,SEEK_SET);
  fwrite(&curr_dog,sizeof(Dog),1,dogfile);  
  fseek(dogfile,0,SEEK_END);
  //Preventing alignment mistakes later on.
  unsigned long dbyl = dogbytes;
  unsigned char dog_request[64] = {0x00};
  memcpy(&dog_request,dog_data,dogbytes);
  //Writing response, algorithm(last 4 bytes of flash memory), dogbytes, and request to end of file.
  fwrite(&dog_response,4,1,dogfile);
  fwrite(curr_dog.dog_flashmem+196,4,1,dogfile);
  fwrite(&dbyl,4,1,dogfile);
  fwrite(dog_request,64,1,dogfile);
}

//Encrypt-Decrypt Request packets.
void crypt_request(Request *request){
    
	unsigned int tmp_mask = (request->mask_key + GOLD_SALT) & 0xFFFFFFFF;
    unsigned char tmb_mask[4] = {0x00};
    memcpy(tmb_mask,&tmp_mask,4);
   
    request->dog_addr ^= (tmp_mask & 0xFFFF);
    request->dog_bytes ^= (tmp_mask & 0xFFFF);
    request->dog_password ^= tmp_mask;
    request->b_hostid ^= (tmp_mask & 0xFF);
    int i;
    for(i = 0 ; i < 256; i++){
     request->dog_data[i] ^=  tmb_mask[i % 4]; 
    }
   
}

//Encrypt-Decrypt Response packets.
void crypt_response(Request *request, Response *response){
    
	unsigned int tmp_mask = (request->mask_key + GOLD_SALT) & 0xFFFFFFFF;
    unsigned char tmb_mask[4] = {0x00};
    memcpy(tmb_mask,&tmp_mask,4);  
    int i;
    for(i = 0 ; i < 200; i++){
     response->dog_data[i] ^=  tmb_mask[i % 4]; 
    }
}



//Print our request packet for debugging purposes.
void print_req(Request *request){
    printf("Request Packet\n");
    printf("Magic = %04X\n",      request->magic);
    printf("Opcode = %04X\n",     request->opcode);
    printf("VendorID = %04X\n",   request->dog_serial);
    printf("Mask_key = %04X\n",   request->mask_key);
    printf("DogAddr = %04X\n",    request->dog_addr);
    printf("DogBytes = %04X\n",   request->dog_bytes);
    printf("DogPasswd = %04X\n",  request->dog_password);
    printf("bShare = %02X\n", request->b_hostid);
}

//Print our request packet for debugging purposes.
void print_resp(Response *response){
    printf("\nResponse Packet\n");
    printf("Serial = %04X\n",response->dog_serial);
    printf("RetCode = %d\n",response->return_code);
    printf("Data:\n");
    int j;
    for(j=0;j<16;j++){
     printf("%02X ",response->dog_data[j]); 
    }
    printf("\n END Data:\n");
}

//Our entry-point.
int process_microdog_xact(int fd, int req, unsigned long* data){
	Request request;
	Response response;
	
	memcpy(&request,*(unsigned long*)data,277);
	
	response.return_code = 0;
	memset(response.dog_data,0x00,256);
	response.dog_serial = curr_dog.dog_serial;

	if(request.magic != MD_MAGIC){
	 printf("ERR-BAD MAGIC\n"); 
	}

	crypt_request(&request);
	if(DEBUG){
		print_req(&request);
	}
	//Determine transaction - Much Functionality WOW
	switch(request.opcode){
		case MD_DOGCHECK:
			printf("MD_DOGCHECK\n");
			break;
		case MD_READDOG:
			printf("MD_READDOG\n");
			if(request.dog_password == curr_dog.dog_password){
				memcpy(&response.dog_data, curr_dog.dog_flashmem+request.dog_addr,request.dog_bytes);
			}else{
				response.return_code = MD_ERR_PW;
			}
			break;
		case MD_WRITEDOG:
			printf("MD_WRITEDOG\n");
			if(request.dog_password == curr_dog.dog_password){
				memcpy(curr_dog.dog_flashmem+request.dog_addr,&request.dog_data,request.dog_bytes);
				fseek(dogfile,0,SEEK_SET);
				fwrite(&curr_dog,224,1,dogfile);
			}else{
				response.return_code = MD_ERR_PW;
			}	
			break;
		case MD_CONVERT:
			printf("");
			unsigned long dog_response = convert(request.dog_bytes,request.dog_data);
			memcpy(&response.dog_data,&dog_response,4);
			break;
		case MD_SETPASSWORD:
			printf("MD_SETPASSWORD\n");
			if(request.dog_password == curr_dog.dog_password){
				curr_dog.dog_password = *(unsigned long *)request.dog_data;
				fseek(dogfile,0,SEEK_SET);
				fwrite(&curr_dog,224,1,dogfile);
				
			}else{
				response.return_code = MD_ERR_PW;
			}	
			break;
		case MD_SETSHARE:
			printf("MD_SETSHARE\n");
			b_share = request.dog_data[0];
			response.dog_data[0] = b_share;
			break;
		case MD_GETLOCKNO:
			printf("MD_GETLOCKNO\n");
			memcpy(&response.dog_data,curr_dog.mfg_serial,4);
			break;
		case MD_LOGIN:
                        printf("");
			int j;
			for(j=0;j<8;j++){
			 response.dog_data[j] = curr_dog.vendor_id[j];
			}
			break;
		case MD_SETDOGCASCADE:
			printf("MD_SETDOGCASCADE\n");
			response.dog_data[0] = request.dog_data[0];
			break;
		case MD_RTADD:
			printf("MD_RTADD\n");
			add_rtkey(request.dog_data,request.dog_bytes,request.dog_password);
			break;
		case MD_SETDOGSERIAL:
			printf("MD_SETDOGSERIAL\n");
			set_dog_serial(request.dog_data);
			break;
		case MD_SETMFGSERIAL:
			printf("MD_SETMFGSERIAL\n");
			set_mfg_serial(request.dog_data);
			break;
		case MD_SETVID:
			printf("MD_SETVID\n");
			set_vendor_id(request.dog_data);
			break;
		case MD_RLRT:
			printf("MD_RLRT\n");
			load_dogfile();
			break;
		default:
			printf("Unsupported Opcode: %04X\n",request.opcode);
			break;	
	}
	if(DEBUG){
	print_resp(&response);
	}
	crypt_response(&request,&response);

	//Copy the response packet to the program.
	memcpy(*(unsigned int *)data-272,&response,208);
	return 0;
}
