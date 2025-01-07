/*
*	TO USE THE DAEMON: arguments must be things to track: cpu, ram ...
*/





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#include <mach/mach.h>


//Bit mask which contains the users wishes of the hardware to track => cpu = 1, ram = 2 ...
int8_t maskFunctionality = 0;




void sig_shutdown_handler(int signal) {
	printf("\nShutting down correctly...\n");
	exit(EXIT_SUCCESS);
}






double getRamUsage() {
	vm_size_t pageSize;
	mach_port_t machPort;
	mach_msg_type_number_t count;
	vm_statistics64_data_t vmStats;

	machPort = mach_host_self();
	count = sizeof(vmStats) / sizeof(natural_t);

	double ramUsage = -1;
	if (KERN_SUCCESS == host_page_size(machPort, &pageSize) && KERN_SUCCESS == host_statistics64(machPort, HOST_VM_INFO, (host_info64_t)&vmStats, &count)) {

		long long usedMemory = ((int64_t)vmStats.active_count + (int64_t)vmStats.inactive_count + (int64_t)vmStats.wire_count) *  (int64_t)pageSize;

		int mib[2] = {CTL_HW, HW_MEMSIZE};
		long long physicalMemory;
		size_t length = sizeof(int64_t);
		sysctl(mib, 2, &physicalMemory, &length, NULL, 0);

		ramUsage = ((double)usedMemory / physicalMemory) * 100.0;

		printf("Physical Memory: %lld\n", physicalMemory);
		printf("Memory Usage: %.2f%%\n", ramUsage);

	}

	return ramUsage;
}






int main(int argc, char const *argv[]) {

	if (argc < 2) {
		fprintf(stderr, "Atleast one argument has to be present! For example: cpu\n");
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



	/*-------		Functuality		  --------*/


	while(true) {
		printf("running on PID %d\n", getpid());


		// CPU
		if ((maskFunctionality & 1) == 1) {
			// TO DO
		}
		

		// RAM
		if ((maskFunctionality & 2) == 2) {
			double ramUsage = getRamUsage();
			
			if (ramUsage > -1 && ramUsage > 35) {
				const char *command = "osascript -e 'display notification \"RAM is over 95% full\" with title \"High RAM Usage\"'";
				system(command);
				sleep(30);
			}
		}

		sleep(8);
	}
}