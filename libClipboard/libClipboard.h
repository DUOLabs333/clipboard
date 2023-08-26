
#ifdef __cplusplus
extern "C"{
#endif

//Keep track of number of formats, but just make it a list of formats and their data
typedef struct {
	char *name;
	char *data;
} Format;

typedef struct {
	int num_formats;
	Format *formats;
} Selection;

Selection get();

void set(Selection sel);

void clipboard_has_changed();

void destroy_selection(Selection sel);

Selection new_selection(int len);

void clipboard_process();

void clipboard_wait();

void* clipboard_init();

#ifdef __cplusplus
}
#endif