#include "libClipboard.h"
#include <stdio.h>
int main(int argc, char** argv){
	clipboard_has_changed();
	Selection result=get();
	for(int i=0;i<result.num_formats;i++){

		fprintf(stderr, "%s\n",result.formats[i]);
		fprintf(stderr, "%s\n",result.data[i]);
	}
}