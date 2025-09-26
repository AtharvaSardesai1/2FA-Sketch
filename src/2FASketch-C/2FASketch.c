#include "2FASketch.h"
#include <stdlib.h>
#include <string.h>

Elastic_2FASketch* Elastic_2FASketch_create(int bucket_num, int thres_set) {
    Elastic_2FASketch* self = malloc(sizeof(*self));
    if (!self) return NULL;
    self->heavy_part = Elastic_2FA_HeavyPart_create(bucket_num);
    if (!self->heavy_part) { free(self); return NULL; }
    self->thres_set = thres_set;
    return self;
}

void Elastic_2FASketch_destroy(Elastic_2FASketch* self) {
    if (!self) return;
    Elastic_2FA_HeavyPart_destroy(self->heavy_part);
    free(self);
}

void Elastic_2FASketch_clear(Elastic_2FASketch* self) {
    if (self && self->heavy_part) Elastic_2FA_HeavyPart_clear(self->heavy_part);
}

void Elastic_2FASketch_insert(Elastic_2FASketch* self, uint32_t flow_id, int f) {
    int res = Elastic_2FA_HeavyPart_insert(self->heavy_part, flow_id, f, self->thres_set);
    if (res == self->thres_set) {
        Elastic_2FA_HeavyPart_insert(self->heavy_part, flow_id, f, 0);
    }
}

uint32_t Elastic_2FASketch_query(Elastic_2FASketch* self, uint32_t flow_id) {
    return Elastic_2FA_HeavyPart_query(self->heavy_part, flow_id, self->thres_set);
}

void Elastic_2FASketch_get_heavy_hitters(Elastic_2FASketch* self, int threshold,
                                         uint32_t* keys, uint32_t* vals, int* num) {
    Elastic_2FA_HeavyPart_get_heavy_hitters(self->heavy_part, threshold, keys, vals, num);
}
