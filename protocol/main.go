package protocol

import(
	"bytes"
	"encoding/base64"
	"encoding/gob"
	"crypto/sha256"
	"encoding/json"
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

func Decode(result []byte)(selection Selection, err error) {
	selection=Selection{}
	data:=make([]byte,base64.StdEncoding.DecodedLen(len(result)))

	n,err:=base64.StdEncoding.Decode(data,result)

	if err!=nil{
		return
	}

	data=data[:n]
	buffer:=bytes.NewBuffer(data)

	err=gob.NewDecoder(buffer).Decode(&selection)

	return
}

func Hash(selection Selection) string {
	hash :=sha256.New()
	marshal,_:=json.Marshal(selection)
	hash.Write(marshal)
	return fmt.Sprintf("%x",hash.Sum(nil))
}
