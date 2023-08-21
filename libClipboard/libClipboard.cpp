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
#endif

int foo=1;
char *bar[]={""};

auto app=new QGuiApplication(foo,bar);

//Make struct for c that has qtguiapplication, so c programs don't have to point to it

bool operator!=(const Selection& a, const Selection& b){ //C++ doesn't have a default struct comparator
	if (a.num_formats!=b.num_formats){
		return true;
	}

	for(int i=0;i<a.num_formats;i++){
		if (strcmp(a.formats[i],b.formats[i]) || strcmp(a.data[i],b.data[i])){
			return true;
		}
	}
	return false;
}

extern "C" {

	Selection get(){
		auto result= Selection();
		auto mime=qGuiApp->clipboard()->mimeData(QClipboard::Clipboard);

		for( auto format: mime->formats()){
			result.formats[result.num_formats]=strdup(format.toStdString().c_str()); //mime goes out of scope when returning, so we don't want to keep pointers to a deleted object
			result.data[result.num_formats]=strdup(mime->data(format).data());
			result.num_formats++;
		}

	return result;
	}


	void set(Selection sel){

		auto mime= QMimeData();
		for(int i=0; i<sel.num_formats;i++){
			mime.setData(QString::fromUtf8(sel.formats[i],-1),QByteArray::fromRawData(sel.data[i],sizeof(*(sel.data[i]))));
		}
		qGuiApp->clipboard()->setMimeData(&mime,QClipboard::Clipboard);
	}
	
	Selection new_selection(){
		Selection sel;
		return sel;
	}
	void clipboard_has_changed(){
		#ifdef __APPLE__ //On MacOS, only works when app window is in focus. So, must do polling
			auto previous_selection=get();
			fprintf(stderr, "HELLO\n");
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