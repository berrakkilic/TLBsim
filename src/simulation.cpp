//
// Created by emreo on 22/06/2024.
//

//berrak:
#include <systemc.h>
#include <unordered_map>
#include "modules.hpp"
#include "structs.h"

Result run_simulation(
        int cycles,
        unsigned tlbSize,
        unsigned tlbLatency,
        unsigned blockSize,
        unsigned v2BlockOffset,
        unsigned memoryLatency,
        size_t numRequests,
        struct Request requests[],
        const char *tracefile) {

    //establish where we will save our results and the modules we will use
    std::cout << "Number of requests: " << numRequests << std::endl ;

    Result result = {0, 0, 0, 0};

    if (requests == nullptr) {
        std::cout << "Nothing to process! Please make a request." << std::endl;
        return result;
    }

    TLB tlb("TLB", tlbSize, blockSize, v2BlockOffset, memoryLatency, tlbLatency);

    std::cout << "Number of gate counts used in this simulation with given values are: " <<
    tlb.primitive_gate_count << std::endl;

    if (tlbSize == 0) {
        std::cerr << "Can't save anything to TLB!" << std::endl;
        return result;
    }

    MEMORY memory("MEMORY");
    CPU cpu("CPU", numRequests, requests);

    // Signal declarations
    sc_signal<int> we_signal;
    sc_signal<uint32_t> virtual_address_signal;
    sc_signal<uint32_t> physical_address_signal;
    sc_signal<uint32_t> data_signal;
    sc_signal<uint32_t> out_data_signal;
    sc_signal<uint32_t> misses;
    sc_signal<uint32_t> hits;
    sc_signal<bool> hit;
    sc_signal<size_t> remaining_cycles;
    sc_signal<size_t> used_cycles;
    sc_signal<size_t> cpu_tlb_latency;
    sc_signal<size_t> cpu_memory_latency;
    sc_clock clk("clk", sc_time(1, SC_SEC)); //random clock, can be changed
    sc_signal<bool> activate_memory;
    sc_signal<bool>activateT;

    // Connect signals to TLB
    tlb.we(we_signal);
    tlb.virtual_address(virtual_address_signal);
    tlb.physical_address.bind(physical_address_signal);
    tlb.hit_bool(hit);
    tlb.misses.bind(misses);
    tlb.hits.bind(hits);
    tlb.activateTLBinTLB(activateT);
    //addition
    tlb.activate(activate_memory);


    // Connect signals to Memory
    memory.we(we_signal);
    memory.address(physical_address_signal);
    memory.data(data_signal);
    memory.out_data(out_data_signal);
    memory.clk(clk);
    memory.activate(activate_memory);

    // Connect signals to CPU
    cpu.clk(clk);
    cpu.remaining_cycles(remaining_cycles);
    cpu.used_cycles(used_cycles);
    cpu.tlb_latency(cpu_tlb_latency);
    cpu.memory_latency(cpu_memory_latency);
    cpu.data(data_signal);
    cpu.we(we_signal);
    cpu.address(virtual_address_signal);
    cpu.hit(hit);
    cpu.activateTLB(activateT);
    //request array binding:


    // Initialize CPU cycle signals, am not sure if im doing used and remaining cycles right
    remaining_cycles.write(cycles);
    used_cycles.write(0);
    cpu_tlb_latency.write(tlbLatency);
    cpu_memory_latency.write(memoryLatency);

    //iterate through each request for the tlb

    /*for (size_t i = 0; i < numRequests; i++) {

        //can't write to zero address, but can read?
        if (requests[i].addr == 0) {
            std::cout << "No valid address to process! Please provide a valid address." << std::endl;
        }

        //connect the virtual address to the tlb

        virtual_address_signal.write(requests[i].addr);
        we_signal.write(requests[i].we);
        data_signal.write(requests[i].data);

        //see if what we are looking for is in the tlb

        sc_start(); //random time, can be changed

        if (requests[i].we == 0) {
            // Read from address
            requests[i].data = out_data_signal.read();
        } else {
            // Write to address???
        }

    }*/

    /*for (size_t i = 0; i < numRequests; ++i) {
        virtual_address_signal.write(requests[i].addr);
        we_signal.write(requests[i].we);
        data_signal.write(requests[i].data);

        sc_start(1 * cycles, SC_SEC); //random time, can be changed

        if (requests[i].we == 0) {
            requests[i].data = out_data_signal.read();
        }
    }*/

    sc_trace_file *traceFile = nullptr;
    if (tracefile != nullptr) {
        traceFile = sc_create_vcd_trace_file(tracefile);
        sc_trace(traceFile, we_signal, "we");
        sc_trace(traceFile, virtual_address_signal, "virtual_address");
        sc_trace(traceFile, physical_address_signal, "physical_address");
        sc_trace(traceFile, data_signal, "data");
        sc_trace(traceFile, out_data_signal, "out_data");
        sc_trace(traceFile, hits, "hits");
        sc_trace(traceFile, misses, "misses");
        sc_trace(traceFile, hit, "hit");
        sc_trace(traceFile, clk, "clock");
    }

    // Run the simulation
    sc_start(cycles, SC_SEC);

    if (traceFile) {
        sc_close_vcd_trace_file(traceFile);
    }

    //calculate number of cycles needed to complete the task + save the number of misses and hits
    //if hit: only tlb
    //if miss: tlb and memory
    result.cycles = hits.read() * tlbLatency + misses.read() * (tlbLatency + memoryLatency);
    result.misses = misses.read();
    result.hits = hits.read();
    result.primitiveGateCount = tlb.primitive_gate_count;

    //check if the number of cycles is over the maximum amount of cycles provided
    if (result.cycles > static_cast<size_t>(cycles)) {
        std::cerr
                << "The number of cycles needed to complete this task is over the maximum amount of cycles provided. The program will now terminate with the false values."
                << std::endl;
        result.cycles = SIZE_MAX;
        return result; //wrong return value?
    }

    return result;

}

//default sc_main, do NOT delete for build
int sc_main(int argc, char *argv[]) {
    return 1;
}
