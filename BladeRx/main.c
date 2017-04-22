#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libbladeRF.h>
#include <time.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <complex.h>
#include <liquid/liquid.h>



/* The RX and TX modules are configured independently for these parameters */
struct module_config {
    bladerf_module module;
    unsigned int frequency;
    unsigned int bandwidth;
    unsigned int samplerate;
    /* Gains */
    bladerf_lna_gain rx_lna;
    int vga1;
    int vga2;
};

int configure_module(struct bladerf *dev, struct module_config *c)
{
    int status;
    status = bladerf_set_frequency(dev, c->module, c->frequency);
    if (status != 0) {
        fprintf(stderr, "Failed to set frequency = %u: %s\n",
                c->frequency, bladerf_strerror(status));
        return status;
    }
    status = bladerf_set_sample_rate(dev, c->module, c->samplerate, NULL);
    if (status != 0) {
        fprintf(stderr, "Failed to set samplerate = %u: %s\n",
                c->samplerate, bladerf_strerror(status));
        return status;
    }
    status = bladerf_set_bandwidth(dev, c->module, c->bandwidth, NULL);
    if (status != 0) {
        fprintf(stderr, "Failed to set bandwidth = %u: %s\n",
                c->bandwidth, bladerf_strerror(status));
        return status;
    }
    switch (c->module) {
        case BLADERF_MODULE_RX:
            /* Configure the gains of the RX LNA, RX VGA1, and RX VGA2  */
            status = bladerf_set_lna_gain(dev, c->rx_lna);
            if (status != 0) {
                fprintf(stderr, "Failed to set RX LNA gain: %s\n",
                        bladerf_strerror(status));
                return status;
            }
            status = bladerf_set_rxvga1(dev, c->vga1);
            if (status != 0) {
                fprintf(stderr, "Failed to set RX VGA1 gain: %s\n",
                        bladerf_strerror(status));
                return status;
            }
            status = bladerf_set_rxvga2(dev, c->vga2);
            if (status != 0) {
                fprintf(stderr, "Failed to set RX VGA2 gain: %s\n",
                        bladerf_strerror(status));
                return status;
            }
            break;
        case BLADERF_MODULE_TX:
            /* Configure the TX VGA1 and TX VGA2 gains */
            status = bladerf_set_txvga1(dev, c->vga1);
            if (status != 0) {
                fprintf(stderr, "Failed to set TX VGA1 gain: %s\n",
                        bladerf_strerror(status));
                return status;
            }
            status = bladerf_set_txvga2(dev, c->vga2);
            if (status != 0) {
                fprintf(stderr, "Failed to set TX VGA2 gain: %s\n",
                        bladerf_strerror(status));
                return status;
            }
            break;
        default:
            status = BLADERF_ERR_INVAL;
            fprintf(stderr, "%s: Invalid module specified (%d)\n",
                    __FUNCTION__, c->module);
    }
    return status;
}


static int init_sync(struct bladerf *dev)
{
    int status;
    /* These items configure the underlying asynch stream used by the sync
     * interface. The "buffer" here refers to those used internally by worker
     * threads, not the user's sample buffers.
     *
     * It is important to remember that TX buffers will not be submitted to
     * the hardware until `buffer_size` samples are provided via the
     * bladerf_sync_tx call.  Similarly, samples will not be available to
     * RX via bladerf_sync_rx() until a block of `buffer_size` samples has been
     * received.
     */
    const unsigned int num_buffers   = 16;
    const unsigned int buffer_size   = 8192;  /* Must be a multiple of 1024 */
    const unsigned int num_transfers = 8;
    const unsigned int timeout_ms    = 3500;
    /* Configure both the device's RX and TX modules for use with the synchronous
     * interface. SC16 Q11 samples *without* metadata are used. */
    status = bladerf_sync_config(dev,
                                 BLADERF_MODULE_RX,
                                 BLADERF_FORMAT_SC16_Q11_META,
                                 num_buffers,
                                 buffer_size,
                                 num_transfers,
                                 timeout_ms);
    if (status != 0) {
        fprintf(stderr, "Failed to configure RX sync interface: %s\n",
                bladerf_strerror(status));
        return status;
    }
    status = bladerf_sync_config(dev,
                                 BLADERF_MODULE_TX,
                                 BLADERF_FORMAT_SC16_Q11_META,
                                 num_buffers,
                                 buffer_size,
                                 num_transfers,
                                 timeout_ms);
    if (status != 0) {
        fprintf(stderr, "Failed to configure TX sync interface: %s\n",
                bladerf_strerror(status));
    }
    return status;
}



