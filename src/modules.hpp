//
// Created by emreo on 22/06/2024.
//

#ifndef MODULES_HPP
#define MODULES_HPP

#include <systemc.h>
#include <iostream>
#include <cmath> // Ensure the inclusion log2
#include "structs.h"

using namespace sc_core;

extern "C" Result run_simulation(
        int cycles,
        unsigned tlbSize,
        unsigned tlbLatency,
        unsigned blockSize,
        unsigned v2BlockOffset,
        unsigned memoryLatency,
        size_t numRequests,
        struct Request requests[],
        const char *tracefile);


//emreo
SC_MODULE(TLB) {


    //we signal is transported to the memory
    //request first comes to the tlb. Checks whether the given virtual address' corresponding physical address
    //is to find in tlb. If yes -> TLB Hit -> less cycle -> sends the request directly to the needed physical address
    //with the signal WE, since the memory should know if it is a write or read signal.
    sc_in<int> we;

    sc_out<bool> activate;
    sc_in<bool> activateTLBinTLB;

    sc_in<uint32_t> virtual_address; // Virtual address for translation
    sc_out<uint32_t> physical_address; // Translated physical address

    std::vector<u_int32_t> tags; // TLB entries array
    std::vector<uint32_t> data; //TLB tag's data (physical page number)
    size_t tlb_size; // TLB size

    uint32_t blocksize;
    uint32_t v2bBlockOffset;
    uint32_t memoryLatency;
    uint32_t tlbLatency;

    // Number of bits
    uint32_t tag_bits;
    uint32_t index_bits;
    uint32_t offset_bits;

    //hits and misses
    sc_out<uint32_t> misses;
    sc_out<uint32_t> hits;

    sc_out<bool> hit_bool;
    size_t primitive_gate_count;
    uint32_t sum_of_indexb_offsetb;
    uint32_t limit;


    SC_CTOR(TLB);

    TLB(sc_module_name name, size_t size, uint32_t bsize, uint32_t v2bOffset, uint32_t memLatency, uint32_t tlbLat) :
            sc_module(name), tlb_size(size), blocksize(bsize), v2bBlockOffset(v2bOffset), memoryLatency(memLatency),
            tlbLatency(tlbLat),
            offset_bits(log2(blocksize)), tag_bits(32 - log2(blocksize) - log2(tlb_size)), index_bits(log2(tlb_size)), sum_of_indexb_offsetb(index_bits + offset_bits) {
        tags.resize(tlb_size);
        data.resize(tlb_size);
        limit = 1 << sum_of_indexb_offsetb;
        primitive_gate_count = calculate_gate_count(tlb_size, tag_bits, 32 - offset_bits - index_bits,
                                                    32); //change the 32 to the actual number of addition functions needed
        SC_THREAD(process_requests);
        sensitive << activateTLBinTLB;
    }
    //emreo:
    void process_requests() {
        while (true) {
            wait();
            //FOR EVERY REQUEST ADD THE TLB LATENCY
            //ACCORDING TO THE MISS/HIT -> ADD MEMORY-LATENCY
            // TODO ADD LATENCY (? it's added in the simulation)

            //YOU ARE ONLY ALLOWED TO READ FROM THE TLB! NO WRITE COMMAND IS ACCEPTED A.K.A THEY ARE IGNORED

            //write and read operations are checked in memory, so you only have to transport the given we to the memory
            // virtual address info:
            uint32_t v_address = virtual_address->read(); // currently wanted address
            uint32_t tag_of_v_address = v_address >> (offset_bits + index_bits);

            uint32_t index_of_v_address = v_address << tag_bits;
            index_of_v_address = index_of_v_address >> (32 - index_bits);

            uint32_t offset_of_v_address = v_address << (32 - offset_bits);
            offset_of_v_address = offset_of_v_address >> (32 - offset_bits);
            /*printf("tag of old element: %d\n", tags[index_of_v_address]); //can be activated if a comparison is wanted
            printf("tag of new virt address: %d\n",  tag_of_v_address);*/
            bool is_hit = (tags[index_of_v_address] == tag_of_v_address);
            hit_bool->write(is_hit);
            //page fault
            if(v_address <= limit){
                printf("\nCAUTION:\n");
                printf("number of index bits: %d\n", index_bits); //can be activated
                printf("offset bit number is: %d\n", offset_bits);
                fprintf(stderr,"PAGE FAULT - TRYING TO ACCESS DISK:\n"
                       "The tag of the given virtuall address might not be calculated properly.\n"
                       "Sum of index bits and offset bits are too big for the given virtuall address,\n"
                       "which might shift the virtuall address so much that the tag becomes 0.\n"
                       "(Tip: decrease the size of tlb (-t) or block size (-b) or make sure,\n all defined virtuall adresses in csv file are "
                       "greater than %d Bits)\n"
                       "The access to disk takes a long time. -> TIME-OUT: exiting!\n", (index_bits + offset_bits));
                exit(EXIT_FAILURE);
            }
            // according to the virtual address, it calculates the line that is looked for
            if (is_hit) {

                physical_address->write((data[index_of_v_address] << offset_bits) + offset_of_v_address);
                //increment the hit number
                hits->write(hits->read() + 1);
                std::cout << "\nTLB Hit: VA: 0x" << std::hex << v_address << std::endl;
                std::cout << "Physical address is: 0x" << std::hex << physical_address->read() << std::endl;
            } else {
                std::cout << "\nTLB Miss: VA: 0x" << std::hex << v_address << " \nCalculating physical address."
                          << std::endl;
                misses->write(misses->read() + 1);

                std::cerr << "offset: 0x" << std::hex << offset_of_v_address << std::endl;
                std::cerr << "index: 0x" << std::hex << index_of_v_address << std::endl;
                std::cerr << "tag: 0x" << std::hex << tag_of_v_address << std::endl;
                //change the tag at the given index:
                tags[index_of_v_address] = tag_of_v_address;
                //change the info at the given address:
                //data must contain the physical page NUMBER -> verschoben
                uint32_t virtual_page_number = v_address >> offset_bits;
                uint32_t physical_page_number = virtual_page_number + v2bBlockOffset;
                data[index_of_v_address] = physical_page_number;
                uint32_t phsy_a = (physical_page_number << offset_bits) + offset_of_v_address;
                physical_address->write(phsy_a);
                wait(SC_ZERO_TIME);
                std::cout << "\nUpdated TLB for given virtual address- VA: 0x" << std::hex << v_address
                          << " \nPhysical page number is: 0x"
                          << physical_page_number << std::endl << "Physical address is:0x" << std::hex
                          << physical_address->read() << std::endl;

                if (activate->read()) {
                    activate->write(false);
                } else {
                    activate->write(true);
                }
            }
        }
    }

    //berrak
    // Function to calculate the gate count
    static unsigned int calculate_gate_count(unsigned int num_entries, unsigned int tag_bits, unsigned int data_bits,
                                             unsigned int adder_func) {
        unsigned int tag_gates = num_entries * tag_bits *
                                 4; // (we need to store the tags) 4 gates per bit: "Beispielweise genügt es für das Speichern von einem Bit 4 primitive Gatter zu veranschlagen."
        unsigned int data_gates = num_entries * data_bits *
                                  4; // (we need to store the data written in TLB) 4 gates per bit: "Beispielweise genügt es für das Speichern von einem Bit 4 primitive Gatter zu veranschlagen."
        unsigned int comparison_gates =
                num_entries * tag_bits * 2; // (we need to compare the tags), im not sure about the 2 gates per bit
        unsigned int adder_gates = adder_func *
                                   150; //150 gates per adder needed: "Beispielweise genügt es für eine Addition von zwei 32-Bit Zahlen ≈ 150 primitive Gatter zu veranschlagen."

        return tag_gates + data_gates + comparison_gates + adder_gates;
    }
};
//bilge
SC_MODULE(MEMORY) {
    sc_in<int> we; // 1 for write and 0 for read
    sc_in<uint32_t> address; // Address for read/write
    sc_in<uint32_t> data; // for write
    sc_out<uint32_t> out_data; // for read
    sc_in<bool> clk;
    sc_in<bool> activate;


    std::unordered_map<uint32_t, u_int32_t> memory;

    void process_requests() {
        if (we->read()) {

            //berrak's addition to error messages

            //make sure that the address is valid
            //address can only be positive

            if (address->read() == 0) {
                std::cerr << "Error: Address " << std::dec << address->read() << " (0x" << std::hex << address->read()
                          << ") is not valid." << std::endl;
                return;
            }

            //make sure that the written data is valid
            //data can only be in decimal or hexademical format

            //data should be smaller than biggest 32-bit number (0x7FFFFFFF)
            //data should be bigger than the smallest 32-bit number (0x80000000)
            //not sure if we have to implement this


            auto it = memory.find(address->read());
            if (it != memory.end()) {
                it->second = data->read(); // puts the data in the map if address is already in the map
            } else {
                memory[address->read()] = data->read(); //puts the data in the map if address is not in the map
            }


        } else {
            //berrak's addition to error messages
            //make sure that the address is valid
            //address can only be positive

            if (address->read() == 0) {
                /*std::cerr << "Error: Address " << std::dec << address->read() << " (0x" << std::hex << address->read()
                          << ") is not valid." << std::endl;*/
                out_data->write(0);
                return;
            }

            auto it = memory.find(address->read());
            if (it != memory.end()) {
                out_data->write(it->second);
            } else {
                out_data->write(0); // Return 0 if address not found
            }
        }
    }

    SC_CTOR(MEMORY) {
        SC_METHOD(process_requests);
        sensitive << activate;
    }

};

