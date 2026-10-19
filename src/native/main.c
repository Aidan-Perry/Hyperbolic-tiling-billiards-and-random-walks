#include "htb/engine.h"
#include <flint/flint.h>

#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_progress(json_t*event,void*user){(void)user;char*s=json_dumps(event,JSON_COMPACT|JSON_ENCODE_ANY);if(s){puts(s);fflush(stdout);free(s);}}

static int run_benchmark(const char*mode){
    int counts[]={10,25,50,100,200};printf("mode,bounces,status,completed,runtime_seconds,comparisons,candidates,max_precision\n");
    for(size_t k=0;k<sizeof(counts)/sizeof(counts[0]);k++){
        htb_run_request q;memset(&q,0,sizeof(q));snprintf(q.run_id,sizeof(q.run_id),"benchmark-%d",counts[k]);q.backend=strcmp(mode,"exact")==0?HTB_BACKEND_CALCIUM:HTB_BACKEND_ARB;
        snprintf(q.tiling.id,sizeof(q.tiling.id),"elliptical-square");snprintf(q.tiling.version,sizeof(q.tiling.version),"1");q.tiling.parameter_count=2;
        snprintf(q.tiling.parameters[0].key,sizeof(q.tiling.parameters[0].key),"centerX");snprintf(q.tiling.parameters[0].value,sizeof(q.tiling.parameters[0].value),"0");
        snprintf(q.tiling.parameters[1].key,sizeof(q.tiling.parameters[1].key),"centerY");snprintf(q.tiling.parameters[1].value,sizeof(q.tiling.parameters[1].value),"0");q.initial_precision=128;q.max_precision=8192;q.memory_limit_mb=2048;q.trajectory_count=1;
        htb_trajectory_input*in=&q.trajectories[0];snprintf(in->id,sizeof(in->id),"A");snprintf(in->start_u,sizeof(in->start_u),"1/10");snprintf(in->start_v,sizeof(in->start_v),"-1/5");snprintf(in->direction_u,sizeof(in->direction_u),"1");snprintf(in->direction_v,sizeof(in->direction_v),"37/100");in->bounces=counts[k];
        htb_run_result r;if(!htb_run(&q,&r,NULL,NULL))return 1;printf("%s,%d,%s,%d,%.9f,%llu,%llu,%ld\n",mode,counts[k],htb_status_name(r.trajectories[0].status),r.trajectories[0].completed_bounces,r.runtime_seconds,(unsigned long long)r.comparisons,(unsigned long long)r.candidates_examined,r.maximum_precision);fflush(stdout);htb_run_result_clear(&r);
    }return 0;
}

int main(int argc,char**argv){
    if(argc>=2&&strcmp(argv[1],"--benchmark")==0)return run_benchmark(argc>=3?argv[2]:"scientific");
    FILE*input=stdin;if(argc==3&&strcmp(argv[1],"--request")==0){input=fopen(argv[2],"rb");if(!input){fprintf(stderr,"cannot open request file\n");return 2;}}
    json_error_t json_error;json_t*root=json_loadf(input,JSON_REJECT_DUPLICATES,&json_error);if(input!=stdin)fclose(input);
    if(!root){json_t*e=json_pack("{s:s,s:s}","type","error","message",json_error.text);print_progress(e,NULL);json_decref(e);return 2;}
    htb_run_request request;char error[512]={0};if(!htb_parse_request_json(root,&request,error,sizeof(error))){json_decref(root);json_t*e=json_pack("{s:s,s:s}","type","error","message",error);print_progress(e,NULL);json_decref(e);return 2;}json_decref(root);
    htb_run_result result;if(!htb_run(&request,&result,print_progress,NULL)){json_t*e=json_pack("{s:s,s:s}","type","error","message","native runner failed");print_progress(e,NULL);json_decref(e);return 1;}
    json_t*out=htb_run_result_json(&request,&result);print_progress(out,NULL);json_decref(out);htb_run_result_clear(&result);flint_cleanup_master();return 0;
}
