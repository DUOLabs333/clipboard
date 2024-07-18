#include <stdint.h>
#ifdef __cplusplus
extern "C"{
#endif

//Keep track of number of formats, but just make it a list of formats and their data
typedef struct {
	char *name;
	int namelen;
	uint8_t *data;
	int datalen;
} Format;

typedef struct {
	int num_formats;
	Format *formats;
} Selection;

Selection clipboard_get();

void clipboard_set(Selection sel);

void clipboard_changed();

void clipboard_destroy_selection(Selection sel);

Selection clipboard_new_selection(int len);

void clipboard_run();

void* clipboard_init();

#ifdef __cplusplus
}
#endif