//emreo:
SC_MODULE(CPU) {
    sc_in<bool> clk;
    sc_out<uint32_t> address;
    sc_out<uint32_t> data;
    sc_out<int> we;

    sc_out<bool> activateTLB;

    sc_in<size_t> tlb_latency;
    sc_in<size_t> memory_latency;

    sc_in<bool> hit;
    sc_out<size_t> remaining_cycles;
    sc_out<size_t> used_cycles;

    Request *requests;
    size_t number_of_requests;
    uint32_t req_index;
    uint32_t originally_given_cycle_number;

    SC_CTOR(CPU) : req_index(0) {
        SC_METHOD(clock_tick);
        sensitive << clk.pos();
    }

    CPU(sc_module_name name, size_t number_of_requests, Request requests[]) : sc_module(name), requests(requests),
                                                                              number_of_requests(number_of_requests),
                                                                              req_index(0),
                                                                              originally_given_cycle_number(500) {
        SC_THREAD(clock_tick);
        sensitive << clk.pos();
    }

    void clock_tick() {
        while (true) {
            if (remaining_cycles.read() > 0 && req_index < number_of_requests) {
                if (!requests[req_index].we) {
                    address.write(requests[req_index].addr);
                    we.write(requests[req_index].we);

                } else {
                    address.write(requests[req_index].addr);
                    we.write(requests[req_index].we);
                    data.write(requests[req_index].data);
                }
                if (activateTLB->read()) {
                    activateTLB->write(false);
                } else {
                    activateTLB->write(true);
                }
                wait(SC_ZERO_TIME);
                req_index++;
                uint32_t current_cycle_used = hit->read() ? tlb_latency->read() : tlb_latency->read() +
                                                                                  memory_latency->read();
                remaining_cycles->write(remaining_cycles->read() - current_cycle_used);
                used_cycles->write(used_cycles->read() + current_cycle_used);
            }
            wait();
        }
        /*if (used_cycles.read() > originally_given_cycle_number) {
            std::cerr << "Too many cycles needed!" << std::endl;
        }*/
    }
};

#endif
