#include "libClipboard.h"
#include <stdio.h>
int main(int argc, char** argv){
	//clipboard_has_changed();
	Selection result=get();

	for(int i=0;i<result.num_formats;i++){

		fprintf(stderr, "%s\n",result.formats[i].name);
		fprintf(stderr, "%s\n",result.formats[i].data);
	}
	result.formats[0].data="Hi!";
	set(result);
	for(int i=0;i<result.num_formats;i++){

		fprintf(stderr, "%s\n",result.formats[i].name);
		fprintf(stderr, "%s\n",result.formats[i].data);
	}
}