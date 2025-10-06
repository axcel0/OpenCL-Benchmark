#include "opencl.hpp" // includes "utilities.hpp"
#include "json_export.hpp"
#include "menu.hpp"

string fraction(const float x) {
	float values[]   = { 1.0f/64.0f, 1.0f/32.0f, 1.0f/24.0f, 1.0f/16.0f, 1.0f/12.0f, 1.0f/8.0f, 1.0f/4.0f, 1.0f/3.0f, 1.0f/2.0f, 2.0f/3.0f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f };
	string strings[] = {   "1/64"  ,   "1/32"  ,   "1/24"  ,   "1/16"  ,   "1/12"  ,   "1/8 ",   "1/4 ", "1/3 ", "1/2 ", "2/3 ", " 1x ", " 2x ", " 4x ", " 8x ", " 16x", " 32x", " 64x" };
	uint i=0u, imin=0u;
	float vmin = max_float;
	for(float v : values) {
		const float vnew = sq(0.01f*x-v);
		if(vnew<=vmin) {
			vmin = vnew;
			imin = i;
		}
		i++;
	}
	return "("+strings[imin]+")";
}

// Helper function to color performance ratios
inline string color_performance(const string& fraction_str) {
	if(contains(fraction_str, "1x") || contains(fraction_str, "2x") || contains(fraction_str, "4x")) {
		return string(ANSI_BRIGHT_GREEN) + fraction_str + ANSI_RESET; // Excellent
	} else if(contains(fraction_str, "1/2") || contains(fraction_str, "2/3")) {
		return string(ANSI_GREEN) + fraction_str + ANSI_RESET; // Good
	} else if(contains(fraction_str, "1/4") || contains(fraction_str, "1/3")) {
		return string(ANSI_YELLOW) + fraction_str + ANSI_RESET; // Moderate
	} else {
		return string(ANSI_RED) + fraction_str + ANSI_RESET; // Low
	}
}

// Helper function to run benchmark iterations with progress indicator
template<typename KernelType>
inline double run_benchmark_iterations(KernelType& kernel, const uint N_iterations, const string& label) {
	double time_min = max_double;
	Clock clock;
	for(uint i=0u; i<N_iterations; i++) {
		const uint progress = (i*100u)/N_iterations;
		print("\r| "+string(ANSI_CYAN)+"Benchmarking "+label+" ... "+to_string(progress)+"%"+string(ANSI_RESET)+"                                    |");
		clock.start();
		kernel.run();
		time_min = fmin(clock.stop(), time_min);
	}
	return time_min;
}

