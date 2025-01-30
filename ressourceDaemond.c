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


// Bit mask which contains the users wishes of the hardware to track => cpu = 1,
// ram = 2 ...
int8_t maskFunctionality = 0;


void sig_shutdown_handler(int signal) {

    // TODO: Check if more ressources need to be freed

    printf("\nShutting down correctly...\n");
    exit(EXIT_SUCCESS);
}



/*
 * Receive the total cpu-usage-estimate (user + system) from top
 */
float calculate_cpu_usage() {
    FILE *fp;
    char buffer[256];
    float user, system, idle, cpu_usage = 0.0;

    // Run the top command to grep the cpu usage and capture its ouput stream
    fp = popen("top -l 1 | grep 'CPU usage'", "r");
    if (fp == NULL) {
        perror(
            "popen failed when trying to read in cpu-usage determined by top");
        return -1;
    }

    // Read the stream into a buffer and parse it
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        sscanf(buffer, "CPU usage: %f%% user, %f%% sys, %f%% idle", &user,
               &system, &idle);
        cpu_usage = (user + system) / 100.0;
    }

    pclose(fp);

    printf("CPU usage: %f\n", cpu_usage);
    return cpu_usage;
}


/*
 * Calculate the currently used ram and the swap pressure of the system based on
 * Mach-Kernel statistics
 */
float calculate_memory_usage() {
    int64_t total_memory = 0;
    size_t size = sizeof(total_memory);
    int64_t page_size = sysconf(_SC_PAGESIZE);

    // Get total amount of system memory
    if (sysctlbyname("hw.memsize", &total_memory, &size, NULL, 0) != 0) {
        perror("Unable to retrieve system memory size: sysctlbyname");
        return -1;
    }

    // Read Mach-Kernel Virtual Memory statistics
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vm_stats;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          (host_info64_t)&vm_stats, &count) != KERN_SUCCESS) {
        fprintf(stderr, "Failed to fetch VM statistics\n");
        return -1;
    }

    // Calculate the used memory
    int64_t active_memory = vm_stats.active_count * page_size;
    int64_t wired_memory = vm_stats.wire_count * page_size;
    int64_t compressed_memory =
        vm_stats.compressor_page_count * page_size *
        1.3; // Acts as the heuristicall evaluated compression-rate

    int64_t used_memory = active_memory + wired_memory + compressed_memory;

    float ram_usage = (used_memory / (float)total_memory);
    printf("RAM USAGE: %.2f\n", ram_usage);

    return ram_usage;
}



/*
 * Calculate the swap pressure of the system based on Mach-Kernel statistics
 */
float calculate_swap_pressure() {
    size_t size = sizeof(int64_t);
    int mib[2];

    // Get swap statistics
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
    printf("SWAP PRESSURE: %.2f\n", swap_pressure);

    return swap_pressure;
}



int main(int argc, char const *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Atleast one argument to track has to be present! For "
                        "example: cpu\n");
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

    // Make sure only change of rates are notified
    bool notify_again = true;
    // Make sure there are enough time gaps between notifications
    int intervall_between_notifies_memory = 0;
    int intervall_between_notifies_cpu = 0;


    /*-------		Functuality		  --------*/

    while (true) {
        printf("running on PID %d\n", getpid());

        // CPU
        if ((maskFunctionality & 1) == 1) {
            float cpu_usage = calculate_cpu_usage();

            if (cpu_usage > -1 && cpu_usage > 0.8 &&
                intervall_between_notifies_cpu > 8) {
                const char *command =
                    "osascript -e 'display notification "
                    "\"High CPU USAGE > 80% for long period of time\" with "
                    "title \"High RAM Usage\"'";
                system(command);
                intervall_between_notifies_cpu = 0;
            }
            intervall_between_notifies_cpu++;
        }

        // RAM
        if ((maskFunctionality & 2) == 2) {
            float ram_usage = calculate_memory_usage();
            float swap_pressure = calculate_swap_pressure();

            if (ram_usage > -1 && swap_pressure > -1 && ram_usage > 0.8 &&
                swap_pressure > 0.6) {

                if (notify_again && intervall_between_notifies_memory > 5) {
                    const char *command =
                        "osascript -e 'display notification "
                        "\"MEMORY USAGE > 80% \nSWAP PRESSURE > 60%\" with "
                        "title \"High RAM Usage\"'";
                    system(command);
                    notify_again = false;
                    intervall_between_notifies_memory = 0;
                }

            } else {
                notify_again = true;
            }

            intervall_between_notifies_memory++;
        }

        sleep(20); // Check for changes every 20s
    }
}