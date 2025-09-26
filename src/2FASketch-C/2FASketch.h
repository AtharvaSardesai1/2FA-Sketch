#ifndef SKETCH_H
#define SKETCH_H

#include <stdint.h>
#include "heavy_part.h"

typedef struct {
    Elastic_2FA_HeavyPart* heavy_part;
    int thres_set;
} Elastic_2FASketch;

Elastic_2FASketch* Elastic_2FASketch_create(int bucket_num, int thres_set);
void Elastic_2FASketch_destroy(Elastic_2FASketch* self);
void Elastic_2FASketch_clear(Elastic_2FASketch* self);
void Elastic_2FASketch_insert(Elastic_2FASketch* self, uint32_t flow_id, int f);
uint32_t Elastic_2FASketch_query(Elastic_2FASketch* self, uint32_t flow_id);
void Elastic_2FASketch_get_heavy_hitters(Elastic_2FASketch* self, int threshold,
                                         uint32_t* keys, uint32_t* vals, int* num);

#endif
