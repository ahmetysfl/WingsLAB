#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
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
    //printf("***** callback invoked!\n");
    //printf("    header  (%s)\n",  _header_valid ? "valid" : "INVALID");
    //printf("    payload (%s)\n", _payload_valid ? "valid" : "INVALID");
    framesyncstats_print(&_stats);

    // type-cast, de-reference, and increment frame counter
    unsigned int * counter = (unsigned int *) _userdata;
    (*counter)++;

    return 0;
}


char* ReadFile(char *filename)
{
   char *buffer = NULL;
   int string_size, read_size;
   FILE *handler = fopen(filename, "r");

   if (handler)
   {
       // Seek the last byte of the file
       fseek(handler, 0, SEEK_END);
       // Offset from the first to the last byte, or in other words, filesize
       string_size = ftell(handler);
       // go back to the start of the file
       rewind(handler);

       // Allocate a string that can hold it all
       buffer = (char*) malloc(sizeof(char) * (string_size + 1) );

       // Read it all in one operation
       read_size = fread(buffer, sizeof(char), string_size, handler);

       // fread doesn't set it so put a \0 in the last position
       // and buffer is now officially a string
       buffer[string_size] = '\0';

       if (string_size != read_size)
       {
           // Something went wrong, throw away the memory and set
           // the buffer to NULL
           free(buffer);
           buffer = NULL;
       }

       // Always remember to close the file.
       fclose(handler);
    }

    return buffer;
}

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
    return i;
}
int main() {
    // options
    unsigned int frame_counter   =   0;     // userdata passed to callback
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
        //printf("%3u : %12.8f + j*%12.8f\n", i, real_part, img_part);
        //printf("___ : %d + j*%d\n\n", cnv2digital(real_part), cnv2digital(img_part));
    }

    fclose(pFile);
    // DESTROY objects
    framegen64_destroy(fg);

    // create frame synchronizer using default properties
    framesync64 fs = framesync64_create(mycallback, (void*)&frame_counter);
    framesync64_print(fs);


    float complex new_buf[2*buf_len];
    /*
    for(i=0; i<2*buf_len; i++)
    {
        new_buf[i]=buf[i%buf_len];
    }
    */


    char *string = ReadFile("rx2.csv");
    const int RXbufferSize = 1000000;
    float complex RXbuf[RXbufferSize];
    int rx_limit = 0;
    //printf("aa : %s",string);
    char *token = strtok(string, "\n,");
    while(token!=NULL)
    {
        //char *dummyString;
        //strcpy(dummyString,lineString);
        //printf("%s",dummyString);
        float realPart = (float) atoi(token)/2048;
        token = strtok(NULL, "\n,");
        float imgPart = (float) atoi(token)/2048;



        RXbuf[rx_limit] = realPart + _Complex_I * imgPart;
        //printf("I: %f,",imgPart);
        //printf("Q: %f\n",realPart);
        token = strtok(NULL, "\n,");
        rx_limit++;
        if (rx_limit==RXbufferSize)
        {

            rx_limit=0;
        }
    }
    framesync64_execute(fs, RXbuf, RXbufferSize);
    //lineString = strtok(string, "\n");
    //printf("%s",lineString);
    // EXECUTE synchronizer and receive the frame in one block


    // DESTROY objects
    framesync64_destroy(fs);

    printf("received %u frames\n", frame_counter);
    printf("done.\n");
    return 0;


    printf("done.\n");
    return 0;
}
