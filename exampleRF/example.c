
Logo
libbladeRF  1.4.3
Nuand bladeRF library
Main Page
Related Pages
Modules
Data Structures
Files

Search
Device Configuration
This page presents an overview on how to configure a bladeRF device via the libbladeRF API. A complete boilerplate code listing may be found at the end of this page.

Acquiring a device handle

First, one must acquire a handle to a bladeRF device. Four common approaches to this are:

Open the first available device.
Open a device with a specified serial number.
Open a device with the specified USB bus and address values.
Choose a device from a list of all available devices.
The first three approaches can be implemented via a call to bladerf_open() or bladerf_open_with_devinfo(). Both of these functions offer the same functionality, but through a slightly different interface. Note that per the documentation of each of these functions, NULL can be specified as one of the arguments to specify, "open any available bladeRF device."

bladerf_open() takes a specially formatted "device identifier" string as an argument. This provides a simple means to give the user full control over how the desired device is specified, perhaps through a command-line argument or GUI text field.

bladerf_open_with_devinfo() uses a bladerf_devinfo structure to identify the device to open. This is generally a better choice when programatically deciding which device to open, as it alleviates the need to craft a device identifier string. This approach is taken in the below snippet, where argv[1] may contain a serial number string. In the case when argc < 2, the dev_info.serial field is left as its wildcard ("any value") field from the bladerf_init_devinfo() call.

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
The last approach in the beginning of this section is a good choice for programs that need to present a list of available devices to a user. A list of connected bladeRF devices may be retrieved via bladerf_get_device_list(). The contents of each bladerf_devinfo could then be presented to a user for selection, and the desired device's bladerf_devinfo can then be passed to bladerf_open_with_devinfo(). The device list can then be freed using bladerf_free_device_list().


Configuring Device parameters

After acquiring a handle to a bladeRF device, the following must be configured prior to transmitting or receiving samples.

Frequency
Gains
Sample Rate
Bandwidth
Note that all of the above are configured on a per-module (i.e, RX or TX) basis.

Selecting Appropriate Values

To better understand each of the available gains controls, see Figures 1, 2, and 3 in the LMS6002D datasheet. In general, one stage should be increased prior to increasing the following stage. For RX, increase the LNA gain, followed by RX VGA1, then RX VGA2. Similarly, increase TX VGA1 first, then TX VGA2.

The selection of sample rate and bandwidth are tightly coupled. In general, the sample rate must be high enough to ensure that all desired signals can be sampled without aliasing. However, note that the LMS6002D provides a discrete set of bandwidth options. This implies that one's sample rate selection will largely be influenced by the bandwidth option that most closely matches the desired value.

The bandwidth parameter controls the LMS6002D's RF front-end (RFFE) low-pass filter (LPF) setting. This should be configured to be less than the sample rate to ensure that the filter has reached maximum attenuation before reaching the desired sample rate value. Otherwise, noise and aliases may "fold" into the frequency range of interest.

Consult the plots in Figure 5 of the LMS6002D datasheet for the RX/TX filter responses. Note that the filter ranges are listed in the datasheet are listed in terms of 0 to Fs/2, whereas bladerf_set_bandwidth() assumes the total bandwidth (-Fs/2 to Fs/2) is being specified. Consider the plot for the 0.75 MHz filter (which corresponds to 1.5 MHz of total BW). At 1 MHz (2 MHZ of total BW), this filter offers over -45 dB of rejection. At ~1.8 MHz (3.6 MHz of total BW), this filter offers over -60 dB of rejection.

Therefore, when calling bladerf_set_bandwidth() with a value of 1.5 MHz, a sample rate 2 MHz would be a preferred minimum, and a sample rate >= 3.6 MHz would be an even better choice.

Applying a set of values

It is common for a program to apply all of these settings during its initialization. Therefore, the example in this page groups these parameters into a structure and then uses a configure_module() helper function to apply them:

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
As shown in the below implementation of configure_module(), the general procedure for applying a parameter involves calling the corresponding function and checking the return value.

Note that some functions, such as bladerf_set_sample_rate() and bladerf_set_bandwidth(), have optional actual output parameters that are set to NULL in this example. These actual values are used to provide feedback about the differences between the requested value and the actual value applied. Some error may occur in converting the sample rate into a rational fraction, as required by the underlying hardware. As previously, mentioned, the LMS600D provides a discrete set of RFFE LPF bandwidth settings. bladerf_set_bandwidth() will choose the closest bandwidth setting, and report this via the optional actual parameter.

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

Complete listing

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
Generated on Sat Jul 25 2015 16:01:38 for libbladeRF by   doxygen 1.8.9.1
