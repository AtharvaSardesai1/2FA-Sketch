// 2FASketch_main.c
#include "2FASketch.h"
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>   // clock_gettime

#define MAX_FILENAME 256
#define MAX_PACKETS 10000000
#define MEMORY_NUMBER 500  // KB
#define TOT_MEM_IN_BYTES (MEMORY_NUMBER * 1024)
#define BUCKET_NUM (TOT_MEM_IN_BYTES / 64)
#define START_FILE_NO 1
#define END_FILE_NO 10

// ---------------- Hash map (ground truth) ----------------
typedef struct {
    uint32_t key;
    uint32_t val;
    char used;
} HTEntry;

typedef struct {
    HTEntry *table;
    size_t capacity;
    size_t size;
} HashMap;

static size_t next_pow2(size_t x) { size_t p = 1; while (p < x) p <<= 1; return p; }

HashMap* hm_create(size_t initial_capacity) {
    HashMap *hm = malloc(sizeof(HashMap));
    if (!hm) return NULL;
    size_t cap = next_pow2(initial_capacity > 8 ? initial_capacity : 8);
    hm->capacity = cap;
    hm->size = 0;
    hm->table = calloc(cap, sizeof(HTEntry));
    if (!hm->table) { free(hm); return NULL; }
    return hm;
}

void hm_free(HashMap *hm) { if (!hm) return; free(hm->table); free(hm); }

static void hm_resize(HashMap *hm, size_t new_capacity) {
    new_capacity = next_pow2(new_capacity);
    HTEntry *old = hm->table;
    size_t oldcap = hm->capacity;
    HTEntry *table = calloc(new_capacity, sizeof(HTEntry));
    for (size_t i = 0; i < oldcap; ++i) {
        if (old[i].used) {
            uint32_t k = old[i].key, v = old[i].val;
            size_t mask = new_capacity - 1;
            size_t idx = ((uint64_t)k * 11400714819323198485ull) & mask;
            while (table[idx].used) idx = (idx + 1) & mask;
            table[idx].used = 1; table[idx].key = k; table[idx].val = v;
        }
    }
    free(old);
    hm->table = table;
    hm->capacity = new_capacity;
}

void hm_inc(HashMap *hm, uint32_t key) {
    if (hm->size * 2 >= hm->capacity) hm_resize(hm, hm->capacity * 2);
    size_t mask = hm->capacity - 1;
    size_t idx = ((uint64_t)key * 11400714819323198485ull) & mask;
    while (hm->table[idx].used) {
        if (hm->table[idx].key == key) { hm->table[idx].val += 1; return; }
        idx = (idx + 1) & mask;
    }
    hm->table[idx].used = 1; hm->table[idx].key = key; hm->table[idx].val = 1; hm->size++;
}

uint32_t hm_get(HashMap *hm, uint32_t key) {
    if (!hm) return 0;
    size_t mask = hm->capacity - 1;
    size_t idx = ((uint64_t)key * 11400714819323198485ull) & mask;
    while (hm->table[idx].used) {
        if (hm->table[idx].key == key) return hm->table[idx].val;
        idx = (idx + 1) & mask;
    }
    return 0;
}

void hm_iterate(HashMap *hm, void (*fn)(uint32_t key, uint32_t val, void *ud), void *ud) {
    if (!hm) return;
    for (size_t i = 0; i < hm->capacity; ++i) if (hm->table[i].used) fn(hm->table[i].key, hm->table[i].val, ud);
}

// ---------------- Trace & GT ----------------
typedef struct { uint8_t key[13]; } FIVE_TUPLE;
FIVE_TUPLE* traces[END_FILE_NO + 1];
int packet_cnts[END_FILE_NO + 1];
HashMap* gt_maps[END_FILE_NO + 1];

uint32_t key_to_uint32_be(const uint8_t *key) {
    return ((uint32_t)key[0] << 24) |
           ((uint32_t)key[1] << 16) |
           ((uint32_t)key[2] << 8)  |
           ((uint32_t)key[3]);
}

void read_traces(const char* prefix) {
    char filename[MAX_FILENAME];
    for (int i = START_FILE_NO; i <= END_FILE_NO; i++) {
        snprintf(filename, sizeof(filename), "%s/%d.dat", prefix, i - 1);
        printf("processing caida dataset...\n");

        FILE* fin = fopen(filename, "rb");
        if (!fin) { perror("open"); traces[i]=NULL; gt_maps[i]=NULL; continue; }

        fseek(fin, 0, SEEK_END);
        long sz = ftell(fin);
        fseek(fin, 0, SEEK_SET);

        int num = (int)(sz / sizeof(FIVE_TUPLE));
        if (num > MAX_PACKETS) num = MAX_PACKETS;
        FIVE_TUPLE* packets = malloc((size_t)num * sizeof(FIVE_TUPLE));
        fread(packets, sizeof(FIVE_TUPLE), num, fin);
        fclose(fin);

        traces[i]=packets; packet_cnts[i]=num;
        HashMap* gt = hm_create(1<<17);
        for (int j=0;j<num;j++) hm_inc(gt,key_to_uint32_be(packets[j].key));
        gt_maps[i]=gt;
        printf("Successfully read in %s, %d packets\n", filename,num);
    }
}

