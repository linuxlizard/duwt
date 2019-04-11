#include <iostream>
#include <sstream>

#include <QApplication>
#include <QtDebug>
#include <QDebug>
#include <QtWidgets>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QWindow>
#include <QtCore/QVariant>

#include "mainwindow.h"
#include "aboutdialog.h"
//#include "testform.h"

#include "cfg80211.h"
#include "util.h"

/* davep 20190318 ; https://www.qcustomplot.com/index.php/introduction */

/* davep 20190319 ; TODO nl_socket_get_fd() + QSocketNotifier */

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent)
{
	create_db();

	setup_ui();
	setup_model();
	setup_views();

	update_scan();

//	setIcon(QIcon("Gnome-network-wireless.svg"));
}

void MainWindow::create_db(void)
{
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName(":memory:");
	if (!db.open()) {
		// messagebox from Qt examples
		QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
			QObject::tr("Unable to establish a database connection.\n"
						"This example needs SQLite support. Please read "
						"the Qt SQL driver documentation for information how "
						"to build it.\n\n"
						"Click Cancel to exit."), QMessageBox::Cancel);
 
		// TODO throw something better
		throw std::runtime_error("no db");
	}

	QSqlQuery query;

	// https://www.sqlite.org/pragma.html#pragma_encoding
	query.exec("pragma encoding=\"UTF-8\"");

	query.exec("create table bss (id integer primary key, "
				"BSSID char(20), SSID varchar(64), Channel int)");
}

void MainWindow::setup_ui(void)
{
	// Qt5.12 Examples Chart
	
	// originally used Qt Designer but couldn't add a QSplitter
	// copy/paste/edit into this method
    QAction *action_Open;
    QAction *action_Save;
    QAction *action_Quit;
    QAction *action_About;
    QAction *actionAbout_Qt;
    QMenu *menu_File;
    QMenu *menu_Help;
	QSplitter *splitter;

	if (objectName().isEmpty())
		setObjectName(QString::fromUtf8("MainWindow"));
	resize(400, 300);

	action_Open = new QAction(tr("&Open"), this);
	action_Open->setObjectName(QString::fromUtf8("action_Open"));
	action_Open->setStatusTip(tr("Open something"));

	action_Save = new QAction(tr("&Save"), this);
	action_Save->setObjectName(QString::fromUtf8("action_Save"));
	action_Save->setStatusTip(tr("Close something"));

	action_Quit = new QAction(tr("&Quit"), this);
	action_Quit->setObjectName(QString::fromUtf8("action_Quit"));
	action_Quit->setStatusTip(tr("Quit the application"));

	action_About = new QAction(tr("&About"), this);
	action_About->setObjectName(QString::fromUtf8("action_About"));

	actionAbout_Qt = new QAction(tr("About &Qt"), this);
	actionAbout_Qt->setObjectName(QString::fromUtf8("actionAbout_Qt"));

	splitter = new QSplitter(this);
	splitter->setObjectName(QString::fromUtf8("splitter"));

	tableView = new QTableView();
	tableView->setObjectName(QString::fromUtf8("tableView"));
	treeView = new QTreeView();
	treeView->setObjectName(QString::fromUtf8("treeView"));

	splitter->addWidget(tableView);
	splitter->addWidget(treeView);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);

	setCentralWidget(splitter);

	menu_File = new QMenu(tr("&File"), this);
	menu_File->setObjectName(QString::fromUtf8("menu_File"));
	menuBar()->addMenu(menu_File);

	menu_Help = new QMenu(tr("&Help"), this);
	menu_Help->setObjectName(QString::fromUtf8("menu_Help"));
	menuBar()->addMenu(menu_Help);

	menu_File->addAction(action_Open);
	menu_File->addAction(action_Save);
	menu_File->addAction(action_Quit);

	menu_Help->addAction(action_About);
	menu_Help->addAction(actionAbout_Qt);

	// XXX do I still need this?
//	retranslateUi(MainWindow);

	statusBar();

	QMetaObject::connectSlotsByName(this);
}

void MainWindow::setup_model(void)
{
	model.setQuery("select BSSID, SSID, Channel from bss order by BSSID desc");
	model.setHeaderData(0, Qt::Horizontal, QObject::tr("BSSID"));
	model.setHeaderData(1, Qt::Horizontal, QObject::tr("SSID"));
	model.setHeaderData(2, Qt::Horizontal, QObject::tr("Channel"));

	tableView->setModel(&model);
}

void MainWindow::setup_views(void)
{
	// TODO set splitter width
	QSplitter *splitter = QObject::findChild<QSplitter*>("splitter");
	if (splitter) {
		qDebug() << "found the splitter";
	}
	else {
		qDebug() << "did not find splitter";
	}

	// increase BSS and SSID column widths
	tableView->setColumnWidth(0, tableView->columnWidth(0)*2);
	tableView->setColumnWidth(1, tableView->columnWidth(1)*2);

}

void MainWindow::update_scan(void)
{
	cfg80211::Cfg80211 cfg80211;

	std::vector<cfg80211::BSS> bss_list;
	cfg80211.get_scan("wlp1s0", bss_list);

	QSqlDatabase db = QSqlDatabase::database();
	QSqlQuery q(db);
	bool flag = q.prepare("INSERT INTO bss (BSSID, SSID, Channel)" 
								" VALUES (:bssid, :ssid, :channel)");
	if (!flag) {
		QSqlError err = db.lastError();
		qDebug() << "prepare failed" << err.text();
		return;
	}

	for ( auto&& bss : bss_list ) {
//		std::stringstream s;
//		s << "found BSS " << bss << " num_ies=" << bss.ie_list.size();
//		std::cerr << "s=" << s << std::endl;
		char mac_addr[20];
		mac_addr_n2a(mac_addr, (const unsigned char *)bss.bssid.data());

		qDebug() << "found BSS=" << mac_addr;

		q.bindValue(":bssid", mac_addr);
		q.bindValue(":ssid", bss.get_ssid().c_str());
		q.bindValue(":channel", bss.freq);

		flag = q.exec();
		if (!flag) {
			qDebug() << "exec failed" << db.lastError();
			break;
		}
	}


	model.setQuery("select BSSID, SSID, Channel from bss order by BSSID desc");
}

void MainWindow::on_action_Open_triggered()
{
	update_scan();
}

void MainWindow::on_action_Quit_triggered()
{
	// quit, I think?
	QCoreApplication::quit();

}

void MainWindow::on_action_About_triggered()
{
	qDebug() << "About triggered";
	AboutDialog dialog(this);
	dialog.exec();
}

void MainWindow::on_action_Save_triggered()
{

}

void MainWindow::on_actionAbout_Qt_triggered()
{
	QApplication::aboutQt();
}
