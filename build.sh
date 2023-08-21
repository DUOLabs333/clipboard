echo "Cloning minimal qt6"
cd libClipboard
git clone --depth=1 -b 6.5.2 https://github.com/qt/qtbase

echo "Configuring QT6 build"
mkdir -p qtbase/build
cd qtbase/build
../configure -disable-widgets -disable-dbus -static -no-opengl

echo "Building QT6"
cmake --build . --parallel

echo "Building libClipboard"
cd ../..
g++ -std=c++17 -c libClipboard.cpp -Iqtbase/build/include -Iqtbase/src

if [[ "$OSTYPE" == "darwin"* ]]; then
	AR(){
		/usr/bin/libtool -static -o "$@"
	}
else
	AR(){
		ar cqT "$1" "${@:2}" && printf "create \"$1\"\naddlib \"$1\"\nsave\nend" | ar -M
	}
fi

AR libClipboard.a libClipboard.o qtbase/build/lib/*.a
rm libClipboard.o

echo "Building project"
go build