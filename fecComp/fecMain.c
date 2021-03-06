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
const int  header_len = 1;
//int  header_len = 8;
const int  payload_len = 2;
//const int  payload_len = 480;
int noBitError = -1;
int validHeader = 0;
int frameDetected = 0;
// options

modulation_scheme ms     =  LIQUID_MODEM_QPSK; // mod. scheme
crc_scheme check         =  LIQUID_CRC_NONE;     // data validity check
fec_scheme fec0          =  LIQUID_FEC_NONE;   // fec (inner)
fec_scheme fec1          =  LIQUID_FEC_NONE;   // fec (outer)
float SNRdB       =  8.0f; // signal-to-noise ratio            // signal-to-noise ratio

// flexframesync callback function
static int callback(unsigned char *  _header,
                    int              _header_valid,
                    unsigned char *  _payload,
                    unsigned int     _payload_len,
                    int              _payload_valid,
                    framesyncstats_s _stats,
                    void *           _userdata);

int random_number(int min_num, int max_num)
{
    int result = 0, low_num = 0, hi_num = 0;

    if (min_num < max_num)
    {
        low_num = min_num;
        hi_num = max_num + 1; // include max_num in output
    } else {
        low_num = max_num + 1; // include max_num in output
        hi_num = min_num;
    }
    result = (rand() % (hi_num - low_num)) + low_num;
    return result;
}

int simulateFrame (modulation_scheme ms,crc_scheme check,fec_scheme fec0,fec_scheme fec1,float SNRdB,float burst_len_percent)
{
    int i = 0;

    // initialize header, payload
    for (i=0; i<header_len; i++)
        header[i] = i;
    for (i=0; i<payload_len; i++)
        payload[i] = rand() & 0xff;

        // payload length
    int debug_enabled        =  0;                 // enable debugging?

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

        // Noise Parameters
        float noise_floor        = -10.0f;             // noise floor
        float dphi               =  0.01f;             // carrier frequency offset
        // derived values
        float nstd  = powf(10.0f, noise_floor/20.0f);         // noise std. dev.
        float gamma = powf(10.0f, (SNRdB+noise_floor)/20.0f); // channel gain
        float abs_sample = 1;
        int sample_len = 0;
        // add noise and push through synchronizer
        for (i=0; i<buf_len; i++) {
            // apply channel gain and carrier offset to input
            abs_sample=cabs(x[i]);
            if (abs_sample==0)
            {
                if (sample_len==0)
                {
                    sample_len=i;
                }

            }

            y[i] = gamma * x[i] * cexpf(_Complex_I*phi);
            phi += dphi;
            // add noise
            y[i] += nstd*( randnf() + _Complex_I*randnf())*M_SQRT1_2;
        }

        if (burst_len_percent>0)
        {
            printf("Sample Len : %d \n",sample_len);
            // Add Burst Error
            int real_burst_length = ceil(burst_len_percent*sample_len);
            int burst_start_pos = random_number(600,sample_len-real_burst_length);

            // Noise Parameters
            noise_floor         = -10.0f;             // noise floor
            float SNRdB_burst             = -0.5;
            // derived values
            float nstd_burst  = powf(10.0f, noise_floor/20.0f);         // noise std. dev.
            float gamma_burst = powf(10.0f, (SNRdB_burst+noise_floor)/20.0f); // channel gain

            printf("Start Burst ERROR : %d,Burst ERROR Len : %d \n", burst_start_pos,real_burst_length);
            for (i=burst_start_pos; i<burst_start_pos+real_burst_length; i++) {
                // apply channel gain and carrier offset to input
                y[i] = gamma_burst * y[i];
                // add noise
                y[i] += nstd_burst*( randnf() + _Complex_I*randnf())*M_SQRT1_2;
            }
        }



        FILE * xFile;
        char s[256];
        sprintf(s,"log_frame_SNR.txt");
        xFile = fopen (s, "a+");
        fprintf(xFile,"Burst Len Percent = %f, SNRdB = %f\n",burst_len_percent,SNRdB);
        for (i=0; i<sample_len; i++)
        {
            fprintf(xFile,"%f;%f\n",crealf(y[i]),cimagf(y[i]));
        }
        fclose(xFile);
        exit(1);

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
    srand(time(NULL));
    int i=0;
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

    fec1          =  1;
    char s[256];
    sprintf(s,"log%d.txt",fec1);
    logFile = fopen (s, "a+");

    float burst_len = 0.2;

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
    int noTrials = 1;
    //int noTrials = 81;
    fprintf(logFile,"BursERRORPercent;SNRdB;bitErrorRate_0;bitErrorRate_1;bitErrorRate_2;bitErrorRate_3;bitErrorRate_4;bitErrorRate_5;bitErrorRate_6;bitErrorRate_7;bitErrorRate_8;bitErrorRate_9;Valid Header,Detected Frame\n");
    fclose(logFile);
    int bitErrorVector[10];

        sprintf(s,"log%d.txt",fec1);
        logFile = fopen (s, "a+");
    for (i=0; i<noTrials; i++)
    {
        simulateFrame (ms,check,fec0,fec1,SNRdB,burst_len);
        bitErrorVector[0]=noBitError;
        noBitError=-1;
        simulateFrame (ms,check,fec0,fec1,SNRdB,burst_len);
        bitErrorVector[1]=noBitError;
        noBitError=-1;
        simulateFrame (ms,check,fec0,fec1,SNRdB,burst_len);
        bitErrorVector[2]=noBitError;
        noBitError=-1;
        simulateFrame (ms,check,fec0,fec1,SNRdB,burst_len);
        bitErrorVector[3]=noBitError;
        noBitError=-1;
        simulateFrame (ms,check,fec0,fec1,SNRdB,burst_len);
        bitErrorVector[4]=noBitError;
        noBitError=-1;
        simulateFrame (ms,check,fec0,fec1,SNRdB,burst_len);
        bitErrorVector[5]=noBitError;
        noBitError=-1;
        simulateFrame (ms,check,fec0,fec1,SNRdB,burst_len);
        bitErrorVector[6]=noBitError;
        noBitError=-1;
        simulateFrame (ms,check,fec0,fec1,SNRdB,burst_len);
        bitErrorVector[7]=noBitError;
        noBitError=-1;
        simulateFrame (ms,check,fec0,fec1,SNRdB,burst_len);
        bitErrorVector[8]=noBitError;
        noBitError=-1;
        simulateFrame (ms,check,fec0,fec1,SNRdB,burst_len);
        bitErrorVector[9]=noBitError;
        noBitError=-1;


        fprintf(logFile,"%f;%f;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d \n",
                burst_len,SNRdB,bitErrorVector[0],bitErrorVector[1],bitErrorVector[2],bitErrorVector[3],bitErrorVector[4],bitErrorVector[5],bitErrorVector[6],bitErrorVector[7],bitErrorVector[8],bitErrorVector[9],validHeader,frameDetected);

        SNRdB = SNRdB - 0.1;
        validHeader=0;
        printf("%d \n",i);

    }
    fclose(logFile);


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
    if (_payload_valid) {
        noBitError = count_bit_errors_array(_payload, payload, _payload_len);
        validHeader = _payload_valid;
    }
    frameDetected = 1;
        //printf("    num payload bit errors  : %u / %u\n",noBitError,_payload_len*8);

    return 0;
}

