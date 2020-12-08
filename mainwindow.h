#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QGraphicsVideoItem>
#include <QCamera>
#include <QVideoProbe>

#include "videocontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init();
    void uninit();

private slots:
    void startCamera();

private:
    Ui::MainWindow *ui;

    VideoController *videocontroller_;
    QCamera *camera_;
    QGraphicsVideoItem *videoItem_;
    QGraphicsScene *scene_;
    QVideoProbe *probe_;
};
#endif // MAINWINDOW_H
