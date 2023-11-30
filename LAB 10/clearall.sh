# Helper script to clean up all the generated text files or images before next analysis

#!/bin/bash

rm cpu_util.txt cpu_util.png error_rate.png error_rate.txt goodput_data.png goodput_data.txt nlwp.png nlwp_data.txt process_snapshots.txt request_rate.png request_rate.txt response_time.png response_time_data.txt throughput.png throughput_data.txt timeout_rate.png timeout_rate.txt clientoutput* ./compiler_error/* ./diffs/* ./executables/* ./outputs/* ./runtime_error/* ./submissions/*

rm ./client_files/*.txt
rm ./final_outputs/*.txt
rm ./total_response_time.txt
rm ./avg_response_time.txt
rm ./throughput.txt
rm ./response_time_plot.png
rm ./backup/*.txt