BenchmarkResults benchmark_device(const Device_Info& device_info) {
	BenchmarkResults results;
	const uint N = 4096u*4096u; // kernel range: N*M*sizeof(float) = 1GB memory allocation
	const uint M = 16u; // coalescence size
	const uint N_kernel = 256u; // iterations for kernel calls
	const uint N_memory = 16u; // iterations for PCIe memory transfers

	double time_double=max_double, time_float=max_double, time_half=max_double;
	double time_long=max_double, time_int=max_double, time_short=max_double, time_char=max_double;
	double time_cw=max_double, time_cr=max_double, time_mw=max_double, time_mr=max_double, time_send=max_double, time_receive=max_double, time_bidirectional=max_double;
	Clock clock;

	const string defines =
		"\n	#define def_N "+to_string(N)+"u"
		"\n	#define def_M "+to_string(M)+"u"
	;
	print("| Compiling ...                                                               |");
	Device device(device_info, defines+get_opencl_c_code());
	Memory<float> buffer(device, N, M);
	
	// Populate device info
	results.device_name = device.info.name;
	results.device_vendor = device.info.vendor;
	results.driver_version = device.info.driver_version;
	results.opencl_version = device.info.opencl_c_version;
	results.compute_units = device.info.compute_units;
	results.clock_frequency = device.info.clock_frequency;
	results.memory_mb = device.info.memory;
	results.tflops_theoretical = device.info.tflops;
	results.is_fp64_capable = device.info.is_fp64_capable;
	results.is_fp16_capable = device.info.is_fp16_capable;
	results.uses_ram = device.info.uses_ram;

	if(device.info.is_fp64_capable) {
		Kernel kernel_double(device, N, "kernel_double", buffer);
		time_double = run_benchmark_iterations(kernel_double, N_kernel, "FP64 compute");
		const float flops_double = 512.0f*(float)N/(float)time_double*1E-12f;
		results.fp64_tflops = flops_double;
		const string perf_str = fraction(100.0f*flops_double/device.info.tflops);
		println("\r| "+string(ANSI_BOLD)+"FP64  compute"+string(ANSI_RESET)+" "+alignr(45u, to_string(flops_double, 3u))+" TFLOPs/s "+color_performance(perf_str)+" |");
	} else {
		println("\r| FP64  compute                                          "+string(ANSI_RED)+"not supported"+string(ANSI_RESET)+"        |");
	}

	Kernel kernel_float(device, N, "kernel_float", buffer);
	time_float = run_benchmark_iterations(kernel_float, N_kernel, "FP32 compute");
	const float flops_float = 2048.0f*(float)N/(float)time_float*1E-12f;
	results.fp32_tflops = flops_float;
	const string perf_str_f = fraction(100.0f*flops_float/device.info.tflops);
	println("\r| "+string(ANSI_BOLD)+"FP32  compute"+string(ANSI_RESET)+" "+alignr(45u, to_string(flops_float, 3u))+" TFLOPs/s "+color_performance(perf_str_f)+" |");

	if(device.info.is_fp16_capable) {
		Kernel kernel_half(device, N, "kernel_half", buffer);
		time_half = run_benchmark_iterations(kernel_half, N_kernel, "FP16 compute");
		const float flops_half = 4096.0f*(float)N/(float)time_half*1E-12f;
		results.fp16_tflops = flops_half;
		const string perf_str_h = fraction(100.0f*flops_half/device.info.tflops);
		println("\r| "+string(ANSI_BOLD)+"FP16  compute"+string(ANSI_RESET)+" "+alignr(45u, to_string(flops_half, 3u))+" TFLOPs/s "+color_performance(perf_str_h)+" |");
	} else {
		println("\r| FP16  compute                                          "+string(ANSI_RED)+"not supported"+string(ANSI_RESET)+"        |");
	}

	Kernel kernel_long(device, N, "kernel_long", buffer);
	time_long = run_benchmark_iterations(kernel_long, N_kernel, "INT64 compute");
	const float flops_long = 32.0f*(float)N/(float)time_long*1E-12f;
	results.int64_tiops = flops_long;
	const string perf_str_l = fraction(100.0f*flops_long/device.info.tflops);
	println("\r| "+string(ANSI_BOLD)+"INT64 compute"+string(ANSI_RESET)+" "+alignr(45u, to_string(flops_long, 3u))+"  TIOPs/s "+color_performance(perf_str_l)+" |");

	Kernel kernel_int(device, N, "kernel_int", buffer);
	time_int = run_benchmark_iterations(kernel_int, N_kernel, "INT32 compute");
	const float flops_int = 2048.0f*(float)N/(float)time_int*1E-12f;
	results.int32_tiops = flops_int;
	const string perf_str_i = fraction(100.0f*flops_int/device.info.tflops);
	println("\r| "+string(ANSI_BOLD)+"INT32 compute"+string(ANSI_RESET)+" "+alignr(45u, to_string(flops_int, 3u))+"  TIOPs/s "+color_performance(perf_str_i)+" |");

	Kernel kernel_short(device, N, "kernel_short", buffer);
	time_short = run_benchmark_iterations(kernel_short, N_kernel, "INT16 compute");
	const float flops_short = 1024.0f*(float)N/(float)time_short*1E-12f;
	results.int16_tiops = flops_short;
	const string perf_str_s = fraction(100.0f*flops_short/device.info.tflops);
	println("\r| "+string(ANSI_BOLD)+"INT16 compute"+string(ANSI_RESET)+" "+alignr(45u, to_string(flops_short, 3u))+"  TIOPs/s "+color_performance(perf_str_s)+" |");

	Kernel kernel_char(device, N, "kernel_char", buffer);
	time_char = run_benchmark_iterations(kernel_char, N_kernel, "INT8  compute");
	const float flops_char = 1024.0f*(float)N/(float)time_char*1E-12f;
	results.int8_tiops = flops_char;
	const string perf_str_c = fraction(100.0f*flops_char/device.info.tflops);
	println("\r| "+string(ANSI_BOLD)+"INT8  compute"+string(ANSI_RESET)+" "+alignr(45u, to_string(flops_char, 3u))+"  TIOPs/s "+color_performance(perf_str_c)+" |");

	Kernel kernel_coalesced_write(device, N, "kernel_coalesced_write" , buffer);
	time_cw = run_benchmark_iterations(kernel_coalesced_write, N_kernel, "coalesced write");
	Kernel kernel_coalesced_read(device, N, "kernel_coalesced_read"  , buffer);
	time_cr = run_benchmark_iterations(kernel_coalesced_read, N_kernel, "coalesced read");
	results.mem_bw_coalesced_read = 4.0f*(float)N*(float)M/(float)(time_cr-time_cw/(double)M)*1E-9f;
	results.mem_bw_coalesced_write = 4.0f*(float)N*(float)M/(float) time_cw *1E-9f;
	println("\r| Memory Bandwidth ( coalesced read      ) "+alignr(29u, to_string(results.mem_bw_coalesced_read, 2u))+" GB/s |");
	println("\r| Memory Bandwidth ( coalesced      write) "+alignr(29u, to_string(results.mem_bw_coalesced_write, 2u))+" GB/s |");

	Kernel kernel_misaligned_write(device, N, "kernel_misaligned_write", buffer);
	time_mw = run_benchmark_iterations(kernel_misaligned_write, N_kernel, "misaligned write");
	Kernel kernel_misaligned_read(device, N, "kernel_misaligned_read" , buffer);
	time_mr = run_benchmark_iterations(kernel_misaligned_read, N_kernel, "misaligned read");
	results.mem_bw_misaligned_read = 4.0f*(float)N*(float)M/(float)(time_mr-time_cw/(double)M)*1E-9f;
	results.mem_bw_misaligned_write = 4.0f*(float)N*(float)M/(float) time_mw *1E-9f;
	println("\r| Memory Bandwidth (misaligned read      ) "+alignr(29u, to_string(results.mem_bw_misaligned_read, 2u))+" GB/s |");
	println("\r| Memory Bandwidth (misaligned      write) "+alignr(29u, to_string(results.mem_bw_misaligned_write, 2u))+" GB/s |");

	if(!device.info.uses_ram) {
		print("| Benchmarking ...                                                            |");
		for(uint i=0u; i<N_memory; i++) {
			clock.start();
			buffer.write_to_device();
			time_send = fmin(clock.stop(), time_send);
		}
		const float bw_send = 4.0f*M*N/(float)time_send*1E-9f;
		results.pcie_bw_send = bw_send;
		println("\r| PCIe   Bandwidth (send                 ) "+alignr(29u, to_string(bw_send, 2u))+" GB/s |");
		print("| Benchmarking ...                                                            |");
		for(uint i=0u; i<N_memory; i++) {
			clock.start();
			buffer.read_from_device();
			time_receive = fmin(clock.stop(), time_receive);
		}
		const float bw_receive = 4.0f*M*N/(float)time_receive*1E-9f;
		results.pcie_bw_receive = bw_receive;
		println("\r| PCIe   Bandwidth (   receive           ) "+alignr(29u, to_string(bw_receive, 2u))+" GB/s |");
		print("| Benchmarking ...                                                            |");
		for(uint i=0u; i<N_memory; i++) {
			clock.start();
			buffer.read_from_device(N*M/2u, N*M, false);
			buffer.write_to_device(0u, N*M/2u, false);
			buffer.finish_queue();
			time_bidirectional = fmin(clock.stop(), time_bidirectional);
		}
		const float bw_bidirectional = 4.0f*M*N/(float)time_bidirectional*1E-9f;
		const float bw_max = fmax(2.0f*fmax(bw_send, bw_receive), bw_bidirectional);
		results.pcie_bw_bidirectional = bw_bidirectional;
		results.pcie_gen = bw_max>17.6f?4:bw_max>8.8f?3:bw_max>4.4f?2:1;
		println("\r| PCIe   Bandwidth (        bidirectional)            (Gen"+to_string(results.pcie_gen)+" x16)"+alignr(8u, to_string(bw_bidirectional, 2u))+" GB/s |");
	}

	println("|-----------------------------------------------------------------------------|");
	
	// Auto-save results to JSON file
	const string filename = "benchmark_results_"+replace(device.info.name, " ", "_")+".json";
	if(results.save_to_file(filename)) {
		println("| "+string(ANSI_GREEN)+"Results saved to: "+filename+string(ANSI_RESET)+string(77u-16u-length(filename), ' ')+"|");
	}
	
	return results;
}

