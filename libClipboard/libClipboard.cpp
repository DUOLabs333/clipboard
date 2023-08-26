#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

#include <QtCore/QString>
#include <QtCore/QMimeData>
#include<QtCore/QObject>

#include <corelib/plugin/qplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include "libClipboard.h"

#ifdef __APPLE__
	Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
	auto ret=setenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM","1",1);
#else
    Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);
#endif

int foo=1;
char *bar[]={""};

bool operator!=(const Selection& a, const Selection& b){ //C++ doesn't have a default struct comparator
	if (a.num_formats!=b.num_formats){
		return true;
	}

	for(int i=0;i<a.num_formats;i++){
		if (strcmp(a.formats[i].name,b.formats[i].name) || strcmp(a.formats[i].data,b.formats[i].data)){
			return true;
		}
	}
	return false;
}

extern "C" {

	void clipboard_process(){
	   qGuiApp->processEvents();
	}
    
    void clipboard_wait(){
        qGuiApp->exec();
    }
    
    void* clipboard_init(){
        return new QGuiApplication(foo,bar);
    }
	Selection get(){

		auto mime=qGuiApp->clipboard()->mimeData(QClipboard::Clipboard);
        
        auto formats=mime->formats();
		auto result=new_selection(formats.size());
       
		for( int i=0; i<result.num_formats; i++){
			auto format=formats[i];
			result.formats[i].name=strdup(format.toStdString().c_str()); //mime goes out of scope when returning, so we don't want to keep pointers to a deleted object
			result.formats[i].data=strdup(mime->data(format).data());
		}

	return result;
	}


	void set(Selection sel){

		auto mime= new QMimeData;
		for(int i=0; i<sel.num_formats;i++){
			mime->setData(strdup(sel.formats[i].name),strdup(sel.formats[i].data));
		}

		qGuiApp->clipboard()->setMimeData(mime,QClipboard::Clipboard);
	}
	
	Selection new_selection(int len){
		auto result = Selection();
		result.num_formats=len;
		result.formats=(Format *)malloc(len*sizeof(Format));
		for( int i=0; i<result.num_formats; i++){
		  result.formats[i]=Format();
		}
		return result;


	}

	void destroy_selection(Selection sel){
		for(int i=0; i<sel.num_formats;i++){
			free(sel.formats[i].name);
			free(sel.formats[i].data);
		}
		free(sel.formats);
	}

	void clipboard_has_changed(){
		#ifdef __APPLE__ //On MacOS, only works when app window is in focus. So, must do polling
			auto previous_selection=get();
			while(true){
				if (get()!=previous_selection){
					return;
				}
			}
		#else
			QEventLoop loop;
			auto connection=QObject::connect(qGuiApp->clipboard(),&QClipboard::dataChanged,&loop,&QEventLoop::quit);
			loop.exec();
			QObject::disconnect(connection);
		#endif
	}

}