#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_action_Open_triggered();

    void on_action_Quit_triggered();

    void on_action_About_triggered();

    void on_action_Save_triggered();

    void on_actionAbout_Qt_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
