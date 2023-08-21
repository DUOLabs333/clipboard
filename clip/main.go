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

func listOfCharsToSliceOfStrings(list [32]*C.char ,length int) []string{
	var slice []string
	for _, str := range list{
		slice=append(slice,C.GoString(str))
		C.free(unsafe.Pointer(str))
	}
	//C.free(list)
	return slice
}
func sliceofStringstoListOfChars(slice []string) (list [32]*C.char){
	for idx,str :=range(slice){
		list[idx]=C.CString(str)
	}
	return list
}

func Get(){
	result := C.get()
	selection :=new(protocol.Selection)

	selection.formats=listOfCharsToSliceOfStrings(result.formats,int(result.num_formats))
	selection.data=listOfCharsToSliceOfStrings(result.data,int(result.num_formats))

	return selection
}
func Set(selection protocol.Selection){
	result:=C.new_selection()
	result.formats=sliceofStringstoListOfChars(selection.formats)
	result.data=sliceofStringstoListOfChars(selection.data)
	C.set(result)
}


