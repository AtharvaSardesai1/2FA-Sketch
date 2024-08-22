#ifndef _DATASET_PARAM_H
#define _DATASET_PARAM_H

#define CAIDA

#ifdef CAIDA
    #define MEMORY_NUMBER 16
#else 
    #define MEMORY_NUMBER 16
	#define ZIPF_ALPHA 1.2
#endif

#define START_FILE_NO 1
#define END_FILE_NO 10

#endif // DATASET_PARAM_H