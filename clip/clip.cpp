#include "clip.hpp"
#include "../lib/lib.h"
#include "../protocol/protocol.hpp"
#include <mutex>
#include <cstring>
std::mutex clipboardLock;

void clip::Changed(){
	return clipboard_changed();

}

 protocol::Selection clip::Get(){
 	clipboardLock.lock();
 	protocol::Selection sel;

	auto raw_sel = clipboard_get();

	for (int i =0; i< raw_sel.num_formats; i++){
		auto& raw_format=raw_sel.formats[i];
		sel.Formats[{raw_format.name, raw_format.namelen}]={raw_format.data, raw_format.data+raw_format.datalen};
	}

	clipboard_destroy_selection(raw_sel);
	clipboardLock.unlock();

	return sel;
 }

void clip::Set(protocol::Selection sel){
	clipboardLock.lock();
	auto raw_sel=clipboard_new_selection(sel.Formats.size());
	
	int i=0;
	for (auto& [key, val] : sel.Formats){
		auto& format=raw_sel.formats[i];
		format.namelen=key.size();
		format.name=(char*)malloc(sizeof(uint8_t)*format.namelen);
		memcpy(format.name, key.data(), format.namelen);


		format.datalen=val.size();
		format.data=(uint8_t*)malloc(sizeof(uint8_t)*format.datalen);
		memcpy(format.data, val.data(), format.datalen);

		i++;
	}

	clipboard_set(raw_sel);
	clipboardLock.unlock();

	clipboard_destroy_selection(raw_sel);
}

void clip::Run(){
	return clipboard_run();
}

void* clip::Init(){
	return clipboard_init();
}


