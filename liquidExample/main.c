#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <liquid/liquid.h>

// user-defined static callback function
static int mycallback(unsigned char *  _header,
                      int              _header_valid,
                      unsigned char *  _payload,
                      unsigned int     _payload_len,
                      int              _payload_valid,
                      framesyncstats_s _stats,
                      void *           _userdata)
{
    printf("***** callback invoked!\n");
    printf("    header  (%s)\n",  _header_valid ? "valid" : "INVALID");
    printf("    payload (%s)\n", _payload_valid ? "valid" : "INVALID");
    framesyncstats_print(&_stats);

    for(int i=0;i<_payload_len;i++)
    {

    }

    // type-cast, de-reference, and increment frame counter
    unsigned int * counter = (unsigned int *) _userdata;
    (*counter)++;

    return 0;
}

int main() {
    FILE *fp;
    fp = fopen( "file.txt" , "w" );

    // options
    unsigned int frame_counter   =   0;     // userdata passed to callback
    float        phase_offset    =   0.3f;  // carrier phase offset
    float        frequency_offset=   0.02f; // carrier frequency offset
    float        SNRdB           =  10.0f;  // signal-to-noise ratio [dB]
    float        noise_floor     = -40.0f;  // noise floor [dB]

    // allocate memory for arrays
    unsigned char header[8];                    // data header
    unsigned char payload[64];                  // data payload
    unsigned int  buf_len = LIQUID_FRAME64_LEN; // length of frame
    float complex buf[buf_len];                 // sample buffer

    // create frame generator
    framegen64 fg = framegen64_create();
    framegen64_print(fg);

    // create frame synchronizer using default properties
    framesync64 fs = framesync64_create(mycallback, (void*)&frame_counter);
    framesync64_print(fs);

    // initialize header, payload
    unsigned int i;
    for (i=0; i<8; i++)
        header[i] = i;
    for (i=0; i<64; i++)
        payload[i] = rand() & 0xff;

    // EXECUTE generator and assemble the frame, storing samples in buffer
    framegen64_execute(fg, header, payload, buf);

    // add channel impairments (attenuation, carrier offset, noise)
    float nstd  = powf(10.0f, noise_floor/20.0f);        // noise std. dev.
    float gamma = powf(10.0f, (SNRdB+noise_floor)/20.0f);// channel gain
    for (i=0; i<buf_len; i++) {
        buf[i] *= gamma;
        buf[i] *= cexpf(_Complex_I*(phase_offset + i*frequency_offset));
        buf[i] += nstd * (randnf() + _Complex_I*randnf())*M_SQRT1_2;
    }

    // EXECUTE synchronizer and receive the frame in one block
    framesync64_execute(fs, buf, buf_len);

    // DESTROY objects
    framegen64_destroy(fg);
    framesync64_destroy(fs);

    printf("received %u frames\n", frame_counter);
    printf("done.\n");
    return 0;
}
