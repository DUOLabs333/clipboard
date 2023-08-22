package clip

/*
#cgo CFLAGS: -I${SRCDIR}/../libClipboard
#cgo LDFLAGS: ${SRCDIR}/../libClipboard/libClipboard.a -lstdc++
#cgo darwin LDFLAGS: ${SRCDIR}/../libClipboard/qtbase/build/src/plugins/platforms/libqcocoa.a -framework CoreFoundation -framework CoreServices -framework CoreGraphics -framework IOKit -framework Metal -framework AppKit -framework Security -framework CoreVideo -framework IOSurface -lz -framework Carbon -framework QuartzCore

#include <libClipboard.h>
#include <stdlib.h>
*/
import "C"

import "unsafe"
import "clipboard/protocol"

func ClipboardHasChanged(){
	C.clipboard_has_changed()
}


func Get() protocol.Selection{
	result := C.get()
	selection :=protocol.NewSelection()

	for _, format := range(unsafe.Slice(result.formats,int(result.num_formats))){
		selection.Formats[C.GoString(format.name)]=C.GoString(format.data)
	}

	return selection
}
func Set(selection protocol.Selection){
	if len(selection.Formats)==0{
		return
	}
	result:=C.new_selection()
	formats:=make([]C.Format,len(selection.Formats))
	i:=0
	for key, value := range(selection.Formats){
		formats[i].name=C.CString(key)
		formats[i].data=C.CString(value)
		i++
	}
	result.formats=&formats[0]
	result.num_formats=C.int(len(formats))
	C.set(result)

}


