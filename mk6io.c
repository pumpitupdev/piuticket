//
// Created by rfx on 2/12/17.
//

/*
 * A Modified version of CKDURs PIUIO Emulator
 * TODO: Clean the fuck out of this, modularize it, put the hooks in hooks.c, a bunch of shit really.
 * // TODO: Add lights, clean up mapping.
 * // TODO: Make this be a configurable part of the patch (e.g. can be disabled if a real mk6io is present)
 */

#include <linux/joystick.h>
#include <pthread.h>
#include <glob.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include "usb.h"
#include "mk6io.h"


// Button Description
int btn_dscr[] = {6, 7, 2, 4, 5,
                  -1, -1, -1, -1, -1
                                  -1, -1, 0, 1};
uint32_t btn_mrk[] =
        {0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010,
         0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000,
         0x04000000, 0x00000200, 0x00000400, 0x00004000};

uint32_t lst_mrk[] =
        {0x00000400, 0x00000000, 0x00000004, 0x00000000, 0x00000008,
         0x00000010, 0x00000001, 0x00000002, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000};

int size_mrk = sizeof(lst_mrk)/sizeof(uint32_t);

const short PIUIO_VENDOR_ID	= 0x0547;
const short PIUIO_PRODUCT_ID = 0x1002;

const short PIUIOBUTTON_VENDOR_ID	= 0x0D2F;
const short PIUIOBUTTON_PRODUCT_ID = 0x1010;

/* proprietary (arbitrary?) request PIUIO requires to handle I/O */
const short PIUIO_CTL_REQ = 0xAE;

/* timeout value for read/writes, in microseconds (so, 10 ms) */
const int REQ_TIMEOUT = 10000;

struct usb_bus *usb_busses = NULL;
struct usb_bus *bus;

FILE **kbds;
int i = 0, k, l;
char key_map[KEY_MAX/8+1];
char key_map2[KEY_MAX/8+1];

char bCouple = 0;
char bFastFire = 0;
char bStateFastFire = 0;
int bMaxStateFastFire = 450;
char bKeyCouple = 0;
char bKeyFastFire = 0;
char bKeyMaxFastFire1 = 0;
char bKeyMaxFastFire2 = 0;

int joy_fd, *axis=NULL, num_of_axis=0, num_of_buttons=0, x;
char *button=NULL, name_of_joystick[80];
struct js_event js;
int rc=0;
//pthread_t thread;
pthread_mutex_t mutex;
//void *input_thread(void *threadid);
int done;
char bytes_k[4];
FILE* hFile = NULL;

/** Declaracion de funciones para HOOKS */
void (*libusb_usb_init)( void ) = NULL;
int (*libusb_usb_find_busses)( void ) = NULL;
int (*libusb_usb_find_devices)( void ) = NULL;
usb_dev_handle *(*libusb_usb_open)( struct usb_device * ) = NULL;

int g_init = 0;
int nKeyboards = 0;

void usb_init(void){
    // And now, init the SDL
    //printf("CKDUR: usb_init %d\n", __LINE__);
    //if(hFile != NULL) fflush(hFile);

    if( !libusb_usb_init )
        libusb_usb_init = dlsym(RTLD_NEXT, "usb_init");
    libusb_usb_init();


    if(g_init) return;
    g_init = 1;

    glob_t keyboards;
    glob("/dev/input/by-path/*event-kbd", 0, 0, &keyboards);

    if(keyboards.gl_pathc > 0){
        nKeyboards = keyboards.gl_pathc;
        printf("Found %d keyboards\n", nKeyboards);
        kbds = (FILE**)malloc(nKeyboards*sizeof(FILE*));
        char **p;
        int n;
        for(p = keyboards.gl_pathv, n = 0; n < (int)(keyboards.gl_pathc); p++, n++) {
            kbds[n] = fopen(*p, "r");
            printf("Opening file: %s\n", *p);
        }
    }

    globfree(&keyboards);

    if( ( joy_fd = open( JOY_DEV , O_RDONLY)) == -1 ){
        printf("MK6IO: Couldn't open joystick\n" );
    }else{
        ioctl( joy_fd, JSIOCGAXES, &num_of_axis );
        ioctl( joy_fd, JSIOCGBUTTONS, &num_of_buttons );
        ioctl( joy_fd, JSIOCGNAME(80), &name_of_joystick );
        axis = (int *) calloc( num_of_axis, sizeof( int ) );
        button = (char *) calloc( num_of_buttons, sizeof( char ) );
        printf("MK6IO: Joystick detected: %s\n\t%d axis\n\t%d buttons\n\n"
                , name_of_joystick
                , num_of_axis
                , num_of_buttons );
        fcntl( joy_fd, F_SETFL, O_NONBLOCK );	/* use non-blocking mode */
    }

    pthread_mutex_init(&mutex, NULL);

    done = 0;
    rc=0;

    bytes_k[0] = 0xFF;
    bytes_k[1] = 0xFF;
    bytes_k[2] = 0xFF;
    bytes_k[3] = 0xFF;
}

