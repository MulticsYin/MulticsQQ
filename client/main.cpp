#include <QtGui/QApplication>
#include "maindlg.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);    
    QTranslator translator;
    translator.load(":/data/zh_CN");
    a.installTranslator(&translator);
    // 用于中文显示
    QTextCodec::setCodecForTr(QTextCodec::codecForName("GB2312"));
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("GB2312"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("GB2312"));
    MainDlg w;
    w.show();
    return a.exec();
}
