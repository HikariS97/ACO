#include "videocontroller.h"
#include <QDebug>
#include <iostream>





VideoController::VideoController()
{

}

VideoController::~VideoController()
{

}

//void VideoController::run()
//{
//    while(running_)
//    {
//        waitForInput();
//        if (!running_) break;
//        process();
//    }
//}

//static FILE *output = fopen("640x480_30fps_YUV400P.yuv", "wb+");
//void VideoController::process()
//{
//    std::unique_ptr<Data> input = getInput();

//    while (input)
//    {
//        std::cout << "." << std::flush;
//        std::unique_ptr<Data> convertedInput = rgb32toyuv420p(std::move(input));
//        fwrite(convertedInput->data.get(), sizeof (convertedInput->data[0]), convertedInput->dataSize, output);
//        input = getInput();
//    }
//}

//std::unique_ptr<Data> VideoController::getInput()
//{
//    bufferMutex_.lock();
//    std::unique_ptr<Data> r;
//    if (!dataBuffer_.empty())
//    {
//        r = std::move(dataBuffer_.front());
//        dataBuffer_.pop_front();
//    }
//    bufferMutex_.unlock();
//    return r;
//}

//void VideoController::push_in_buffer(const QVideoFrame &frame)
//{  // in Main thread.
//    wakeUp();

//    QVideoFrame frame_tmp = frame;
//    frame_tmp.map(QAbstractVideoBuffer::ReadOnly);
//    std::unique_ptr<Data> newFrame(new Data);
//    QVideoFrame input = QVideoFrame(QImage(frame_tmp.bits(),            \
//                                                frame_tmp.width(),           \
//                                                frame_tmp.height(),          \
//                                                frame_tmp.bytesPerLine(),    \
//                                                QVideoFrame::imageFormatFromPixelFormat(frame_tmp.pixelFormat())\
//                                                ).convertToFormat(QImage::Format_RGBX8888));
//    frame_tmp.unmap();
//    input.map(QAbstractVideoBuffer::ReadOnly);
//    qDebug() << input.pixelFormat();
//    qDebug() << int(input.bits()[0]);
//    qDebug() << int(input.bits()[1]);
//    qDebug() << int(input.bits()[2]);
//    qDebug() << int(input.bits()[3]);
//    newFrame->width = input.width();
//    newFrame->height = input.height();
//    newFrame->dataSize = input.mappedBytes();
//    newFrame->framerate = 30;  // TODO: 这怎么算来着？
//    newFrame->bytesPerLine = input.bytesPerLine();

//    std::unique_ptr<uchar[]> data(new uchar[newFrame->dataSize]);
//    memcpy(data.get(), input.bits(), newFrame->dataSize);
//    input.unmap();

//    newFrame->data = std::move(data);
//    dataBuffer_.push_back(std::move(newFrame));
//}

//void VideoController::wakeUp()
//{
//    waitMutex_.lock();
//    hasInput_.wakeOne();
//    waitMutex_.unlock();
//}

//void VideoController::waitForInput()
//{
//    waitMutex_.lock();
//    hasInput_.wait(&waitMutex_);
//    waitMutex_.unlock();
//}
