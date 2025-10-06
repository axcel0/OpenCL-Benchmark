#pragma once

#include "utilities.hpp"
#include <fstream>

// Simple JSON export functionality for benchmark results
class BenchmarkResults {
public:
	string device_name;
	string device_vendor;
	string driver_version;
	string opencl_version;
	uint compute_units;
	uint clock_frequency;
	uint memory_mb;
	float tflops_theoretical;
	
	// Compute results
	float fp64_tflops;
	float fp32_tflops;
	float fp16_tflops;
	float int64_tiops;
	float int32_tiops;
	float int16_tiops;
	float int8_tiops;
	
	// Memory bandwidth results
	float mem_bw_coalesced_read;
	float mem_bw_coalesced_write;
	float mem_bw_misaligned_read;
	float mem_bw_misaligned_write;
	
	// PCIe bandwidth results
	float pcie_bw_send;
	float pcie_bw_receive;
	float pcie_bw_bidirectional;
	uint pcie_gen;
	
	// Capability flags
	bool is_fp64_capable;
	bool is_fp16_capable;
	bool uses_ram;
	
	BenchmarkResults() : 
		compute_units(0), clock_frequency(0), memory_mb(0), tflops_theoretical(0.0f),
		fp64_tflops(-1.0f), fp32_tflops(-1.0f), fp16_tflops(-1.0f),
		int64_tiops(-1.0f), int32_tiops(-1.0f), int16_tiops(-1.0f), int8_tiops(-1.0f),
		mem_bw_coalesced_read(0.0f), mem_bw_coalesced_write(0.0f),
		mem_bw_misaligned_read(0.0f), mem_bw_misaligned_write(0.0f),
		pcie_bw_send(0.0f), pcie_bw_receive(0.0f), pcie_bw_bidirectional(0.0f), pcie_gen(0),
		is_fp64_capable(false), is_fp16_capable(false), uses_ram(false) {}
	
	string to_json() const {
		string json = "{\n";
		json += "  \"device\": {\n";
		json += "    \"name\": \"" + device_name + "\",\n";
		json += "    \"vendor\": \"" + device_vendor + "\",\n";
		json += "    \"driver_version\": \"" + driver_version + "\",\n";
		json += "    \"opencl_version\": \"" + opencl_version + "\",\n";
		json += "    \"compute_units\": " + to_string(compute_units) + ",\n";
		json += "    \"clock_frequency_mhz\": " + to_string(clock_frequency) + ",\n";
		json += "    \"memory_mb\": " + to_string(memory_mb) + ",\n";
		json += "    \"theoretical_tflops\": " + to_string(tflops_theoretical, 3u) + ",\n";
		json += "    \"is_fp64_capable\": " + string(is_fp64_capable ? "true" : "false") + ",\n";
		json += "    \"is_fp16_capable\": " + string(is_fp16_capable ? "true" : "false") + ",\n";
		json += "    \"uses_ram\": " + string(uses_ram ? "true" : "false") + "\n";
		json += "  },\n";
		
		json += "  \"compute_performance\": {\n";
		if(fp64_tflops >= 0.0f) json += "    \"fp64_tflops\": " + to_string(fp64_tflops, 3u) + ",\n";
		json += "    \"fp32_tflops\": " + to_string(fp32_tflops, 3u) + ",\n";
		if(fp16_tflops >= 0.0f) json += "    \"fp16_tflops\": " + to_string(fp16_tflops, 3u) + ",\n";
		json += "    \"int64_tiops\": " + to_string(int64_tiops, 3u) + ",\n";
		json += "    \"int32_tiops\": " + to_string(int32_tiops, 3u) + ",\n";
		json += "    \"int16_tiops\": " + to_string(int16_tiops, 3u) + ",\n";
		json += "    \"int8_tiops\": " + to_string(int8_tiops, 3u) + "\n";
		json += "  },\n";
		
		json += "  \"memory_bandwidth_gb_s\": {\n";
		json += "    \"coalesced_read\": " + to_string(mem_bw_coalesced_read, 2u) + ",\n";
		json += "    \"coalesced_write\": " + to_string(mem_bw_coalesced_write, 2u) + ",\n";
		json += "    \"misaligned_read\": " + to_string(mem_bw_misaligned_read, 2u) + ",\n";
		json += "    \"misaligned_write\": " + to_string(mem_bw_misaligned_write, 2u) + "\n";
		json += "  }";
		
		if(!uses_ram && pcie_bw_send > 0.0f) {
			json += ",\n  \"pcie_bandwidth_gb_s\": {\n";
			json += "    \"send\": " + to_string(pcie_bw_send, 2u) + ",\n";
			json += "    \"receive\": " + to_string(pcie_bw_receive, 2u) + ",\n";
			json += "    \"bidirectional\": " + to_string(pcie_bw_bidirectional, 2u) + ",\n";
			json += "    \"generation\": " + to_string(pcie_gen) + "\n";
			json += "  }";
		}
		
		json += "\n}\n";
		return json;
	}
	
	bool save_to_file(const string& filename) const {
		std::ofstream file(filename);
		if(!file.is_open()) return false;
		file << to_json();
		file.close();
		return true;
	}
};
