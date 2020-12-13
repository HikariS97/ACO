#include <QPainter>
#include <QCameraInfo>
#include <iostream>
#include <QThread>

#include "videoinputstream.h"
#include "videoencoder.h"


VideoInputStream::VideoInputStream(QWidget *parent) : QWidget(parent)
{
}


void VideoInputStream::init()
{
    /* 初始化 VideoSurface 和 Camera */

    videoSurface_ = new VideoSurface;

    // TODO：选择哪个摄像头。
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    Q_ASSERT_X(cameras.size(), "VideoInputStream", "No Available cameras.");
    for (const QCameraInfo &cameraInfo : cameras) {
        camera_ = new QCamera(cameraInfo);
    }

    camera_->setViewfinder(videoSurface_);
    camera_->load();
    camera_->setCaptureMode(QCamera::CaptureVideo);

    // TODO：选择哪个摄像模式。
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


    camera_->start();

    connect(videoSurface_, SIGNAL(frameAvailable(const QVideoFrame&)),
            this, SLOT(frameHandler(const QVideoFrame&)));

    // TODO
    initEncoder();
}

void VideoInputStream::uninit()
{
    delete camera_;
    delete videoSurface_;
}

void VideoInputStream::frameHandler(const QVideoFrame &frame)
{
    currFrame_ = frame;
    update();  // TODO：定时器模拟帧率。

    if (!onlyDisplay_)
        pushInBuffer();
    else
        if (!dataBuffer_.empty())
            emptyBuffer();
}

void VideoInputStream::pushInBuffer()
{
    std::unique_ptr<Data> newFrame(new Data);

    currFrame_.map(QAbstractVideoBuffer::ReadOnly);
    QVideoFrame input = QVideoFrame(QImage(currFrame_.bits(),            \
                                                currFrame_.width(),           \
                                                currFrame_.height(),          \
                                                currFrame_.bytesPerLine(),    \
                                                QVideoFrame::imageFormatFromPixelFormat(currFrame_.pixelFormat())\
                                                 ).convertToFormat(QImage::Format_RGBX8888));
    currFrame_.unmap();

    // 检查是否转换成功。
    if (!input.isValid()) {
        std::cerr << "Input frame's format error." << std::endl;
        abort();
    }

    input.map(QAbstractVideoBuffer::ReadOnly);
    newFrame->width = input.width();
    newFrame->height = input.height();
    newFrame->dataSize = input.mappedBytes();
    newFrame->framerate = 30;  // TODO: 这怎么算来着？
    newFrame->bytesPerLine = input.bytesPerLine();

    std::unique_ptr<uchar[]> data(new uchar[newFrame->dataSize]);
    memcpy(data.get(), input.bits(), newFrame->dataSize);
    input.unmap();

    newFrame->data = std::move(data);

    // Put into buffer.
    bufferMutex_.lock();
    dataBuffer_.push_back(std::move(newFrame));
    bufferMutex_.unlock();
    videoEncoder_->waitDataBuffer_.wakeAll();
}

void VideoInputStream::initEncoder()
{
    videoEncoder_ = new VideoEncoder;
    videoEncoder_->init();

    encoderThread_ = new QThread;
    videoEncoder_->moveToThread(encoderThread_);

    encoderThread_->start();


    connect(encoderThread_, &QThread::finished, videoEncoder_, &QObject::deleteLater);
    connect(this, SIGNAL(startEncodeLoop(VideoInputStream*)),
            videoEncoder_, SLOT(encodeLoop(VideoInputStream*)));

    emit startEncodeLoop(this);
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

void VideoInputStream::emptyBuffer()
{
    bufferMutex_.lock();
    std::deque<std::unique_ptr<Data>> tmp;
    std::swap(dataBuffer_, tmp);
    bufferMutex_.unlock();
}
