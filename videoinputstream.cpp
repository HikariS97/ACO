#include <QPainter>
#include <QCameraInfo>
#include <QThread>

#include "videoinputstream.h"
#include "videoencoder.h"


VideoInputStream::VideoInputStream(QWidget *parent) : QWidget(parent),
    isrunning(false),
    numProc_(false)
{
}


VideoInputStream::~VideoInputStream()
{
}


void VideoInputStream::init()
{
    /* 初始化 VideoSurface 和 Camera */
    videoSurface_ = new VideoSurface;

    // TODO：选择哪个摄像头。
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    Q_ASSERT_X(cameras.size(), "VideoInputStream", "No Available cameras.");
    for (const QCameraInfo &cameraInfo : cameras) {  // TODO
        camera_ = new QCamera(cameraInfo);
        break;
    }

    camera_->setViewfinder(videoSurface_);
    camera_->load();  // Load, then set.
    camera_->setCaptureMode(QCamera::CaptureVideo);

    // TODO：手动选择摄像模式。
    QList<QCameraViewfinderSettings> viewSets = camera_->supportedViewfinderSettings();
    Q_ASSERT_X(viewSets.size(), "VideoInputStream", "No supportedViewfinderSettings.");
    for (auto viewSet : viewSets) {
        viewSet.setResolution(640, 480);
        viewSet.setPixelFormat(QVideoFrame::Format_Jpeg);
        viewSet.setMaximumFrameRate(30);
        viewSet.setMinimumFrameRate(30);
        camera_->setViewfinderSettings(viewSet);
        qDebug() <<" max rate = " << viewSet.maximumFrameRate()\
                 << "min rate = "<< viewSet.minimumFrameRate()\
                 << "resolution = "<<viewSet.resolution()\
                 << "Format = "<<viewSet.pixelFormat();
        break;
    }


    connect(videoSurface_, SIGNAL(frameAvailable(const QVideoFrame&)),
            this, SLOT(handle_frame(const QVideoFrame&)));
}


void VideoInputStream::uninit()
{
    delete camera_;
    delete videoSurface_;
}


void VideoInputStream::start()
{
    bool isFree = !isrunning || numProc_ != 0;
    if (isFree) {
        camera_->start();
        isrunning = true;
        qDebug("Videostream sets to running!");
    }
    else {
        qFatal("Videostream is not closed properly last time.");
    }

}

void VideoInputStream::stop()
{
    // Block Connection.
    emit close_proc();

    if (numProc_ == 0) {
        camera_->stop();
        isrunning = false;
        qDebug("Videosteam stops!");
    }
    else {
        qFatal("Video processors' closing failed.");
    }
}

void VideoInputStream::handle_frame(const QVideoFrame &frame)
{
    currFrame_ = frame;
    update();  // TODO：定时器模拟帧率。

    if (numProc_ != 0) {
        emit transfer_frame(frame);
    }
}


void VideoInputStream::add_proc()
{
    numProcMutex_.lock();
    ++numProc_;
    numProcMutex_.unlock();
}

void VideoInputStream::delete_proc()
{
    numProcMutex_.lock();
    --numProc_;
    Q_ASSERT_X(numProc_ < 0, "delete_proc", "numProc < 0");
    numProcMutex_.unlock();
}


void VideoInputStream::test_print()
{
    qDebug() << "Finished!";
}


void VideoInputStream::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    if (currFrame_.map(QAbstractVideoBuffer::ReadOnly)) {
        QImage img = QImage(currFrame_.bits(),        \
                            currFrame_.width(),       \
                            currFrame_.height(),      \
                            currFrame_.bytesPerLine(),\
                            QVideoFrame::imageFormatFromPixelFormat(currFrame_.pixelFormat())).mirrored(true, true);
        painter.drawImage(QRect(QPoint(0,0), img.size()), img, QRect(QPoint(0,0), img.size()));
        currFrame_.unmap();
    }
}
