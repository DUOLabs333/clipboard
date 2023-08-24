echo "Cloning minimal qt6"
cd libClipboard
git clone --depth=1 -b 6.5.2 https://github.com/qt/qtbase

echo "Configuring QT6 build"
mkdir -p qtbase/build
cd qtbase/build
#../configure -disable-widgets -disable-dbus -static -no-opengl -qt-harfbuzz -disable-feature-zstd
if [[ "$OSTYPE" == "darwin"* ]]; then
	ADDITIONAL_FLAGS=
else
	ADDITIONAL_FLAGS="-DINPUT_sessionmanager=no"
fi
cmake -DBUILD_SHARED_LIBS=OFF -DINPUT_widgets=no -DINPUT_dbus=no -DINPUT_timezone=no -DINPUT_icu=no -DINPUT_opengl=no -DINPUT_harfbuzz=no -DINPUT_zstd=no -G "Ninja" -DINPUT_libb2=qt -DINPUT_mimetype_database_compression=none -DINPUT_system_zlib=no -DINPUT_pcre2=on -DINPUT_system_pcre2=no -DINPUT_png=qt -DINPUT_glib=no -DINPUT_vnc=no -DINPUT_directfb=no  -DINPUT_linuxfb=no -DINPUT_testlib=no -DINPUT_network=no -DINPUT_sql=no -DINPUT_textmarkdownreader=no -DINPUT_imageformatplugin=no -DINPUT_libinput=no -DINPUT_system_xcb_xinput=on $ADDITIONAL_FLAGS -DINPUT_fontconfig=no -DINPUT_freetype=no -DINPUT_evdev=no -DINPUT_tslib=no -DINPUT_vulkan=no ..

echo "Building QT6"
if [[ "$OSTYPE" == "darwin"* ]]; then
	JOBS=
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then #Machine runs out of memory if set any higher
	JOBS=3
fi
cmake --build . --parallel $JOBS

echo "Building libClipboard"
cd ../..
g++ -std=c++17 -c libClipboard.cpp -Iqtbase/build/include -Iqtbase/src

if [[ "$OSTYPE" == "darwin"* ]]; then
	AR(){
		/usr/bin/libtool -static -o "$@"
	}
else
	AR(){
		rm -f "$1"; ar cqT "$1" "${@:2}" && printf "create \"$1\"\naddlib \"$1\"\nsave\nend" | ar -M
	}
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
	ADDITIONAL_LIBS="qtbase/build/plugins/platforms/libqcocoa.a"
elif [[ -z ${WAYLAND_DISPLAY+x} ]] && [[ ! -z ${DISPLAY+x} ]]; then
	ADDITIONAL_LIBS="qtbase/build/plugins/platforms/libqxcb.a"
fi

AR libClipboard.a libClipboard.o qtbase/build/lib/*.a "$ADDITIONAL_LIBS"
rm libClipboard.o

echo "Building project"
cd ..
GOPROXY=direct go build