void free_traces() {
    for (int i=START_FILE_NO;i<=END_FILE_NO;i++) {
        free(traces[i]); traces[i]=NULL;
        hm_free(gt_maps[i]); gt_maps[i]=NULL;
    }
}

// ---------------- GT helper ----------------
typedef struct { int threshold; size_t above_count; } GTCountCtx;
static void count_gt_if_above(uint32_t k, uint32_t v, void* ud){
    GTCountCtx* c=(GTCountCtx*)ud; if ((int)v>=c->threshold) c->above_count++;
}

// ---------------- Main ----------------
int main(int argc,char*argv[]){
    if(argc<2){printf("Usage: %s <trace_dir>\n",argv[0]);return 1;}
    read_traces(argv[1]);

    printf("\nMeasurement by Algorithm 2FASketch Starts, memory: %dKB\n", MEMORY_NUMBER);

    double totP=0, totR=0, totF=0, totARE=0, totAAE=0, totThr=0;
    int files=0;

    for(int idx=START_FILE_NO;idx<=END_FILE_NO;idx++){
        if(!traces[idx]||!gt_maps[idx])continue;
        int n=packet_cnts[idx];
        size_t flows=gt_maps[idx]->size;
        printf("\nThere are %zu flows\n",flows);

        int threshold=(int)((double)n/10000.0);
        if(threshold<1)threshold=1;

        Elastic_2FASketch* sketch=Elastic_2FASketch_create(BUCKET_NUM,threshold/2);
        // warmup
        int warmup=n<10000?n:10000;
        for(int i=0;i<warmup;i++){
            uint32_t k=key_to_uint32_be(traces[idx][i].key);
            uint8_t k4[4]={k>>24,k>>16,k>>8,k};
            Elastic_2FASketch_insert(sketch,k4,1);
        }
        Elastic_2FASketch_clear(sketch);

        // measure time
        struct timespec t0,t1;
        clock_gettime(CLOCK_MONOTONIC,&t0);
        for(int i=0;i<n;i++){
            uint32_t k=key_to_uint32_be(traces[idx][i].key);
            uint8_t k4[4]={k>>24,k>>16,k>>8,k};
            Elastic_2FASketch_insert(sketch,k4,1);
        }
        clock_gettime(CLOCK_MONOTONIC,&t1);
        double elapsed=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)/1e9;
        double throughput=(n/elapsed)/1e6;
        totThr+=throughput;

        double cnt_ratio=Elastic_2FA_HeavyPart_get_cnt_ratio(sketch->heavy_part);
        int cnt_all=sketch->heavy_part->cnt_all;
        printf("2FA Sketch cnt ratio: %.6f, %d\n",cnt_ratio,cnt_all);

        int maxhh=sketch->heavy_part->bucket_num*COUNTER_PER_BUCKET;
        uint32_t*hh_keys=malloc(sizeof(uint32_t)*maxhh);
        uint32_t*hh_vals=malloc(sizeof(uint32_t)*maxhh);
        int num_hitters=maxhh;
        Elastic_2FASketch_get_heavy_hitters(sketch,threshold,hh_keys,hh_vals,&num_hitters);

        printf("heavy hitters: <srcIP, count>, threshold=%d, number=%d\n",threshold,num_hitters);

        GTCountCtx ctx={threshold,0}; hm_iterate(gt_maps[idx],count_gt_if_above,&ctx);
        size_t gt_heavy_n=ctx.above_count;

        size_t TP=0,FP=0; double sumAAE=0,sumARE=0; size_t counted=0;
        for(int i=0;i<num_hitters;i++){
            uint32_t k=hh_keys[i]; uint32_t est=hh_vals[i];
            uint32_t real=hm_get(gt_maps[idx],k);
            if(real>= (uint32_t)threshold){
                TP++; double err=fabs((double)est-real);
                sumAAE+=err; if(real>0) sumARE+=err/(double)real; counted++;
            }else FP++;
        }

        double P=(TP+FP)?(double)TP/(TP+FP):1.0;
        double R=(gt_heavy_n>0)?(double)TP/(double)gt_heavy_n:1.0;
        double F=(P+R>0)?2*P*R/(P+R):0.0;
        double AAE=counted?(sumAAE/counted):0.0;
        double ARE=counted?(sumARE/counted):0.0;

        printf("precision_rate=%.6f\n",P);
        printf("recall_rate=%.6f\n",R);
        printf("F_score=%.6f\n",F);
        printf("ARE=%.6f\n",ARE);
        printf("AAE=%.6f\n",AAE);
        printf("throughput=%.2f Mpps\n",throughput);

        totP+=P; totR+=R; totF+=F; totARE+=ARE; totAAE+=AAE; files++;

        free(hh_keys); free(hh_vals);
        Elastic_2FASketch_destroy(sketch);
    }

    if(files>0){
        printf("\nfive average results below can be ignored if calculating CDF\n");
        printf("average precision rate=%.6f\n",totP/files);
        printf("average recallrate=%.6f\n",totR/files);
        printf("average F1 score=%.6f\n",totF/files);
        printf("average ARE=%.6f\n",totARE/files);
        printf("average AAE=%.6f\n",totAAE/files);
        printf("average throughput=%.2f Mpps\n",totThr/files);
    }
    free_traces();
    return 0;
}