int sync_rx_meta(struct bladerf *dev, int16_t *samples, unsigned int samples_len, unsigned int samplerate,
                               unsigned int timeout_ms)
{
    int status;
    struct bladerf_metadata meta;
    unsigned int i;
    /* 150 ms timestamp increment */
    const uint64_t ts_inc_150ms = ((uint64_t) samplerate) * 150 / 1000;
    /* 1 ms timestamp increment */
    const uint64_t ts_inc_1ms = samplerate / 1000;
    memset(&meta, 0, sizeof(meta));
    /* Perform scheduled RXs by having meta.timestamp set appropriately
     * and ensuring the BLADERF_META_FLAG_RX_NOW flag is cleared. */
    meta.flags = 0;
    /* Retrieve the current timestamp */
    status = bladerf_get_timestamp(dev, BLADERF_MODULE_RX, &meta.timestamp);
    if (status != 0) {
        fprintf(stderr, "Failed to get current RX timestamp: %s\n",
                bladerf_strerror(status));
    } else {
        printf("Current RX timestamp: 0x%016"PRIx64"\n", meta.timestamp);
    }
    /* Schedule first RX to be 150 ms in the future */
    //meta.timestamp += ts_inc_150ms;
    /* Receive samples and do work on them */
    status = bladerf_sync_rx(dev, samples, samples_len,
                             &meta, 2 * timeout_ms);
    if (status != 0) {
        fprintf(stderr, "Scheduled RX failed: %s\n\n",
                bladerf_strerror(status));
    } else if (meta.status & BLADERF_META_STATUS_OVERRUN) {
        fprintf(stderr, "Overrun detected in scheduled RX. "
                "%u valid samples were read.\n\n", meta.actual_count);
    } else {
        printf("RX'd %u samples at t=0x%016"PRIx64"\n",
               meta.actual_count, meta.timestamp);
        fflush(stdout);
    }

    return status;
}


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

    // type-cast, de-reference, and increment frame counter
    unsigned int * counter = (unsigned int *) _userdata;
    (*counter)++;
    printf("Frame Counter: %d, \n",*counter);
    return 0;
}

/* Usage:
 *   libbladeRF_example_boilerplate [serial #]
 *
 * If a serial number is supplied, the program will attempt to open the
 * device with the provided serial number.
 *
 * Otherwise, the first available device will be used.
 */
