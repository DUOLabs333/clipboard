#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

#include <QtCore/QString>
#include <QtCore/QMimeData>
#include <QtCore/QObject>
#include <QtCore/QThread>

#include <corelib/plugin/qplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include "lib.h"
#include <ctime>
#include <string_view>
#include <string>
#include <tuple>

#ifdef __APPLE__
	Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
	auto ret=setenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM","1",1);
#else
    Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);
#endif

int foo=1;
char *bar[]={""};


bool operator!=(const Format& a, const Format& b){
	(std::string_view(a.namelen, a.name) != std::string_view(b.namelen, b.name)) || (std::string_view(a.datalen, a.data) != std::string_view(b.datalen, b.data)); 
}

bool operator!=(const Selection& a, const Selection& b){ //C++ doesn't have a default struct comparator
	if (a.num_formats!=b.num_formats){
		return true;
	}

	for(int i=0;i<a.num_formats;i++){
		if (a.formats[i]!=b.formats[i]){
			return true;
		}
	}
	return false;
}


std::pair<char*, int> string_to_char(std::string str){
	char* buf;
	int size;

	size=str.size();
	buf=(char*)malloc(size);

	memcpy(buf, str.data(), size);

	return {buf, int};
}

std::string char_to_string(char* buf, int size){
	return std::string(buf, size);
}

extern "C" {
    
    void clipboard_run(){
        while(1){
            qGuiApp->processEvents();
            #ifdef __linux__
                QThread::msleep(50); //Without the sleep, processEvents will be too fast for, so get and set will be delayed and be inconsistently run (this is also why we don't use exec())
            #endif
        }
    }
    
    void* clipboard_init(){
        return new QGuiApplication(foo,bar);
    }
    Selection clipboard_get(){
	auto mime=qGuiApp->clipboard()->mimeData(QClipboard::Clipboard);

	auto formats=mime->formats();
	auto result=new_selection(formats.size());

	for( int i=0; i<result.num_formats; i++){
		auto& mime_format=formats[i];
		auto& result_format=result.formats[i];

		auto& name=mime_format.toStdString();
		auto& data=mime->data(mime_format);
		
		std::tie(result_format.name, result_format.namelen) = string_to_char(name); //mime goes out of scope when returning, so we don't want to keep pointers to a deleted object

		std::tie(result_format.data, result_format.datalen) = string_to_char(data);
	}
	return result;
	}


	void clipboard_set(Selection sel){
		auto mime= new QMimeData;
		for(int i=0; i<sel.num_formats;i++){
			auto& sel_format=sel.formats[i];
			mime->setData(QString(sel_format.name, sel_format.namelen),QByteArray(sel_format.data, sel_format.datalen));
		}

		qGuiApp->clipboard()->setMimeData(mime,QClipboard::Clipboard);
	}
	
	Selection clipboard_new_selection(int len){
		auto result = Selection();
		result.num_formats=len;
		result.formats=(Format *)malloc(len*sizeof(Format));
		for( int i=0; i<result.num_formats; i++){
		  result.formats[i]=Format();
		}
		return result;


	}

	void clipboard_destroy_selection(Selection sel){
		for(int i=0; i<sel.num_formats;i++){
			free(sel.formats[i].name);
			free(sel.formats[i].data);
		}
		free(sel.formats);
	}

	void clipboard_changed(){
		#ifdef __APPLE__ //On MacOS, only works when app window is in focus. So, must do polling
			QThread::sleep(5);
		#else
			QEventLoop loop;
			auto connection=QObject::connect(qGuiApp->clipboard(),&QClipboard::dataChanged,&loop,&QEventLoop::quit);
			loop.exec();
			QObject::disconnect(connection);
		#endif
	}

}
