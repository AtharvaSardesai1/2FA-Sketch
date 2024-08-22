#ifndef _CHAINSKETCH_H
#define _CHAINSKETCH_H

#include <math.h>
#include <random>
#include <vector>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "../common/BOBHash32.h"


int MAXINT = 1000000000, chainlength = 2;
unsigned int bucket = 0, bucket1 = 0;
int loc = -1, ii = 0;
static std::random_device rd;

template <int TOT_MEM_IN_BYTES>
class ChainSketch
{

	typedef struct SBUCKET_type
	{
		int count;
		uint32_t key;
	} SBucket;

	struct Chain_type
	{
		// Counter table
		SBucket **counts;
		// Outer sketch depth and width
		int depth;
		int width;
		// # key word bits
		BOBHash32 *hardner;
	};

public:
	ChainSketch()
	{
		Chain_.depth = 4;
		Chain_.width = TOT_MEM_IN_BYTES/4/8;
		Chain_.counts = new SBucket *[TOT_MEM_IN_BYTES/8];
		for (int i = 0; i < TOT_MEM_IN_BYTES/8; i++)
		{
			Chain_.counts[i] = (SBucket *)calloc(1, sizeof(SBucket));
			memset(Chain_.counts[i], 0, sizeof(SBucket));
			// Chain_.counts[i]->key = 0;
		}
		Chain_.hardner = new BOBHash32[Chain_.depth];
		unsigned int seed = rd() % MAX_PRIME32;

		for (int i = 0; i < Chain_.depth; i++)
		{
			Chain_.hardner[i].initialize((seed++) % MAX_PRIME32);
		}
	}

	~ChainSketch()
	{
		for (int i = 0; i < Chain_.depth * Chain_.width; i++)
		{
			free(Chain_.counts[i]);
		}
		delete[] Chain_.hardner;
		delete[] Chain_.counts;
	}

	void insert(uint8_t *key, int val = 1)
	{
		uint32_t min = 99999999, index, fp;
		ChainSketch::SBucket *sbucket;
		ChainSketch::SBucket *sbucket1;
		if (!chainlength)
			return;
		fp = *((uint32_t *)key);
		for (int i = 0; i < Chain_.depth; i++)
		{
			bucket = Chain_.hardner[i].run((const char *)key, 4) % Chain_.width;
			index = i * Chain_.width + bucket;
			sbucket = Chain_.counts[index];
			if (sbucket->count == 0)
			{
				sbucket->key = fp; // memcpy(sbucket->key, key, key_len);
				sbucket->count = val;
				return;
			}
			if (fp == sbucket->key) // if (memcmp(key, sbucket->key, key_len) == 0)
			{
				sbucket->count += val;
				return;
			}
			if (sbucket->count < min)
			{
				min = sbucket->count;
				loc = index;
				bucket1 = bucket;
				ii = i;
			}
		}
		sbucket = Chain_.counts[loc];
		int k = rd() % (sbucket->count + val) + 1;
		if (k <= val && chainlength > 0)
		{
			index = ii * Chain_.width + (bucket1 + 1) % Chain_.width;
			sbucket1 = Chain_.counts[index];
			// if (memcmp(sbucket1->key, sbucket->key, key_len) == 0 && index != loc)
			if (sbucket1->key == sbucket->key && index != loc)
			{
				sbucket1->count += sbucket->count;
			}
			// else if (memcmp(sbucket1->key, key, key_len) == 0 && index != loc)
			else if (sbucket1->key == fp && index != loc)
			{
				int p = sbucket1->count;
				sbucket1->count = sbucket->count;
				sbucket1->key = sbucket->key; // memcpy(sbucket1->key, sbucket->key, key_len);
				sbucket->count += p;
			}
			else if (index != loc)
			{
				double round = 1;
				while (round <= chainlength && sbucket->count > sbucket1->count)
				{
					double va = sbucket->count * 1.0 / (sbucket1->count + sbucket->count);
					double po = pow(va, round);
					int ro = 1;
					while (ro * po < 10 && ro < MAXINT)
					{
						ro *= 10;
					}

					int newk = rd() % ro + 1;
					if (newk <= int(ro * po))
					{
						sbucket1->key = sbucket->key; // memcpy(sbucket1->key, sbucket->key, key_len);
						sbucket1->count = sbucket->count;
						break;
					}
					index = ii * Chain_.width + (index + 1) % Chain_.width;
					sbucket1 = Chain_.counts[index];
					round = round + 1;
					if (sbucket1->key == fp) // if (memcmp(sbucket1->key, key, key_len) == 0)
					{
						int p = sbucket1->count;
						sbucket1->count = 0;
						sbucket->count += p;
					}
				}
			}
			sbucket->key = fp; // memcpy(sbucket->key, key, key_len);
			sbucket->count += val;
		}
	}

	void get_heavy_hitters(int thresh, vector<std::pair<string, int>> &results)
	{
		std::unordered_map<string, int> ground;
		string key;

		for (int i = 0; i < Chain_.width * Chain_.depth; i++)
		{
			key = string((const char *)(&(Chain_.counts[i]->key)), 4); // memcpy(key, Chain_.counts[i]->key, key_len);
			ground[key] += Chain_.counts[i]->count;
		}
		for (auto it = ground.begin(); it != ground.end();)
		{
			if (it->second < (int)thresh)
			{
				it = ground.erase(it);
			}
			else
				it++;
		}

		for (auto it = ground.begin(); it != ground.end(); it++)
		{
			std::pair<string, int> node;
			node.first = it->first; // memcpy(key, it->first.key, key_len);
			node.second = it->second;
			results.push_back(node);
		}

		std::cout << "results.size = " << results.size() << std::endl;
	}

	void reset()
	{

		for (int i = 0; i < Chain_.depth * Chain_.width; i++)
		{
			Chain_.counts[i]->count = 0;
			Chain_.counts[i]->key = 0;
			// memset(Chain_.counts[i]->key, 0, key_len);
		}
	}

private:
	void setBucket(int row, int column, int count, unsigned char *key)
	{

		int index = row * Chain_.width + column;
		Chain_.counts[index]->count = count;
		Chain_.counts[index]->key = *((uint32_t *)key);
		// memcpy(Chain_.counts[index]->key, key, key_len);
	}

	ChainSketch::SBucket **getTable()
	{

		return Chain_.counts;
	}

	Chain_type Chain_;
};

#endif