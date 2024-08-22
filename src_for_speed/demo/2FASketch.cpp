#include <stdio.h>
#include<iostream>
#include<fstream>
#include <stdlib.h>
#include <unordered_map>
#include <vector>
#include<time.h>
#include "../2FASketch/2FASketch.h"
using namespace std;

#define MEMORY_NUMBER 100
#define START_FILE_NO 1
#define END_FILE_NO 10


struct FIVE_TUPLE{	char key[13];	};
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];

void ReadInTraces(const char *trace_prefix)
{
	for(int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
	{
		char datafileName[100];
		sprintf(datafileName,"%s%d.dat",trace_prefix,datafileCnt-1);
		FILE *fin = fopen(datafileName, "rb");

		FIVE_TUPLE tmp_five_tuple;
		while(fread(&tmp_five_tuple, 1, 13, fin) == 13)
		{
			traces[datafileCnt-1].push_back(tmp_five_tuple);
		}
		fclose(fin);

	printf("Successfully read in %s, %ld packets\n", datafileName, traces[datafileCnt-1].size());

	}
	printf("\n");
}
//argv[1]:out_file
//argv[2]:label_name
int main(int argc,char* argv[])
{
	ReadInTraces("../../data/");
	ofstream fout;
#define TOT_MEM_IN_BYTES (MEMORY_NUMBER * 1024)
#define TOT_BUCKET_NUM (TOT_MEM_IN_BYTES/64)
	Elastic_2FASketch<TOT_BUCKET_NUM> *E_2FA = NULL;
	fout.open(argv[1],ios::app);
	double start_time,end_time;
	double total_time;

	for(int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
	{
		unordered_map<string, int> Real_Freq;
		int packet_cnt=(int)traces[datafileCnt - 1].size();
#define HEAVY_HITTER_THRESHOLD(total_packet) (total_packet * 1 / 10000)
		
		double threshold=HEAVY_HITTER_THRESHOLD(packet_cnt);
		E_2FA = new Elastic_2FASketch<TOT_BUCKET_NUM>(threshold * 0.5);
		start_time=clock();
		for(int i = 0; i < packet_cnt; ++i)
		{
			E_2FA->insert((uint8_t*)(traces[datafileCnt-1][i].key));
		}
  
		end_time=clock();
	 	total_time=((double)(end_time-start_time))/CLOCKS_PER_SEC;	

		vector< pair<string, int> > heavy_hitters;
		start_time=clock();
		E_2FA->get_heavy_hitters(HEAVY_HITTER_THRESHOLD(packet_cnt), heavy_hitters);
		end_time=clock();
		//total_time+=((double)(end_time-start_time))/CLOCKS_PER_SEC;
		fout<<argv[2]<<","<<((double)MEMORY_NUMBER)/1000<<","<<(double)packet_cnt/total_time/1000000<<endl;
		printf("%f\n",total_time);
		delete E_2FA;
		Real_Freq.clear();
	}
}	