int main(int argc, char *argv[])
{
    int status;
    unsigned int frame_counter   =   0;     // userdata passed to callback
    struct module_config config;
    struct bladerf *dev = NULL;
    struct bladerf_devinfo dev_info;
    fprintf(stderr,"Current Time: %d \n", time(NULL));

    /* Initialize the information used to identify the desired device
     * to all wildcard (i.e., "any device") values */
    bladerf_init_devinfo(&dev_info);
    /* Request a device with the provided serial number.
     * Invalid strings should simply fail to match a device. */
    if (argc >= 2) {
        strncpy(dev_info.serial, argv[1], sizeof(dev_info.serial) - 1);
    }
    status = bladerf_open_with_devinfo(&dev, &dev_info);

    if (status != 0) {
        fprintf(stderr, "Unable to open device: %s\n",
                bladerf_strerror(status));
        return 1;
    }

    fprintf(stderr,"Device is selected, Serial Number: %s \n",dev_info.serial);

    /* Set up RX module parameters */
    config.module     = BLADERF_MODULE_RX;
    config.frequency  = 2400000000;
    config.bandwidth  = 2000000;
    config.samplerate = 1000000;
    config.rx_lna     = BLADERF_LNA_GAIN_MAX;
    config.vga1       = 30;
    config.vga2       = 3;
    status = configure_module(dev, &config);
    if (status != 0) {
        fprintf(stderr, "Failed to configure RX module. Exiting.\n");
        goto out;
    }

    fprintf(stderr,"RX device is configured, Serial Number: %s \n",dev_info.serial);

    /* Set up TX module parameters */
    config.module     = BLADERF_MODULE_TX;
    config.frequency  = 2400000000;
    config.bandwidth  = 2000000;
    config.samplerate = 1000000;
    config.vga1       = -14;
    config.vga2       = 0;
    status = configure_module(dev, &config);
    if (status != 0) {
        fprintf(stderr, "Failed to configure TX module. Exiting.\n");
        goto out;
    }

    fprintf(stderr,"TX device is configured, Serial Number: %s \n",dev_info.serial);
    /* Application code goes here.
     *
     * Don't forget to call bladerf_enable_module() before attempting to
     * transmit or receive samples!
     */


    /* Initialize synch interface on RX and TX modules */
    status = init_sync(dev);
    if (status != 0) {
        goto out;
    }
    status = bladerf_enable_module(dev, BLADERF_MODULE_RX, true);
    if (status != 0) {
        fprintf(stderr, "Failed to enable RX module: %s\n",
                bladerf_strerror(status));
        goto out;
    }

    fprintf(stderr, "Success to enable RX module: %s\n", bladerf_strerror(status));

    status = bladerf_enable_module(dev, BLADERF_MODULE_TX, true);
    if (status != 0) {
        fprintf(stderr, "Failed to enable TX module: %s\n",
                bladerf_strerror(status));
        goto out;
    }

    fprintf(stderr, "Success to enable TX module: %s\n", bladerf_strerror(status));


    fprintf(stderr,"Current Time: %d \n", time(NULL));

    /* "User" samples buffers and their associated sizes, in units of samples.
     * Recall that one sample = two int16_t values. */
    int16_t *rx_samples = NULL;
    const unsigned int samples_len = 5000000; /* May be any (reasonable) size */
    /* Allocate a buffer to store received samples in */
    rx_samples = malloc(samples_len * 2 * sizeof(int16_t));
    if (rx_samples == NULL) {
        perror("malloc");
        return BLADERF_ERR_MEM;
    }

    status = init_sync(dev);
    if (status != 0) {
        fprintf(stderr, "Failed to SYNC module: %s\n",
                bladerf_strerror(status));
        goto out;
    }
        status = bladerf_enable_module(dev, BLADERF_MODULE_RX, true);
    if (status != 0) {
        fprintf(stderr, "Failed to enable RX module: %s\n",
                bladerf_strerror(status));
        goto out;
    }

    sync_rx_meta(dev,rx_samples,samples_len,config.samplerate,5000);

    // create frame synchronizer using default properties
    framesync64 fs = framesync64_create(mycallback, (void*)&frame_counter);
    framesync64_print(fs);
    float complex* rx_buf;
    rx_buf = malloc(samples_len * sizeof(float complex));
    if (rx_buf == NULL) {
        perror("malloc");
        return BLADERF_ERR_MEM;
    }


     for(int i=0; i<samples_len; i++)
    {
        rx_buf[i] = (float)rx_samples[2*i]/2048 + _Complex_I * (float) rx_samples[2*i+1]/2048;
    }

    framesync64_execute(fs, rx_buf, samples_len);
    framesync64_destroy(fs);

    printf("received %u frames\n", frame_counter);
    printf("done.\n");
    return 0;


out:
    fprintf(stderr,"Connection is closed.");
    bladerf_close(dev);
    return status;
}
