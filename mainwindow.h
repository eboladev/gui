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
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_toolButton_2_clicked();

    void on_viewParseButton_clicked();

    void on_toolButton_4_clicked();

    void on_toolButton_3_clicked();

    void on_toolButton_5_clicked();



private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
