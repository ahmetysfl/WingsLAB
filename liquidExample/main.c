#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <complex.h>
#include <liquid/liquid.h>

int cnv2digital(float f)
{
    int i = f * 2048;

    if (i>2047)
    {
        i=2047;
    }
    else if(i<-2048)
    {
        i=-2048;
    }
}
int main() {
    // allocate memory for arrays
    unsigned char header[8];                    // data header
    unsigned char payload[64];                  // data payload
    unsigned int  buf_len = LIQUID_FRAME64_LEN; // length of frame
    float complex buf[buf_len];                 // sample buffer

    // CREATE frame generator
    framegen64 fg = framegen64_create();
    framegen64_print(fg);

    // initialize header, payload
    unsigned int i;
    for (i=0; i<8; i++)
        header[i] = i;
    for (i=0; i<64; i++)
        payload[i] = rand() & 0xff;

    // EXECUTE generator and assemble the frame
    framegen64_execute(fg, header, payload, buf);

    FILE * pFile;
    char stringLine[16];
    pFile = fopen ("tx.csv", "wb");
    // print a few of the generated frame samples to the screen
    for (i=0; i<buf_len; i++)
    {
        float real_part = crealf(buf[i]);
        float img_part = cimagf(buf[i]);

        fprintf (pFile , "%d,%d\n",cnv2digital(real_part), cnv2digital(img_part));
        printf("%3u : %12.8f + j*%12.8f\n", i, real_part, img_part);
        printf("___ : %d + j*%d\n\n", cnv2digital(real_part), cnv2digital(img_part));
    }

    fclose(pFile);
    // DESTROY objects
    framegen64_destroy(fg);

    printf("done.\n");
    return 0;
}
