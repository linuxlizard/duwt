#include <QApplication>
#include <QtDebug> // include <QtDebug> to use <<
#include <QDebug>
#include "mainwindow.h"

#include "galileo.h"

// https://doc.qt.io/qt-5/debug.html

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// https://doc.qt.io/qt-5/appicon.html
	// https://developer.gnome.org/platform-overview/stable/dev-launching.html.en
	app.setWindowIcon(QIcon("Gnome-network-wireless.svg"));

    MainWindow w;
	QSize sz = w.size();
	sz.rwidth() *= 2;
	w.resize(sz);
	w.show();

	return app.exec();
}
