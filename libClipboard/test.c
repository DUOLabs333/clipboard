#include "libClipboard.h"
#include <stdio.h>
#include <unistd.h>
int main(int argc, char** argv){
	//clipboard_has_changed();
    clipboard_init();
	Selection result=get();

	for(int i=0;i<result.num_formats;i++){

		fprintf(stderr, "What's being set: %s\n",result.formats[i].name);
		fprintf(stderr, "What's being set: %s\n\n",result.formats[i].data);
		if (!strcmp(result.formats[i].name,"text/plain")){
		  result.formats[i].data="Hello!";
		}
		if(!strcmp(result.formats[i].name,"application/x-copyq-owner")){
		  result.formats[i].name="";
		}
	}
	set(result);
	//sleep(1);
	result=get();
	for(int i=0;i<result.num_formats;i++){

		fprintf(stderr, "\nWhat's being get: %s\n",result.formats[i].name);
		fprintf(stderr, "What's being get: %s\n",result.formats[i].data);
	}
	clipboard_wait();

}