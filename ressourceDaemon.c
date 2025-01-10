/*
 *	TO USE THE DAEMON: arguments must be things to track: cpu, ram ...
 */

#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

// Bit mask <w<hich contains the users wishes of the hardware to track => cpu = 1,
// ram = 2 ...
int8_t maskFunctionality = 0;

void sig_shutdown_handler(int signal) {
    printf("\nShutting down correctly...\n");
    exit(EXIT_SUCCESS);
}



/*
 * Calculate the currently used ram and the swap pressure of the system based on
 * Mach-Kernel statistics
*/
float calculate_ram_usage() {
    int64_t total_memory = 0;
    size_t size = sizeof(total_memory);
    int64_t page_size = sysconf(_SC_PAGESIZE);


    // get total amount of system memory
    if (sysctlbyname("hw.memsize", &total_memory, &size, NULL, 0) != 0) {
        perror("Unable to retrieve system memory size: sysctlbyname");
        return -1;
    }


    // read Mach-Kernel Virtual Memory statistics
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vm_stats;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          (host_info64_t)&vm_stats, &count) != KERN_SUCCESS) {
        fprintf(stderr, "Failed to fetch VM statistics\n");
        return -1;
    }

    // calculate the used memory
    int64_t active_memory = vm_stats.active_count * page_size;
    int64_t wired_memory = vm_stats.wire_count * page_size;
    int64_t compressed_memory =
        vm_stats.compressor_page_count * page_size *
        1.3; // acts as the heuristicall evaluated compression-rate

    int64_t used_memory = active_memory + wired_memory + compressed_memory;


    float ram_usage = (used_memory / (float)total_memory);
    printf("RAM USAGE: %.2f%%\n", ram_usage);

    return ram_usage;
}



/*
 * Calculate the swap pressure of the system based on Mach-Kernel statistics
 */
float calculate_swap_pressure() {
    size_t size = sizeof(int64_t);
    int mib[2];
    uint64_t swap_total;

    // get swap statistics
    mib[0] = CTL_VM;
    mib[1] = VM_SWAPUSAGE;
    struct xsw_usage swap_usage;

    size = sizeof(swap_usage);
    if (sysctl(mib, 2, &swap_usage, &size, NULL, 0) == -1) {
        perror("sysctl VM_SWAPUSAGE failed");
        return -1;
    }

    float swap_pressure =
        ((float)swap_usage.xsu_used / (float)swap_usage.xsu_total);
    printf("SWAP PRESSURE: %.2f%%\n", swap_pressure);

    return swap_pressure;
}




int main(int argc, char const *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
                "Atleast one argument to track has to be present! For example: cpu\n");
        exit(EXIT_FAILURE);
    }

    // set the bitmask
    for (int i = 1; argv[i] != NULL; ++i) {
        if (strcmp(argv[i], "cpu") == 0) {
            maskFunctionality = maskFunctionality | 1;
        } else if (strcmp(argv[i], "ram") == 0) {
            maskFunctionality = maskFunctionality | 2;
        }
    }

    // Signal Handlers
    signal(SIGABRT, sig_shutdown_handler);
    signal(SIGINT, sig_shutdown_handler);
    signal(SIGTERM, sig_shutdown_handler);


    // make sure only change of rates are notified
    bool notify_again = true;
    // make sure there are enough time gaps between notifications
    int intervall_between_notifies = 0;



    /*-------		Functuality		  --------*/

    while (true) {
        printf("running on PID %d\n", getpid());

        // CPU
        if ((maskFunctionality & 1) == 1) {
            // TODO
        }


        // RAM
        if ((maskFunctionality & 2) == 2) {
            float ram_usage = calculate_ram_usage();
            float swap_pressure = calculate_swap_pressure();

            
            if (ram_usage > -1 && swap_pressure > -1 && ram_usage > 0.8 &&
                swap_pressure > 0.6) {

                if (notify_again && intervall_between_notifies > 5) {
                    const char *command = "osascript -e 'display notification "
                                          "\"MEMORY USAGE > 80% \nSSWAP PRESSURE > 60%\" with "
                                          "title \"High RAM Usage\"'";
                    system(command);
                    notify_again = false;
                    intervall_between_notifies = 0;
                }
                
                
            } else {
                notify_again = true;
            }
        }

        intervall_between_notifies++;
        sleep(20); // check for changes every 20s
    }
}