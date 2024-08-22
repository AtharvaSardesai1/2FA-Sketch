# 2FASketch

## Introduction
Detecting heavy hitters, which are flows exceeding a specified threshold, is crucial for network measurement, but it faces challenges due to increasing throughput and memory constraints. Existing sketch-based solutions, particularly those using Comparative Counter Voting, have limitations in efficiently identifying heavy hitters. This paper introduces the Two-Factor Armor (2FA) Sketch, a novel data structure designed to enhance heavy hitter detection in data streams. 2FA Sketch implements dual-layer protection through an improved <tt>Arbitration</tt> strategy for in-bucket competition and a cross-bucket conflict <tt>Avoidance</tt> hashing scheme. By theoretically deriving an optimal $\lambda$ parameter and redesigning $vote^+_{new}$ as a conflict indicator, it optimizes the Comparative Counter Voting strategy. Experimental results show that 2FA Sketch outperforms the standard Elastic Sketch, reducing error rates by 2.5 to 19.7 times and increasing processing speed by 1.03 times.

## About this repo
- `data`: traces for test, each 13 bytes in a trace is a five tuple: (SrcIP:SrcPort, DstIP:DstPort, protocol)

- `src`: contains codes of 2FASketch and other algorithms implemented on CPU, all of which can be used to detect heavy hitters. They are  ChainSketch, SpaceSaving, Count/CM sketch with a min-heap (CountHeap/CMHeap), Elastic, 1FA and 2FASketch respectively. The codes of measuring their accuracy are also added in these algorithms'  .cpp file.

- `src_for_speed`: algorithms in this folder are the same as those in `src`. The codes in this folder aim at measuring their speed.   

  For all the algorithms, the default memory size is 100KB and can be modified. Besides, codes used to print out the estimated heavy hitters are commented out. If you want to see related results, just modify it.

## Requirements
- SIMD instructions are used in Elastic, 1FA and 2FASketch to achieve higher speed, so the CPU must support AVX2 instruction set.
- g++

## How to make
- `cd ./src/demo; make;` then you can find executable file and test the metrics of accuracy of the above algorithms in `demo`.
- Executable file: `./elastic.out; ./1FA.out; ./2FASketch.out; ./chainsketch.out; ./cmheap.out; ./countheap.out; ./spacesaving.out ` are all followed by  three parameters: the name of output file, algorithms' label name and tested metrics' model name. We use 1~7 to represent the task of measuring ARE, AAE, PR, RR, F1 score, AE's CDF and RE's CDF, respectively.  
- `cd ./src_for_speed/demo; make;` then you can find executable file and test he metrics of speed of  the above algorithms in `demo`. Executable files' names are the same as those in folder `./src/demo`, but only followed by two parameters: the name of output file, algorithms' label name.


