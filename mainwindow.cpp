#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCameraInfo>
#include <QThread>
#include <QTimer>

#include "videoinputstream.h"
#include "videoencoder.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , videoInputStream_(nullptr)
    , videoEncoder_(nullptr)
{
    ui->setupUi(this);
    videoInputStream_ = ui->videoInputStream_;

    // TODO：Qustion?
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->updateGeometry();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_button_VideoStream_clicked()
{
    /* init->start->stop->uninit */
    if (videoInputStream_->isrunning) {
        videoInputStream_->stop();
        videoInputStream_->uninit();
        // TODO: set button's name value.
    }
    else {
        videoInputStream_->init();
        videoInputStream_->start();
        // TODO: set button's name value.
    }
}

void MainWindow::on_button_Encoder_clicked()
{
    if (videoEncoder_ == nullptr)
    {
        videoEncoder_ = new VideoEncoder;
        if (!videoEncoder_->init()) {
            qFatal("Video encoder openning failed.");
        }

        videoEncoderThread_ = new QThread;
        videoEncoder_->moveToThread(videoEncoderThread_);
        videoEncoderThread_->start();

        /* Connect 总结
         * 1. 线程清理；
         * 2. 转发和接收；
         * 3. 视频流关闭。
         */
        connect(videoEncoderThread_, &QThread::finished, videoEncoder_, &QObject::deleteLater);
        connect(videoEncoderThread_, &QThread::finished, videoInputStream_, &VideoInputStream::test_print);  // TODO: TEST.
        connect(videoInputStream_, SIGNAL(transfer_frame(const QVideoFrame &frame)), videoEncoder_, SLOT(receive_frame(const QVideoFrame &frame)));
        connect(videoInputStream_, SIGNAL(close_proc()), videoEncoder_, SLOT(close_proc()));

        videoInputStream_->add_proc();
    }
    else {
        // TODO: Deconstructor.
        videoEncoder_->close();
        delete videoEncoder_;
        videoEncoder_ = nullptr;

        videoInputStream_->delete_proc();
    }
}
