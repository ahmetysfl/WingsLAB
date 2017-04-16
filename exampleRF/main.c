#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libbladeRF.h>
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

struct bladerf * open_bladerf_from_serial(const char *serial)
{
    int status;
    struct bladerf *dev;
    struct bladerf_devinfo info;
    /* Initialize all fields to "don't care" wildcard values.
     *
     * Immediately passing this to bladerf_open_with_devinfo() would cause
     * libbladeRF to open any device on any available backend. */
    bladerf_init_devinfo(&info);
    /* Specify the desired device's serial number, while leaving all other
     * fields in the info structure wildcard values */
    strncpy(info.serial, serial, BLADERF_SERIAL_LENGTH - 1);
    info.serial[BLADERF_SERIAL_LENGTH - 1] = '\0';
    status = bladerf_open_with_devinfo(&dev, &info);
    if (status == BLADERF_ERR_NODEV) {
        printf("No devices available with serial=%s\n", serial);
        return NULL;
    } else if (status != 0) {
        fprintf(stderr, "Failed to open device with serial=%s (%s)\n",
                serial, bladerf_strerror(status));
        return NULL;
    } else {
        return dev;
    }
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
    struct module_config config;
    struct bladerf *dev = NULL;
    struct bladerf_devinfo dev_info;

    printf("Serial number is set : f005b46aa0600603344a9c3d1c684125 \n");

    /* Initialize the information used to identify the desired device
     * to all wildcard (i.e., "any device") values */
    bladerf_init_devinfo(&dev_info);
    /* Request a device with the provided serial number.
     * Invalid strings should simply fail to match a device. */
    if (argc >= 2) {
        strncpy(dev_info.serial, argv[1], sizeof(dev_info.serial) - 1);
    }

    /* Initialize the information used to identify the desired device
     * to all wildcard (i.e., "any device") values */
    //bladerf_init_devinfo(&dev_info);
    /* Request a device with the provided serial number.
     * Invalid strings should simply fail to match a device. */
    status = bladerf_open_with_devinfo(&dev, &dev_info);
    if (status != 0) {
        fprintf(stderr, "Unable to open device: %s\n",
                bladerf_strerror(status));
        return 1;
    }

    printf("Device is selected, Serial Number: %s",dev_info.serial);

    /* Set up RX module parameters */
    config.module     = BLADERF_MODULE_RX;
    config.frequency  = 910000000;
    config.bandwidth  = 2000000;
    config.samplerate = 300000;
    config.rx_lna     = BLADERF_LNA_GAIN_MAX;
    config.vga1       = 30;
    config.vga2       = 3;
    status = configure_module(dev, &config);
    if (status != 0) {
        fprintf(stderr, "Failed to configure RX module. Exiting.\n");
        goto out;
    }
    /* Set up TX module parameters */
    config.module     = BLADERF_MODULE_TX;
    config.frequency  = 918000000;
    config.bandwidth  = 1500000;
    config.samplerate = 250000;
    config.vga1       = -14;
    config.vga2       = 0;
    status = configure_module(dev, &config);
    if (status != 0) {
        fprintf(stderr, "Failed to configure TX module. Exiting.\n");
        goto out;
    }
    /* Application code goes here.
     *
     * Don't forget to call bladerf_enable_module() before attempting to
     * transmit or receive samples!
     */
out:
    bladerf_close(dev);
    return status;
}

