package protocol

import(
	"bytes"
	"encoding/base64"
	"encoding/gob"
	"crypto/sha256"
	"fmt"
)

type Selection struct {
	Formats map[string]string
}

func NewSelection() Selection{
	result:=Selection{}
	result.Formats=make(map[string]string)
	return result
}
func Encode (selection Selection) []byte {
	var data []byte

	buffer:=bytes.NewBuffer(data)

	gob.NewEncoder(buffer).Encode(selection)

	result:=make([]byte,base64.StdEncoding.EncodedLen(len(buffer.Bytes())))
	base64.StdEncoding.Encode(result,buffer.Bytes())

	result=append(result,[]byte("\n")...)

	return result
}

func Decode(result []byte) Selection {
	selection:=Selection{}
	var data []byte

	base64.StdEncoding.Decode(data,result)

	buffer:=bytes.NewBuffer(data)

	gob.NewDecoder(buffer).Decode(&selection)

	return selection
}

func Hash(selection Selection) string {
	hash :=sha256.New()
	hash.Write(Encode(selection))
	return fmt.Sprintf("%x",hash.Sum(nil))
}
