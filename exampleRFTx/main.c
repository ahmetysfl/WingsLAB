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


void delay(int milliseconds)
{
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}

static int init_sync_tx(struct bladerf *dev)
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
                                 BLADERF_MODULE_TX,
                                 BLADERF_FORMAT_SC16_Q11_META,
                                 num_buffers,
                                 buffer_size,
                                 num_transfers,
                                 timeout_ms);
    if (status != 0) {
        fprintf(stderr, "Failed to configure TX sync interface: %s\n",
                bladerf_strerror(status));
        return status;
    }

    fprintf(stderr, "Success to configure TX sync interface: %s\n", bladerf_strerror(status));

    return status;
}

static int init_sync_rx(struct bladerf *dev)
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

    fprintf(stderr, "Success to configure RX sync interface: %s\n", bladerf_strerror(status));

    return status;
}

int wait_for_timestamp(struct bladerf *dev, bladerf_module module,
                       uint64_t timestamp, unsigned int timeout_ms)
{
    int status;
    uint64_t curr_ts = 0;
    unsigned int slept_ms = 0;
    bool done;
    do {
        status = bladerf_get_timestamp(dev, module, &curr_ts);
        done = (status != 0) || curr_ts >= timestamp;
        if (!done) {
            if (slept_ms > timeout_ms) {
                done = true;
                status = BLADERF_ERR_TIMEOUT;
            } else {
                usleep(10000);
                slept_ms += 10;
            }
        }
    } while (!done);
    return status;
}


int sync_tx_meta_sched_example(struct bladerf *dev,
                             int16_t *samples, unsigned int num_samples,
                             unsigned int tx_count, unsigned int samplerate,
                             unsigned int timeout_ms)
{
    int status = 0;
    unsigned int i;
    struct bladerf_metadata meta;
    memset(&meta, 0, sizeof(meta));
    /* Send entire burst worth of samples in one function call */
    meta.flags = BLADERF_META_FLAG_TX_BURST_START |
                 BLADERF_META_FLAG_TX_BURST_END;
    /* Retrieve the current timestamp so we can schedule our transmission
     * in the future. */
    status = bladerf_get_timestamp(dev, BLADERF_MODULE_TX, &meta.timestamp);
    if (status != 0) {
        fprintf(stderr, "Failed to get current TX timestamp: %s\n",
                bladerf_strerror(status));
        return status;
    } else {
        printf("Current TX timestamp: %016"PRIu64"\n", meta.timestamp);
    }
    for (i = 0; i < tx_count && status == 0; i++) {
        /* Schedule burst 5 ms into the future */
        meta.timestamp += samplerate / 200;
        status = bladerf_sync_tx(dev, samples, num_samples, &meta, timeout_ms);
        if (status != 0) {
            fprintf(stderr, "TX failed: %s\n", bladerf_strerror(status));
            return status;
        } else {
            printf("TX'd @ t=%016"PRIu64"\n", meta.timestamp);
        }
    }
    /* Wait for samples to finish being transmitted. */
    if (status == 0) {
        meta.timestamp += 2 * num_samples;
        status = wait_for_timestamp(dev, BLADERF_MODULE_TX,
                                    meta.timestamp, timeout_ms);
        if (status != 0) {
            fprintf(stderr, "Failed to wait for timestamp.\n");
        }
    }
    return status;
}

int sync_rx_example(struct bladerf *dev, int16_t * rx_samples, int samples_len,struct bladerf_metadata *meta)
{
    int status, ret;
    bool have_tx_data = false;

    /* Initialize synch interface on RX and TX modules */
    status = init_sync_rx(dev);
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

    // Receive Samples From Modules
    status = bladerf_sync_rx(dev, rx_samples, samples_len, meta, 10000);
    if (status != 0) {
        fprintf(stderr, "Failed to RX samples: %s\n", bladerf_strerror(status));
    }
    status = 1;



out:
    ret = status;
    /* Disable RX module, shutting down our underlying RX stream */
    status = bladerf_enable_module(dev, BLADERF_MODULE_RX, false);
    if (status != 0) {
        fprintf(stderr, "Failed to disable RX module: %s\n",
                bladerf_strerror(status));
    }
    /* Disable TX module, shutting down our underlying TX stream */
    status = bladerf_enable_module(dev, BLADERF_MODULE_TX, false);
    if (status != 0) {
        fprintf(stderr, "Failed to disable TX module: %s\n",
                bladerf_strerror(status));
    }
    return ret;
}


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
    time_t rawtime;
    struct tm * timeinfo;

    printf("LIQUID_FRAME64_LEN: %d",LIQUID_FRAME64_LEN);
    rawtime = time(NULL);
    fprintf(stderr,"Current Time: %d \n", time(NULL));
    delay(1000);
    rawtime = time(NULL);
    fprintf(stderr,"Current local time and date: %d \n", rawtime);
    fprintf(stderr,"Serial is selected. \n");
    int status;
    struct module_config config;
    struct bladerf *dev = NULL;
    struct bladerf_devinfo dev_info;

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
    }

   fprintf(stderr,"TX device is configured, Serial Number: %s \n",dev_info.serial);

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


    /* Application code goes here.
     *
     * Don't forget to call bladerf_enable_module() before attempting to
     * transmit or receive samples!
     */

    /* "User" samples buffers and their associated sizes, in units of samples.
     * Recall that one sample = two int16_t values. */
    int16_t *tx_samples = NULL;
    const unsigned int samples_len = LIQUID_FRAME64_LEN; /* May be any (reasonable) size */
    /* Allocate a buffer to store received samples in */

    tx_samples = malloc(samples_len * 2 * sizeof(int16_t));
    if (tx_samples == NULL) {
        perror("malloc");
        return BLADERF_ERR_MEM;
    }

    for(int i=0; i<samples_len; i++)
    {
        float real_part = crealf(buf[i]);
        float img_part = cimagf(buf[i]);
        tx_samples[2*i] = cnv2digital(real_part);
        tx_samples[2*i + 1] = cnv2digital(img_part);
    }

    sync_tx_meta_sched_example(dev,tx_samples,samples_len,1,config.samplerate,5000);

out:
    bladerf_close(dev);
    return status;
}
