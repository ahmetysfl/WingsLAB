//
// flexframesync_example.c
//
// This example demonstrates the basic interface to the flexframegen and
// flexframesync objects used to completely encapsulate raw data bytes
// into frame samples (nearly) ready for over-the-air transmission. A
// 14-byte header and variable length payload are encoded into baseband
// symbols using the flexframegen object.  The resulting symbols are
// interpolated using a root-Nyquist filter and the resulting samples are
// then fed into the flexframesync object which attempts to decode the
// frame. Whenever frame is found and properly decoded, its callback
// function is invoked.
//
// SEE ALSO: flexframesync_reconfig_example.c
//           framesync64_example.c
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <getopt.h>
#include <assert.h>

#include "liquid/liquid.h"


// global arrays
unsigned char * header;
unsigned char * payload;
FILE* logFile;
const int  header_len = 8;
const int  payload_len = 480;
int noBitError = 0;
int validHeader = 0;
int frameDetected = 0;
// options

modulation_scheme ms     =  LIQUID_MODEM_QPSK; // mod. scheme
crc_scheme check         =  LIQUID_CRC_NONE;     // data validity check
fec_scheme fec0          =  LIQUID_FEC_NONE;   // fec (inner)
fec_scheme fec1          =  LIQUID_FEC_NONE;   // fec (outer)
float SNRdB       =  20.0f; // signal-to-noise ratio            // signal-to-noise ratio

// flexframesync callback function
static int callback(unsigned char *  _header,
                    int              _header_valid,
                    unsigned char *  _payload,
                    unsigned int     _payload_len,
                    int              _payload_valid,
                    framesyncstats_s _stats,
                    void *           _userdata);

int simulateFrame (modulation_scheme ms,crc_scheme check,fec_scheme fec0,fec_scheme fec1,float SNRdB)
{

        // payload length
    int debug_enabled        =  0;                 // enable debugging?
    float noise_floor        = -40.0f;             // noise floor
    float dphi               =  0.01f;             // carrier frequency offset


    // derived values
    float nstd  = powf(10.0f, noise_floor/20.0f);         // noise std. dev.
    float gamma = powf(10.0f, (SNRdB+noise_floor)/20.0f); // channel gain

    // create flexframegen object
    flexframegenprops_s fgprops;
    flexframegenprops_init_default(&fgprops);
    fgprops.mod_scheme  = ms;
    fgprops.check       = check;
    fgprops.fec0        = fec0;
    fgprops.fec1        = fec1;
    flexframegen fg = flexframegen_create(&fgprops);

    // create flexframesync object
    flexframesync fs = flexframesync_create(callback,&fec1);
    if (debug_enabled)
        flexframesync_debug_enable(fs);

    // assemble the frame (NULL pointers for default values)
    flexframegen_assemble(fg, header, payload, payload_len);
    //flexframegen_print2(logFile,fg);

    // generate the frame in blocks
    unsigned int  buf_len = 25000;
    float complex x[buf_len];
    float complex y[buf_len];

    int frame_complete = 0;
    float phi = 0.0f;

        // write samples to buffer
        frame_complete = flexframegen_write_samples(fg, x, buf_len);

        if(!frame_complete)
        {
            printf("Error buf is not suffiecnt");
            return 0;
        }
        // add noise and push through synchronizer
        for (int i=0; i<buf_len; i++) {
            // apply channel gain and carrier offset to input
            y[i] = gamma * x[i] * cexpf(_Complex_I*phi);
            phi += dphi;

            // add noise
            y[i] += nstd*( randnf() + _Complex_I*randnf())*M_SQRT1_2;
        }


        // run through frame synchronizer
        flexframesync_execute(fs, y, buf_len);


    // export debugging file
    if (debug_enabled)
        flexframesync_debug_print(fs, "flexframesync_debug.m");

    //flexframesync_print(fs);
    // destroy allocated objects
    flexframegen_destroy(fg);
    flexframesync_destroy(fs);

}


int main(int argc, char *argv[])
{


    //srand( time(NULL) );
    payload = malloc(payload_len* sizeof(unsigned char));
    if (payload == NULL) {
        perror("malloc");
        return 0;
    }
    header = malloc(header_len* sizeof(unsigned char));
    if (header == NULL) {
        perror("malloc");
        return 0;
    }

    // initialize header, payload
    unsigned int i;
    for (i=0; i<header_len; i++)
        header[i] = i;
    for (i=0; i<payload_len; i++)
        payload[i] = rand() & 0xff;

    fec1          =  8;
    char s[256];
    sprintf(s,"log%d.txt",fec1);
    logFile = fopen (s, "a+");


    // create flexframegen object
    flexframegenprops_s fgprops;
    flexframegenprops_init_default(&fgprops);
    fgprops.mod_scheme  = ms;
    fgprops.check       = check;
    fgprops.fec0        = fec0;
    fgprops.fec1        = fec1;
    flexframegen fg = flexframegen_create(&fgprops);
    // assemble the frame (NULL pointers for default values)
    flexframegen_assemble(fg, header, payload, payload_len);
    flexframegen_print2(logFile,fg);
    flexframegen_destroy(fg);
    int noTrials = 250;
    fprintf(logFile,"SNRdB,bitErrorRate,Valid Header,Detected Frame\n");
    fclose(logFile);
    for (i=0; i<noTrials; i++)
    {
        simulateFrame (ms,check,fec0,fec1,SNRdB);
        char s[256];
        sprintf(s,"log%d.txt",fec1);
        logFile = fopen (s, "a+");
        fprintf(logFile,"%f,%d,%d,%d \n",SNRdB,noBitError,validHeader,frameDetected);
        fclose(logFile);
        SNRdB = SNRdB - 0.25;
        noBitError=0;
        validHeader=0;
        printf("%d \n",i);

    }



    printf("done.\n");
    return 0;
}

static int callback(unsigned char *  _header,
                    int              _header_valid,
                    unsigned char *  _payload,
                    unsigned int     _payload_len,
                    int              _payload_valid,
                    framesyncstats_s _stats,
                    void *           _userdata)
{
    //printf("******** callback invoked\n");


    // count bit errors (assuming all-zero message)
    unsigned int bit_errors = 0;
    unsigned int i;
    //framesyncstats_print(&_stats);
    //printf("    header crc          :   %s\n", _header_valid ?  "pass" : "FAIL");
    //printf("    payload length      :   %u\n", _payload_len);
    //printf("    payload crc         :   %s\n", _payload_valid ?  "pass" : "FAIL");
    noBitError = count_bit_errors_array(_payload, payload, _payload_len);
    validHeader = _payload_valid;
    frameDetected = 1;
        //printf("    num payload bit errors  : %u / %u\n",noBitError,_payload_len*8);

    return 0;
}

