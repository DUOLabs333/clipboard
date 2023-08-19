echo "Cloning minimal qt6"
git clone --depth=1 https://github.com/qt/qtbase

echo "Configuring build"
mkdir -p qtbase/build
cd qtbase/build
../configure -disable-widgets -disable-dbus -static -plugindir src/plugins -no-opengl

echo "Building"
cmake --build . --parallel

echo "Building project"
LD_FLAGS="lib/*.a"
if [[ "$OSTYPE" == "darwin"* ]]; then
	LD_FLAGS=$LD_FLAGS" src/plugins/platforms/libqcocoa.a -framework CoreFoundation -framework CoreServices -framework CoreGraphics -framework IOKit -framework Metal -framework AppKit -framework Security -framework CoreVideo -framework IOSurface -lz -framework Carbon -framework QuartzCore"
	CGO_LDFLAGS=$LD_FLAGS go build