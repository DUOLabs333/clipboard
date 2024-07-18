import subprocess
import os
class qt6(BuildBase):
   def build(cls):
    os.chdir("external")

    print("Cloning minimal QT6") 
    subprocess.run(["git", "clone", "--depth=1", "-b", "6.5.2", "https://github.com/qt/qtbase"])
    os.chdir("qtbase")
    os.mkdir("build")
    os.chdir("build")

    if PLATFORM=="darwin":
       ADDITIONAL_FLAGS=[]
       JOBS=[] 
    else:
        ADDITIONAL_FLAGS=["-DINPUT_sessionmanager=no"]
        JOBS=["3"]
    
    print("Configuring QT6 build")
    subprocess.run(['cmake', '-DBUILD_SHARED_LIBS=OFF', '-DINPUT_widgets=no', '-DINPUT_dbus=no', '-DINPUT_timezone=no', '-DINPUT_icu=no', '-DINPUT_opengl=no', '-DINPUT_harfbuzz=no', '-DINPUT_zstd=no', '-G', 'Ninja', '-DINPUT_libb2=qt', '-DINPUT_mimetype_database_compression=none', '-DINPUT_system_zlib=no', '-DINPUT_pcre2=on', '-DINPUT_system_pcre2=no', '-DINPUT_png=qt', '-DINPUT_glib=no', '-DINPUT_vnc=no', '-DINPUT_directfb=no', '-DINPUT_linuxfb=no', '-DINPUT_testlib=no', '-DINPUT_network=no', '-DINPUT_sql=no', '-DINPUT_textmarkdownreader=no', '-DINPUT_imageformatplugin=no', '-DINPUT_libinput=no', '-DINPUT_system_xcb_xinput=on']+ADDITIONAL_FLAGS+['-DINPUT_fontconfig=no', '-DINPUT_freetype=no', '-DINPUT_evdev=no', '-DINPUT_tslib=no', '-DINPUT_vulkan=no', '..'])

    print("Building QT6")
    subprocess.run(["cmake", "--build", ".", "--parallel"]+JOBS)


class libClipboard(BuildBase):
    SRC_FILES=["libClipboard/libClipboard.cpp"]
    INCLUDE_FILES=["libClipboard/libClipboard.h"]

class main(BuildBase):
    
    SRC_FILES=["main.cpp", "protocol/*", "clip/*"]

    STATIC_LIBS=[libClipboard, "qtbase/build/lib/*"]

    FRAMEWORKS=['CoreFoundation', 'CoreServices', 'CoreGraphics', 'IOKit', 'Metal', 'AppKit', 'Security', 'CoreVideo', 'IOSurface', 'Carbon', 'QuartzCore']

    SHARED_LIBS=(["z"] if PLATFORM=="darwin" else ['double-conversion', 'm', 'double-conversion', 'xcb', 'X11', 'Xrender', 'xcb-shape', 'xcb-icccm', 'xcb-sync', 'xcb-xfixes', 'xcb-randr', 'xcb-keysyms', 'xcb-xkb', 'xcb-shm', 'xcb-image', 'xcb-render', 'xcb-render-util', 'xcb-xinput', 'xcb-cursor', 'X11-xcb', 'xkbcommon', 'xkbcommon-x11'])

    OUTPUT_NAME="clipboard"
    def __init__(self):
        if PLATFORM=="darwin":
            self.STATIC_LIBS.append("qtbase/build/plugins/platforms/libqcocoa.a")
        elif ("WAYLAND_DISPLAY" not in os.environ) and ("DISPLAY" in os.envion):
            self.STATIC_LIBS.append("qtbase/build/plugins/platforms/libqxcb.a")
