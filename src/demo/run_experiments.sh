#!/bin/bash

# Create results directory if it doesn't exist
mkdir -p ../../data/results

# Memory sizes to test (in KB)
MEM_SIZES=(100 200 300 400 500)

# Algorithms to test
ALGORITHMS=(
    "1FA"
    "2FASketch" 
    "chainsketch"
    "cmheap"
    "countheap"
    "elastic"
    "spacesaving"
)

# Output file for summary
SUMMARY_FILE="../../data/results/summary_metrics.csv"

# Write CSV header if file doesn't exist
if [ ! -f "$SUMMARY_FILE" ]; then
    echo "algorithm,memory_kb,avg_precision,avg_recall,avg_f1,avg_are,avg_aae" > "$SUMMARY_FILE"
fi

# Function to extract metrics from output
extract_metrics() {
    local output_file=$1
    
    if [ ! -f "$output_file" ]; then
        echo "0,0,0,0,0"
        return 1
    fi
    
    # Initialize variables with defaults
    local precision="0"
    local recall="0" 
    local f1="0"
    local are="0"
    local aae="0"
    
    # Read the file and extract metrics using grep and cut
    if grep -q "average precision rate=" "$output_file"; then
        precision=$(grep "average precision rate=" "$output_file" | tail -1 | cut -d'=' -f2)
    fi
    
    if grep -q "average recallrate=" "$output_file"; then
        recall=$(grep "average recallrate=" "$output_file" | tail -1 | cut -d'=' -f2)
    fi
    
    if grep -q "average F1 score=" "$output_file"; then
        f1=$(grep "average F1 score=" "$output_file" | tail -1 | cut -d'=' -f2)
    fi
    
    if grep -q "average ARE=" "$output_file"; then
        are=$(grep "average ARE=" "$output_file" | tail -1 | cut -d'=' -f2)
    fi
    
    if grep -q "average AAE=" "$output_file"; then
        aae=$(grep "average AAE=" "$output_file" | tail -1 | cut -d'=' -f2)
    fi
    
    # Return the metrics as a comma-separated string
    echo "$precision,$recall,$f1,$are,$aae"
    return 0
}

# Function to run a single experiment
run_experiment() {
    local algo=$1
    local mem=$2
    
    echo "=== Running $algo with ${mem}KB memory ==="
    
    # Run the algorithm and save output to file
    local output_file="../../data/results/${algo}_${mem}KB.txt"
    
    # Run the algorithm and redirect ALL output to the file
    ./${algo}.out > "$output_file" 2>&1
    
    # Wait a moment for the file to be written
    sleep 1
    
    # Also display the output on console for monitoring
    echo "Output saved to $output_file"
    echo "Last few lines:"
    tail -5 "$output_file"
    
    # Extract and save metrics
    if metrics=$(extract_metrics "$output_file"); then
        # Check if we got actual values (not all zeros)
        if [[ "$metrics" != "0,0,0,0,0" ]]; then
            echo "$algo,$mem,$metrics" >> "$SUMMARY_FILE"
            echo "Metrics recorded: $algo,$mem,$metrics"
        else
            echo "Warning: No valid metrics found for $algo with ${mem}KB memory"
            echo "Checking file content:"
            head -20 "$output_file"
            echo "$algo,$mem,0,0,0,0,0" >> "$SUMMARY_FILE"
        fi
    else
        echo "Warning: Failed to extract metrics for $algo with ${mem}KB memory"
        echo "$algo,$mem,0,0,0,0,0" >> "$SUMMARY_FILE"
    fi
}

# Main experiment loop
for mem in "${MEM_SIZES[@]}"; do
    echo -e "\n===== Testing with ${mem}KB memory ====="
    
    # Update memory size in dataset_param.h
    sed -i "s/#define MEMORY_NUMBER [0-9]\+/#define MEMORY_NUMBER $mem/g" dataset_param.h
    
    # Recompile all algorithms
    echo "Recompiling with ${mem}KB memory..."
    make clean
    make -j$(nproc) || { echo "Compilation failed"; exit 1; }
    
    # Run each algorithm
    for algo in "${ALGORITHMS[@]}"; do
        run_experiment "$algo" "$mem"
    done
done

echo -e "\n===== All experiments completed ====="
echo "Results are saved in ../../data/results/"
echo "Summary metrics saved to $SUMMARY_FILE"

# Display final summary
echo -e "\n===== Summary of Results ====="
if [ -f "$SUMMARY_FILE" ]; then
    cat "$SUMMARY_FILE"
fi