#define keyp(keymap, key) (keymap[key/8] & (1 << (key % 8)))
char bytes_p[4];
uint32_t mem = 0xFFFFFFFF;
struct timeval  tv;
double tick, last_tick;
char bytes_b[4] = {0x0,0x0,0x0,0x0};
inline double get_time_ms(){
    gettimeofday(&tv, NULL);

    return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ; // convert tv_sec & tv_usec to millisecond
}
struct usb_device* pec;
int usb_control_msg(usb_dev_handle *dev, int requesttype, int request,
                    int value, int index, char *bytes, int size, int timeout){


    pec = (struct usb_device*)dev;




    if(pec->descriptor.idVendor == PIUIO_VENDOR_ID && pec->descriptor.idProduct == PIUIO_PRODUCT_ID &&
       (requesttype & USB_DIR_IN) && (requesttype & USB_TYPE_VENDOR) && (request & PIUIO_CTL_REQ)){
        {
            bytes_p[0] = 0xFF;
            bytes_p[1] = 0xFF;
            bytes_p[2] = 0xFF;
            bytes_p[3] = 0xFF;
            for(i = 0; i < nKeyboards; i++) if(kbds[i] != NULL){

                    memset(key_map, 0, sizeof(key_map));    //  Initate the array to zero's
                    ioctl(fileno(kbds[i]), EVIOCGKEY(sizeof(key_map)), key_map);    //  Fill the keymap with the current keyboard state


                    char byte = 0;
                    if(keyp(key_map, KEY_KP7) || keyp(key_map, KEY_R))
                        byte |= (0x1);
                    if(keyp(key_map, KEY_KP9) || keyp(key_map, KEY_Y))
                        byte |= (0x2);
                    if(keyp(key_map, KEY_KP5) || keyp(key_map, KEY_G))
                        byte |= (0x4);
                    if(keyp(key_map, KEY_KP1) || keyp(key_map, KEY_V))
                        byte |= (0x8);
                    if(keyp(key_map, KEY_KP3) || keyp(key_map, KEY_N))
                        byte |= (0x10);
                    if(keyp(key_map, KEY_U))
                        byte |= (0x20);
                    if(keyp(key_map, KEY_I))
                        byte |= (0x40);
                    if(keyp(key_map, KEY_O))
                        byte |= (0x80);

                    bytes_p[2] &= ~byte;



                    byte = 0;
                    if(keyp(key_map, KEY_Q))
                        byte |= (0x1);
                    if(keyp(key_map, KEY_E))
                        byte |= (0x2);
                    if(keyp(key_map, KEY_S))
                        byte |= (0x4);
                    if(keyp(key_map, KEY_Z))
                        byte |= (0x8);
                    if(keyp(key_map, KEY_C))
                        byte |= (0x10);
                    if(keyp(key_map, KEY_P))
                        byte |= (0x20);
                    if(keyp(key_map, KEY_J))
                        byte |= (0x40);
                    if(keyp(key_map, KEY_K))
                        byte |= (0x80);

                    bytes_p[0] &= ~byte;

                    byte = 0;
                    if(keyp(key_map, KEY_L))
                        byte |= (0x1);
                    if(keyp(key_map, KEY_M))
                        byte |= (0x2);
                    if(keyp(key_map, KEY_F6))
                        byte |= (0x4);
                    if(keyp(key_map, KEY_7))
                        byte |= (0x8);
                    if(keyp(key_map, KEY_8))
                        byte |= (0x10);
                    if(keyp(key_map, KEY_9))
                        byte |= (0x20);
                    if(keyp(key_map, KEY_0))
                        byte |= (0x40);
                    if(keyp(key_map, KEY_COMMA))
                        byte |= (0x80);
                    bytes_p[3] &= ~byte;

                    byte = 0;
                    if(keyp(key_map, KEY_F7))
                        byte |= (0x1);
                    if(keyp(key_map, KEY_F1))
                        byte |= (0x2);
                    if(keyp(key_map, KEY_F5))
                        byte |= (0x4);
                    if(keyp(key_map, KEY_F8))
                        byte |= (0x8);
                    if(keyp(key_map, KEY_F9))
                        byte |= (0x10);
                    if(keyp(key_map, KEY_F10))
                        byte |= (0x20);
                    if(keyp(key_map, KEY_F2))
                        byte |= (0x40);
                    if(keyp(key_map, KEY_F3))
                        byte |= (0x80);
                    bytes_p[1] &= ~byte;

                    if(bCouple){
                        bytes_p[2] &= ~byte;
                    }

                    if(keyp(key_map, KEY_SPACE)){
                        bytes_p[2] &= 0xE0;
                        bytes_p[0] &= 0xE0;
                    }

                    if(bFastFire){
                        //bStateFastFire++;
                        //if(bStateFastFire >= bMaxStateFastFire) bStateFastFire = 0;
                        tick = get_time_ms();
                        if((tick-last_tick) >= ((double) bMaxStateFastFire)) last_tick = tick;
                        bytes_p[2] &= (tick-last_tick) < ((double) bMaxStateFastFire/2)?0xE0:0xFF;
                        bytes_p[0] &= (tick-last_tick) < ((double) bMaxStateFastFire/2)?0xE0:0xFF;
                    }
                }
        }
        if(joy_fd != -1){
            while(read(joy_fd, &js, sizeof(struct js_event)) > 0){

                if((js.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON &&
                   js.number < size_mrk){
                    if(js.value)
                        mem &= ~lst_mrk[js.number];
                    else
                        mem |=  lst_mrk[js.number];
                }

            }
        }
        bytes[0] = (mem & 0xFF) & bytes_p[0];
        bytes[1] = ((mem >> 8) & 0xFF) & bytes_p[1];
        bytes[2] = ((mem >> 16) & 0xFF) & bytes_p[2];
        bytes[3] = ((mem >> 24) & 0xFF) & bytes_p[3];
        return 8;   // ITG says that
    }
    else if(pec->descriptor.idVendor == PIUIO_VENDOR_ID && pec->descriptor.idProduct == PIUIO_PRODUCT_ID &&
            (requesttype & USB_DIR_OUT) && (requesttype & USB_TYPE_VENDOR) && (request & PIUIO_CTL_REQ)){
        return 8;   // ITG says that
    }
    else if(pec->descriptor.idVendor == PIUIOBUTTON_VENDOR_ID && pec->descriptor.idProduct == PIUIOBUTTON_PRODUCT_ID){
        char byte = 0;

        for(i = 0; i < nKeyboards; i++) if(kbds[i] != NULL ){

                memset(key_map2, 0, sizeof(key_map2));    //  Initate the array to zero's
                ioctl(fileno(kbds[i]), EVIOCGKEY(sizeof(key_map2)), key_map2);

                if(keyp(key_map2, KEY_BACKSPACE))
                    byte |= (0x1);
                if(keyp(key_map2, KEY_LEFT))
                    byte |= (0x2);
                if(keyp(key_map2, KEY_RIGHT))
                    byte |= (0x4);
                if(keyp(key_map2, KEY_ENTER))
                    byte |= (0x8);
                if(keyp(key_map2, KEY_KPMINUS))
                    byte |= (0x10);
                if(keyp(key_map2, KEY_KP4))
                    byte |= (0x20);
                if(keyp(key_map2, KEY_KP6))
                    byte |= (0x40);
                if(keyp(key_map2, KEY_KPENTER))
                    byte |= (0x80);
            }
        byte = ~byte;

        bytes[0] = byte;
        bytes[1] = 0xFF;
        return 0;
    }

    return 8;
}

int usb_claim_interface(usb_dev_handle *dev, int interface){
    return 0;
}

typedef unsigned int uint;

int g_found_busses = 0;
int usb_find_busses(void){

    // Create the usb bus object

    if(!g_found_busses) bus = (struct usb_bus*)malloc(sizeof(struct usb_bus));
    memset(bus, 0, sizeof(struct usb_bus));
    bus->next=NULL;
    bus->prev=NULL;

    usb_busses = bus;
    g_found_busses = 1;

    return 1;
}

int g_usb_device1=0;
int g_usb_device2=0;
struct usb_device* g_dev = NULL;
struct usb_device* g_dev2 = NULL; // For button
int usb_find_devices(void){

    if(!g_usb_device1){
        g_dev = (struct usb_device*)malloc(sizeof(struct usb_device));
        memset(g_dev, 0, sizeof(struct usb_device));
        g_dev->bus = bus;    // Same bus... LOL
        // A new config for PIUIO
        g_dev->config = (struct usb_config_descriptor*)malloc(sizeof(struct usb_config_descriptor));
        memset(g_dev->config, 0, sizeof(struct usb_config_descriptor));
        g_dev->config->interface = (struct usb_interface*)malloc(sizeof(struct usb_interface));
        memset(g_dev->config->interface, 0, sizeof(struct usb_interface));
        g_dev->config->interface->altsetting = (struct usb_interface_descriptor*)malloc(sizeof(struct usb_interface_descriptor));
        memset(g_dev->config->interface->altsetting, 0, sizeof(struct usb_interface_descriptor));
        g_dev->config->interface->altsetting->endpoint = (struct usb_endpoint_descriptor*)malloc(sizeof(struct usb_endpoint_descriptor));
        memset(g_dev->config->interface->altsetting->endpoint, 0, sizeof(struct usb_endpoint_descriptor));
        // Set the bitches for PIUIO
        g_dev->descriptor.idVendor = PIUIO_VENDOR_ID;
        g_dev->descriptor.idProduct = PIUIO_PRODUCT_ID;
        g_usb_device1=1;
    }
    if(!g_usb_device2){
        g_dev2 = (struct usb_device*)malloc(sizeof(struct usb_device));
        memset(g_dev2, 0, sizeof(struct usb_device));
        g_dev2->bus = bus;    // Same bus... LOL
        // A new config for PIUIOBUTTON
        g_dev2->config = (struct usb_config_descriptor*)malloc(sizeof(struct usb_config_descriptor));
        memset(g_dev2->config, 0, sizeof(struct usb_config_descriptor));
        g_dev2->config->interface = (struct usb_interface*)malloc(sizeof(struct usb_interface));
        memset(g_dev2->config->interface, 0, sizeof(struct usb_interface));
        g_dev2->config->interface->altsetting = (struct usb_interface_descriptor*)malloc(sizeof(struct usb_interface_descriptor));
        memset(g_dev2->config->interface->altsetting, 0, sizeof(struct usb_interface_descriptor));
        g_dev2->config->interface->altsetting->endpoint = (struct usb_endpoint_descriptor*)malloc(sizeof(struct usb_endpoint_descriptor));
        memset(g_dev2->config->interface->altsetting->endpoint, 0, sizeof(struct usb_endpoint_descriptor));
        // Set the bitches for PIUIOBUTTON
        g_dev2->descriptor.idVendor = PIUIOBUTTON_VENDOR_ID;
        g_dev2->descriptor.idProduct = PIUIOBUTTON_PRODUCT_ID;
        g_usb_device2=1;
    }

    //printf("%x %x\n", (uint)g_dev, (uint)g_dev2);

    //Next connected to PIUIO is PIUIOBUTTON
    //This is ONLY if PIUIOBUTTON exists
    //if(bIsButton)
    {
        g_dev->next = g_dev2;
        g_dev2->prev = g_dev;
    }


    // Finally, ret
    bus->devices = g_dev;
    bus->root_dev = g_dev;
    return 1;
}

usb_dev_handle *usb_open(struct usb_device *dev){

    void* p = (void*)dev;
    usb_dev_handle* pd = (usb_dev_handle*)p;
    return pd;
}

int usb_reset(usb_dev_handle *dev) {
    return 0;
}

int usb_set_altinterface(usb_dev_handle *dev, int alternate){
    return 0;
}

int usb_set_configuration(usb_dev_handle *dev, int configuration){
    return 0;
}

struct usb_bus *usb_get_busses(void){
    return usb_busses;
}