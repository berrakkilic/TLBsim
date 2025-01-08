//
// Created by emreo on 22/06/2024.
//
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include "structs.h"

//emreo:
extern struct Result run_simulation(int cycles,
                                    unsigned tlbSize,
                                    unsigned tlbLatency,
                                    unsigned blockSize,
                                    unsigned v2BlockOffset,
                                    unsigned memoryLatency,
                                    size_t numRequests,
                                    struct Request requests[],
                                    const char *tracefile);


char csv_filename[1024];  // Adjust the size as needed
const char *default_prefix = "example/";
unsigned cycles = 80000; // Standardwert für Zyklen
unsigned blocksize = 4096; // Defaultwert für Blockgröße in Bytes -> TLB Kachelgroesse als 4 KiB (2^12 Byte) genommen
unsigned v2b_offset = 0; // Standardwert für v2b block offset
unsigned tlb_size = 64; // Standardwert für die Größe des TLB in Einträgen
unsigned tlb_latency = 1; // Standardwslwert für TLB Latenz

// Intel-Core i7-10700K                &&        AMD Ryzen 9 3900X
//357 cycles at max turbo frequency    &&        322 Cycles at max turbo frequency
unsigned memory_latency = 300; // Standardwert für Speicherlatenz
unsigned *tracefile = NULL; // Kein Tracefile standardmäßig
char *inputfile = NULL; // has to be changed from NULL to the default file


void printOptions() {
    printf("Usage: program_name [options]\n");
    printf("Options:\n");
    printf("  -c, --cycles <number>              Number of cycles (default: 80000)\n");
    printf("  -b, --blocksize <number>           Block size in bytes (default: 4096 bytes = 4KiB) \n");
    printf("  -v, --v2b-block-offset <number>    v2b block offset (default: 0)\n");
    printf("  -t, --tlb-size <number>            TLB size in entries (default: 64)\n");
    printf("  -l, --tlb-latency <number>         TLB latency in cycles (default: 1)\n");
    printf("  -m, --memory-latency <number>      Main memory latency in cycles (default: 300)\n");
    printf("  -f, --tf <filename>                Output file for a trace file (no default)\n");
    printf("  -h, --help                         Displays this help information.\n");
    printf("  <filename>                         Input file containing data to be processed\n");
}

//emreo:
struct option long_options[] = {
        {"cycles", required_argument, 0, 'c'},
        {"blocksize", required_argument, 0, 'b'},
        {"v2b-block-offset", required_argument, 0, 'v'},
        {"tlb-size", required_argument, 0, 't'},
        {"tlb-latency", required_argument, 0, 'l'},
        {"memory-latency", required_argument, 0, 'm'},
        {"tf", required_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {"example", required_argument, 0, 'e' },
        {0, 0, 0, 0}
};
int calculate_req_number(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening file");
        return -1;
    }

    int lines_count = 0;
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        lines_count++;
    }

    fclose(file);
    return lines_count;
}

//CSV FILE:


//bilge und emre
void process_csv_line(const char *line, struct Request *request) {
    char access_type;
    char address_str[32];
    char data_str[32];
    int num_fields;

    num_fields = sscanf(line, "%c,%31[^,],%31s", &access_type, address_str, data_str);
    if (access_type != 'W' && access_type != 'R') {
        fprintf(stderr, "ERROR: first argument must be either 'W' or 'R'!\n");
        exit(EXIT_FAILURE);
    }
    // Determine we value
    request->we = (access_type == 'W') ? 1 : 0;

    // Convert address
    if (address_str[0] == '0' && (address_str[1] == 'x' || address_str[1] == 'X')) {
        request->addr = strtol(address_str, NULL, 16);
    } else {
        request->addr = strtol(address_str, NULL, 10);
    }

    // Convert data if present and we is 1
    if (request->we == 1) {
        //write operation
        if(num_fields <=2 ){
            fprintf(stderr, "ERROR: not enough arguments defined for write (W) operarion!");
            printf("usage: <W,<address>,<data>>\n");
            exit(EXIT_FAILURE);
        }
        if (data_str[0] == '0' && (data_str[1] == 'x' || data_str[1] == 'X')) {
            request->data = strtol(data_str, NULL, 16);
        } else {
            request->data = strtol(data_str, NULL, 10);
        }
    } else {
        //read
        if(num_fields == 3){
            fprintf(stderr, "ERROR: too many arguments for read (R) instruction!\n");
            exit(EXIT_FAILURE);
        }
        request->data = 0;
    }
}

void read_csv_file(const char *filename, struct Request *requests) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening file\n");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int index = 0;
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character if present
        line[strcspn(line, "\n")] = '\0';
        process_csv_line(line, &requests[index]);
        index++;
    }
    fclose(file);
}


//emreo:
int main(int argc, char *argv[]) {
    int option_index = 0;
    int c;

    if ( argc <= 1 ){
        printf("invalid input!\n");
        printf("press 'h' for help");
        printOptions();
        return 1;
    }
    //TODO:
    //we still need to create an exception class
    // that will fetch the input mistakes of the users
    //emreo:

    // Default-Werte definieren

    while ((c = getopt_long(argc, argv, "c:b:v:t:l:m:f:he:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'c':
                cycles = atoi(optarg);
                break;
            case 'b':
                blocksize = atoi(optarg);
                if(blocksize <= 0 || (blocksize & (blocksize - 1)) != 0){
                    fprintf(stderr, "Error: Block size must be a power of two.\n");
                    return 1;
                }
                break;
            case 'v':
                v2b_offset = atoi(optarg);
                break;
            case 't':
                tlb_size = atoi(optarg);
                break;
            case 'l':
                tlb_latency = atoi(optarg);
                break;
            case 'm':
                memory_latency = atoi(optarg);
                break;
            case 'f':
                tracefile = strdup(optarg);
                break;
            case 'h':
                printOptions();
                exit(EXIT_SUCCESS);
            case 'e':
                if (optarg != NULL) {
                    snprintf(csv_filename, sizeof(csv_filename), "%s%s", default_prefix, optarg);
                } else {
                    fprintf(stderr, "Option -e requires an argument.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "Unknown option or missing argument. Use -h or --help for more information.\n");
                exit(EXIT_FAILURE);
        }
    }

    //bilge
    //After all options have been processed, optind will point to the first non-option argument.
    int number_of_req = calculate_req_number(csv_filename);
    if (number_of_req <= 0) {
        fprintf(stderr, "Error reading csv-file or file is empty\n");
        return 1;
    }

    struct Request *requests = malloc(number_of_req * sizeof(struct Request));
    if (!requests) {
        fprintf(stderr, "Error allocating memory for requests");
        return 1;
    }


    read_csv_file(csv_filename, requests);


    struct Result result = run_simulation(cycles, tlb_size, tlb_latency, blocksize, v2b_offset, memory_latency,
                   number_of_req, requests, tracefile);

    if(result.cycles > cycles){
        printf("\nCAUTION: \nCycle number for the given command series exceeds the given number of cycles. "
               "The result might return a wrong cycle value.\n ");
    }

    printf("\nRESULT:\n");
    printf("cycles done: %d\n", result.cycles);
    printf("miss number:" "%d\n",result.misses);
    printf("hit number:" "%d\n",result.hits);
    printf("primitive gate count: %d\n", result.primitiveGateCount);

    free(requests);
    return 0;

}
