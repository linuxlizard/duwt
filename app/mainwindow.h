#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlQueryModel>
#include <QTableView>
#include <QTreeView>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

	void setup_ui(void);
	void setup_model(void);
	void setup_views(void);

private slots:
    void on_action_Open_triggered();
    void on_action_Quit_triggered();
    void on_action_About_triggered();
    void on_action_Save_triggered();
    void on_actionAbout_Qt_triggered();

private:
//    Ui::MainWindow *ui;

	QTableView* tableView;
	QTreeView* treeView;

	QSqlQueryModel model;
	
	void create_db(void);
	void update_scan(void);
};

#endif // MAINWINDOW_H
