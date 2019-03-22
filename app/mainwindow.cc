#include <iostream>
#include <sstream>

#include <QApplication>
#include <QtDebug>
#include <QDebug>
#include <QHeaderView>
#include <QTableView>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QMainWindow>
#include <QWindow>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"
//#include "testform.h"

#include "../netlink.hh"
#include "../util.h"

/* davep 20190318 ; https://www.qcustomplot.com/index.php/introduction */

/* davep 20190319 ; nl_socket_get_fd() + QSocketNotifier */

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	create_db();

    model.setQuery("select BSSID, SSID, Channel from bss order by BSSID desc");
    model.setHeaderData(0, Qt::Horizontal, QObject::tr("BSSID"));
    model.setHeaderData(1, Qt::Horizontal, QObject::tr("SSID"));
    model.setHeaderData(2, Qt::Horizontal, QObject::tr("Channel"));

	update_scan();

	ui->tableView->setModel(&model);

	// increase BSS and SSID column widths
	ui->tableView->setColumnWidth(0, ui->tableView->columnWidth(0)*2);
	ui->tableView->setColumnWidth(1, ui->tableView->columnWidth(1)*2);

//	setIcon(QIcon("Gnome-network-wireless.svg"));
}

MainWindow::~MainWindow()
{
    delete ui;
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
    query.exec("create table bss (id integer primary key, "
               "BSSID char(20), SSID varchar(64), Channel int)");
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
