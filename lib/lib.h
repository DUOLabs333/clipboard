#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C"{
#endif

//Keep track of number of formats, but just make it a list of formats and their data
typedef struct {
	char *name;
	size_t namelen;
	uint8_t *data;
	size_t datalen;
} Format;

typedef struct {
	size_t num_formats;
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
