#include <stdio.h>
#include <stdlib.h>
#include<iostream>
#include<fstream>
#include <unordered_map>
#include <vector>
#include<algorithm>

#include "../CMHeap/CMHeap.h"
#include "dataset_param.h"
using namespace std;



struct FIVE_TUPLE{	char key[13];	};
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO+  1];

void ReadInTraces(const char *trace_prefix)
{
	for(int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
	{
		double z_alpha = 0;
		char datafileName[100];

		#ifdef CAIDA 
			sprintf(datafileName,"%s%d.dat",trace_prefix,datafileCnt-1);
			printf("processing caida dataset...\n");
		#elif defined(MAWI)
			sprintf(datafileName,"%smw_%d.dat",trace_prefix,datafileCnt-1);
			printf("processing mawi dataset...\n");
		#else
			z_alpha = ZIPF_ALPHA;
			sprintf(datafileName,"%szipf/zipf_%.1f/%d.dat", trace_prefix, z_alpha, datafileCnt-1);
    		printf("processing zipf synthetic dataset with skewness %.1f for file %s...\n", z_alpha, datafileName);
		#endif

		FILE *fin = fopen(datafileName, "rb");

		FIVE_TUPLE tmp_five_tuple;
		traces[datafileCnt - 1].clear();

		while(fread(&tmp_five_tuple, 1, 13, fin) == 13)
		{
			traces[datafileCnt - 1].push_back(tmp_five_tuple);
		}
		fclose(fin);


		printf("Successfully read in %s, %ld packets\n", datafileName, traces[datafileCnt - 1].size());
	}
	printf("\n");
}
vector< pair<double,double> > AE_for_sort,AE,RE_for_sort,RE;
bool operator<(const  pair<double,double> & a,const pair<double,double>  & b)
{
	return a.first<b.first;
}
//argv[1]:out_file
//argv[2]:label_name
//argv[3]:out_model
int main(int argc,char* argv[])
{

	ReadInTraces("../../data/");
	ofstream fout;
	bool flag=0;
	int out_model=atoi(argv[3]);
	if(out_model==6||out_model==7)
		flag=1;
       fout.open(argv[1],ios::app);
#define HEAP_CAPACITY (MEMORY_NUMBER/4 * 1024 / 64)
	CMHeap<4, HEAP_CAPACITY> *cmheap = NULL;
	double average_ARE=0,average_AAE=0;
	double average_precision_rate=0;
	double average_recall_rate=0;
	double average_F_score=0;


	printf("Measurement by Algorithm CMHeap Starts, memory: %dKB\n", MEMORY_NUMBER);
	for(int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
	{
		AE.clear(),AE_for_sort.clear();
		RE.clear(),RE_for_sort.clear();
		unordered_map<string, int> Real_Freq;
		cmheap = new CMHeap<4, HEAP_CAPACITY>(MEMORY_NUMBER/4 * 1024*3);

		int packet_cnt = (int)traces[datafileCnt - 1].size();
		for(int i = 0; i < packet_cnt; ++i)
		{
			cmheap->insert((uint8_t*)(traces[datafileCnt - 1][i].key));

			string str((const char*)(traces[datafileCnt - 1][i].key), 4);

			Real_Freq[str]++;
		}
		double F_score;
		double recall_rate=0;
		double precision_rate=0;
		
#define HEAVY_HITTER_THRESHOLD(total_packet) (total_packet * 1 / 10000)
		vector< pair<string, uint32_t> > heavy_hitters;
		cmheap->get_heavy_hitters(HEAVY_HITTER_THRESHOLD(packet_cnt), heavy_hitters);
			
		
		printf("%d.dat: ", datafileCnt - 1);
		
		double ARE=0,AAE=0;
		double threshold=HEAVY_HITTER_THRESHOLD(packet_cnt);
		unordered_map<string,int>::iterator it;
		
		for(it=Real_Freq.begin();it!=Real_Freq.end();++it)
                {
			if((*it).second>=threshold)
				recall_rate++;
                }
		printf("heavy hitters: <srcIP, count>, threshold=%d\n", HEAVY_HITTER_THRESHOLD(packet_cnt));
		for(int i = 0, j = 0; i < (int)heavy_hitters.size(); ++i)
		{
			uint32_t srcIP;
			memcpy(&srcIP, heavy_hitters[i].first.c_str(), 4);
			it=Real_Freq.find(heavy_hitters[i].first);
//			printf("<%.8x, %d>", srcIP, (int)heavy_hitters[i].second);
//			if(++j % 5 == 0)
//				printf("\n");
//			else printf("\t");
 			if((*it).second>=threshold){
				precision_rate++;
			}
 			double temp_RE=abs((int)heavy_hitters[i].second-(*it).second)/(double)(*it).second;
			ARE+=temp_RE;
			double temp_AE=(double)abs((int)heavy_hitters[i].second-(*it).second);
			AAE+=temp_AE;
			AE_for_sort.push_back(make_pair(temp_AE,0));
			RE_for_sort.push_back(make_pair(temp_RE,0));
		}
		sort(AE_for_sort.begin(),AE_for_sort.end());
		sort(RE_for_sort.begin(),RE_for_sort.end());
		AE.push_back(make_pair(0,0));
		RE.push_back(make_pair(0,0));
		for(int AE_i=0;AE_i<(int)AE_for_sort.size();++AE_i)
		{
			double temp_cdf=((double)(AE_i+1))/(int)AE_for_sort.size();
			AE.push_back(make_pair(AE_for_sort[AE_i].first,temp_cdf));
			RE.push_back(make_pair(RE_for_sort[AE_i].first,temp_cdf));
		}

//		printf("\n");
                ARE/=heavy_hitters.size();
		AAE/=heavy_hitters.size();
		recall_rate=precision_rate/recall_rate;
		// variable recall_rate on the right means the number of all the true heavy hitters in the dataset and precision_rate means the number of the predicted heavy hitters using the algorithm. After the two divisions, they will mean recall rate and precision rate, respectively.
		precision_rate/=heavy_hitters.size();
		F_score=2*recall_rate*precision_rate/(precision_rate+recall_rate);
		printf("precision_rate=%f\n",precision_rate);
		printf("recall_rate=%f\n",recall_rate);
		printf("F_score=%f\n",F_score);
		printf("ARE=%f\n",ARE);
                printf("AAE=%f\n",AAE);
		average_ARE+=ARE;
		average_AAE+=AAE;
		average_precision_rate+=precision_rate;
		average_recall_rate+=recall_rate;
		average_F_score+=F_score;
		switch(out_model){
			case 1:
		fout<<argv[2]<<","<<((double)MEMORY_NUMBER)/1000<<","<<ARE<<endl;
		break;
			case 2:
              fout<<argv[2]<<","<<((double)MEMORY_NUMBER)/1000<<","<<AAE<<endl;
	        break;
			case 3:

 		fout<<argv[2]<<","<<((double)MEMORY_NUMBER)/1000<<","<<precision_rate<<endl;
		break;
			case 4:
              fout<<argv[2]<<","<<((double)MEMORY_NUMBER)/1000<<","<<recall_rate<<endl;
		break;
			case 5:
              fout<<argv[2]<<","<<((double)MEMORY_NUMBER)/1000<<","<<F_score<<endl;
		break;
			case 6:
		for(int AE_i=0;AE_i<(int)AE.size();++AE_i)
		{
			fout<<argv[2]<<","<<AE[AE_i].first<<","<<AE[AE_i].second<<endl;
		}
		break;
			case 7:
   		for(int RE_i=0;RE_i<(int)RE.size();++RE_i)
		{
			fout<<argv[2]<<","<<RE[RE_i].first<<","<<RE[RE_i].second<<endl;
		}
		break;
	}
		if(flag)
			break;
	      	delete cmheap;
		Real_Freq.clear();
     }
		average_ARE/=10;
		average_AAE/=10;
		average_precision_rate/=10;
		average_recall_rate/=10;
		average_F_score/=10;
		printf("five average results below can be ignored if calculating CDF\n");
		printf("average precision rate=%f\n",average_precision_rate);
		printf("average recallrate=%f\n",average_recall_rate);
		printf("average F1 score=%f\n",average_F_score);
		printf("average ARE=%f\n",average_ARE);
        	printf("average AAE=%f\n",average_AAE);
}	
