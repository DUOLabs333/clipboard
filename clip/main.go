package clip

/*
#cgo CFLAGS: -I${SRCDIR}/../libClipboard
#cgo LDFLAGS: ${SRCDIR}/../libClipboard/libClipboard.a -lstdc++ -lm
#cgo darwin LDFLAGS: -framework CoreFoundation -framework CoreServices -framework CoreGraphics -framework IOKit -framework Metal -framework AppKit -framework Security -framework CoreVideo -framework IOSurface -lz -framework Carbon -framework QuartzCore
#cgo linux LDFLAGS: -ldouble-conversion -lstdc++ -lm -ldouble-conversion -lxcb -lX11 -lXrender -lxcb-shape -lxcb-icccm -lxcb-sync -lxcb-xfixes -lxcb-randr -lxcb-keysyms -lxcb-xkb -lxcb-shm -lxcb-image -lxcb-render -lxcb-render-util -lxcb-xinput -lxcb-cursor -lX11-xcb -lxkbcommon -lxkbcommon-x11

#include <libClipboard.h>
#include <stdlib.h>
*/
import "C"

import "unsafe"
import "clipboard/protocol"
import "sync"

var clipboardLock sync.RWMutex

func ClipboardHasChanged(){
	C.clipboard_has_changed()
}

func Get() protocol.Selection{
	clipboardLock.RLock()
	defer clipboardLock.RUnlock()
	
	result := C.get()
	selection :=protocol.NewSelection()

	for _, format := range(unsafe.Slice(result.formats,int(result.num_formats))){
		selection.Formats[C.GoString(format.name)]=C.GoString(format.data)
	}
	C.destroy_selection(result)
	return selection
}
func Set(selection protocol.Selection){

	if len(selection.Formats)==0{
		return
	}
	
	result:=C.new_selection(C.int(len(selection.Formats)));


	i:=0
	formats:=unsafe.Slice(result.formats,int(result.num_formats))

	for key, value := range(selection.Formats){
		formats[i].name=C.CString(key)
		formats[i].data=C.CString(value)
		i++
	}
	clipboardLock.Lock()
	C.set(result)
	clipboardLock.Unlock()
	C.destroy_selection(result)

}

func Wait(){
	C.clipboard_wait()
}

func Init() unsafe.Pointer {
	return C.clipboard_init()
}


