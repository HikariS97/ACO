#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QCamera>



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class VideoInputStream;
class VideoEncoder;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_button_VideoStream_clicked();
    void on_button_Encoder_clicked();

private:
    Ui::MainWindow *ui;

    VideoInputStream *videoInputStream_;

    // Video Encoder.
    QThread *videoEncoderThread_;
    VideoEncoder *videoEncoder_;
};
#endif // MAINWINDOW_H
