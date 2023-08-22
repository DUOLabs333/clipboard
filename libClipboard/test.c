#include "libClipboard.h"
#include <stdio.h>
#include <unistd.h>
int main(int argc, char** argv){
	//clipboard_has_changed();
	//init();
	Selection result=get();

	for(int i=0;i<result.num_formats;i++){

		fprintf(stderr, "What's being set: %s\n",result.formats[i].name);
		fprintf(stderr, "What's being set: %s\n\n",result.formats[i].data);
	}
	result.formats[0].data="Hi!";
	set(result);

	result=get();
	for(int i=0;i<result.num_formats;i++){

		fprintf(stderr, "\nWhat's being get: %s\n",result.formats[i].name);
		fprintf(stderr, "What's being get: %s\n",result.formats[i].data);
	}
	sleep(10000000000);
}