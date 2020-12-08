#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCameraInfo>
#include <QThread>
#include <QTimer>

#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

void MainWindow::init()
{
    videoItem_ = new QGraphicsVideoItem;
    scene_ = new QGraphicsScene;
    probe_ = new QVideoProbe;
    videocontroller_ = new VideoController;

    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : cameras) {
        camera_ = new QCamera(cameraInfo);
    }

    camera_->setViewfinder(videoItem_);
    camera_->load();
    camera_->setCaptureMode(QCamera::CaptureVideo);


    videoItem_->setSize((ui->VideoDisplayer->geometry()).size());
    videoItem_->setPos(0, 0);

    QList<QCameraViewfinderSettings> viewSets = camera_->supportedViewfinderSettings();
    if (viewSets.size())
    {
        for (auto viewSet : viewSets) {
            qDebug() <<" max rate = " << viewSet.maximumFrameRate()\
                     << "min rate = "<< viewSet.minimumFrameRate()\
                     << "resolution = "<<viewSet.resolution()\
                     << "Format = "<<viewSet.pixelFormat();
            camera_->setViewfinderSettings(viewSet);
            break;
        }
    }
    else
        std::cerr << "No supportedViewfinderSettings." << std::endl;

    probe_->setSource(camera_);
    connect(probe_, SIGNAL(videoFrameProbed(QVideoFrame)), videocontroller_, SLOT(push_in_buffer(QVideoFrame)));

    ui->VideoDisplayer->setScene(scene_);
    ui->VideoDisplayer->scene()->addItem(videoItem_);
    ui->VideoDisplayer->show();

    QTimer *timer = new QTimer(this);
    timer->singleShot(100, this, SLOT(startCamera()));
    delete timer;

    videocontroller_->start();
}

void MainWindow::startCamera()
{
    camera_->start();
}

void MainWindow::uninit()
{
    delete videocontroller_;
    delete videoItem_;
    delete camera_;
    delete scene_;
}

MainWindow::~MainWindow()
{
    delete ui;
    uninit();
}

