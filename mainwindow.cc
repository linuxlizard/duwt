#include <QtDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"
//#include "testform.h"

#include "netlink.hh"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_Open_triggered()
{
	Netlink netlink;
//    TestForm testform;
//    testform.setModal(true);
//    testform.exec();
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