int main(int argc, char* argv[]) {
	vector<string> main_arguments = get_main_arguments(argc, argv);
	
	// Check if command line arguments were provided (legacy mode)
	if((int)main_arguments.size() > 0) {
		// Legacy mode: run benchmark directly with specified devices
		println(".-----------------------------------------------------------------------------.");
		const vector<Device_Info> devices = get_devices();
		vector<BenchmarkResults> all_results;
		
		for(uint i=0u; i<(uint)main_arguments.size(); i++) {
			all_results.push_back(benchmark_device(select_device_with_id(to_int(main_arguments[i]), devices)));
		}
		
		#ifdef _WIN32
		println("|-----------------------------------------------------------------------------|");
		println("| "+string(ANSI_BRIGHT_CYAN)+"Done. Press Enter to exit."+string(ANSI_RESET)+"                                                  |");
		println("'-----------------------------------------------------------------------------'");
		wait();
		#else
		println("'-----------------------------------------------------------------------------'");
		#endif
		
		return 0;
	}
	
	// Interactive menu mode
	while(true) {
		const vector<Device_Info> devices = get_devices();
		
		vector<string> menu_options;
		menu_options.push_back("[1] Run Full Benchmark (All Devices)");
		menu_options.push_back("[2] Select Specific Device");
		menu_options.push_back("[3] Quick Test (FP32 + Memory Only)");
		menu_options.push_back("[4] View System Information");
		menu_options.push_back("[5] About OpenCL-Benchmark");
		menu_options.push_back("[0] Exit");
		
		int choice = show_menu("OpenCL Benchmark Suite v2.0 (Optimized)", menu_options);
		
		if(choice == -1 || choice == 5) {
			println("\n"+string(ANSI_BRIGHT_CYAN)+"Thank you for using OpenCL-Benchmark!"+string(ANSI_RESET)+"\n");
			break;
		}
		
		switch(choice) {
			case 0: // Run Full Benchmark (All Devices)
			{
				println(".-----------------------------------------------------------------------------.");
				vector<BenchmarkResults> all_results;
				for(uint i=0u; i<(uint)devices.size(); i++) {
					all_results.push_back(benchmark_device(devices[i]));
				}
				println("'-----------------------------------------------------------------------------'");
				println("\n"+string(ANSI_GREEN)+"Press Enter to continue..."+string(ANSI_RESET));
				wait();
				break;
			}
			
			case 1: // Select Specific Device
			{
				vector<string> device_menu;
				for(uint i=0u; i<(uint)devices.size(); i++) {
					device_menu.push_back("["+to_string(i)+"] "+devices[i].name);
				}
				device_menu.push_back("< Back");
				
				int device_choice = show_menu("Select Device to Benchmark", device_menu);
				
				if(device_choice >= 0 && device_choice < (int)devices.size()) {
					println(".-----------------------------------------------------------------------------.");
					benchmark_device(devices[device_choice]);
					println("'-----------------------------------------------------------------------------'");
					println("\n"+string(ANSI_GREEN)+"Press Enter to continue..."+string(ANSI_RESET));
					wait();
				}
				break;
			}
			
			case 2: // Quick Test
			{
				println("\n"+string(ANSI_YELLOW)+"Quick Test not yet implemented."+string(ANSI_RESET));
				println(string(ANSI_GREEN)+"Press Enter to continue..."+string(ANSI_RESET));
				wait();
				break;
			}
			
			case 3: // View System Information
			{
				#ifdef _WIN32
				system("cls");
				#else
				system("clear");
				#endif
				
				println(string(ANSI_BRIGHT_CYAN));
				println("┌" + string(78, '─') + "┐");
				println("│" + alignr(79u, "System Information") + "│");
				println("├" + string(78, '─') + "┤");
				println(string(ANSI_RESET));
				
				for(uint i=0u; i<(uint)devices.size(); i++) {
					print_device_info(devices[i]);
				}
				
				println(string(ANSI_BRIGHT_CYAN));
				println("└" + string(78, '─') + "┘");
				println(string(ANSI_RESET));
				
				println("\n"+string(ANSI_GREEN)+"Press Enter to continue..."+string(ANSI_RESET));
				wait();
				break;
			}
			
			case 4: // About
			{
				#ifdef _WIN32
				system("cls");
				#else
				system("clear");
				#endif
				
				println(string(ANSI_BRIGHT_CYAN));
				println("┌" + string(78, '─') + "┐");
				println("│" + alignr(79u, "About OpenCL-Benchmark v2.0") + "│");
				println("├" + string(78, '─') + "┤");
				println(string(ANSI_RESET));
				println("│                                                                              │");
				println("│  " + string(ANSI_BOLD) + "OpenCL-Benchmark v2.0 (Optimized Edition)" + string(ANSI_RESET) + "                            │");
				println("│                                                                              │");
				println("│  A comprehensive OpenCL benchmarking tool for GPU/CPU performance testing.  │");
				println("│                                                                              │");
				println("│  " + string(ANSI_YELLOW) + "Features:" + string(ANSI_RESET) + "                                                                   │");
				println("│    ✓ FP64, FP32, FP16 compute performance                                   │");
				println("│    ✓ INT64, INT32, INT16, INT8 compute performance                          │");
				println("│    ✓ Memory bandwidth (coalesced & misaligned)                              │");
				println("│    ✓ PCIe bandwidth testing                                                 │");
				println("│    ✓ JSON result export                                                     │");
				println("│    ✓ Colored output & progress indicators                                   │");
				println("│    ✓ Interactive menu system                                                │");
				println("│                                                                              │");
				println("│  " + string(ANSI_GREEN) + "GitHub:" + string(ANSI_RESET) + " https://github.com/ProjectPhysX/OpenCL-Benchmark              │");
				println("│  " + string(ANSI_CYAN) + "Optimized by:" + string(ANSI_RESET) + " AI Assistant (October 2025)                             │");
				println("│                                                                              │");
				println(string(ANSI_BRIGHT_CYAN));
				println("└" + string(78, '─') + "┘");
				println(string(ANSI_RESET));
				
				println("\n"+string(ANSI_GREEN)+"Press Enter to continue..."+string(ANSI_RESET));
				wait();
				break;
			}
		}
	}
	
	return 0;
}
