
#ifdef __cplusplus
extern "C"{
#endif
typedef struct {
		char* data[32];
		char* formats[32];
		int num_formats;

} Selection;


Selection get();

void set(Selection sel);

void clipboard_has_changed();

void destroy_selection(Selection sel);

Selection new_selection();
#ifdef __cplusplus
}
#endif