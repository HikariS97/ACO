#include "videocontroller.h"
#include <QDebug>
#include <iostream>


template <typename T>
static inline T clamp(T x)
{
    if (x < 0)
        x = 0;
    else if (x > 255)
        x = 255;
    return x;
}

static std::unique_ptr<Data> rgb32toyuv400p(std::unique_ptr<Data> data);
static std::unique_ptr<Data> rgb32toyuv420p(std::unique_ptr<Data> data);

VideoController::VideoController()
{

}

VideoController::~VideoController()
{

}

void VideoController::run()
{
    while(running_)
    {
        waitForInput();
        if (!running_) break;
        process();
    }
}

static FILE *output = fopen("640x480_30fps_YUV400P.yuv", "wb+");
void VideoController::process()
{
    std::unique_ptr<Data> input = getInput();

    while (input)
    {
        std::cout << "." << std::flush;
        std::unique_ptr<Data> convertedInput = rgb32toyuv420p(std::move(input));
        fwrite(convertedInput->data.get(), sizeof (convertedInput->data[0]), convertedInput->dataSize, output);
        input = getInput();
    }
}

std::unique_ptr<Data> VideoController::getInput()
{
    bufferMutex_.lock();
    std::unique_ptr<Data> r;
    if (!dataBuffer_.empty())
    {
        r = std::move(dataBuffer_.front());
        dataBuffer_.pop_front();
    }
    bufferMutex_.unlock();
    return r;
}

void VideoController::push_in_buffer(const QVideoFrame &frame)
{  // in Main thread.
    wakeUp();

    QVideoFrame frame_tmp = frame;
    frame_tmp.map(QAbstractVideoBuffer::ReadOnly);
    std::unique_ptr<Data> newFrame(new Data);
    QVideoFrame input = QVideoFrame(QImage(frame_tmp.bits(),            \
                                                frame_tmp.width(),           \
                                                frame_tmp.height(),          \
                                                frame_tmp.bytesPerLine(),    \
                                                QVideoFrame::imageFormatFromPixelFormat(frame_tmp.pixelFormat())\
                                                ).convertToFormat(QImage::Format_RGBX8888));
    frame_tmp.unmap();
    input.map(QAbstractVideoBuffer::ReadOnly);
    qDebug() << input.pixelFormat();
    qDebug() << int(input.bits()[0]);
    qDebug() << int(input.bits()[1]);
    qDebug() << int(input.bits()[2]);
    qDebug() << int(input.bits()[3]);
    newFrame->width = input.width();
    newFrame->height = input.height();
    newFrame->dataSize = input.mappedBytes();
    newFrame->framerate = 30;  // TODO: 这怎么算来着？
    newFrame->bytesPerLine = input.bytesPerLine();

    std::unique_ptr<uchar[]> data(new uchar[newFrame->dataSize]);
    memcpy(data.get(), input.bits(), newFrame->dataSize);
    input.unmap();

    newFrame->data = std::move(data);
    dataBuffer_.push_back(std::move(newFrame));
}

void VideoController::wakeUp()
{
    waitMutex_.lock();
    hasInput_.wakeOne();
    waitMutex_.unlock();
}

void VideoController::waitForInput()
{
    waitMutex_.lock();
    hasInput_.wait(&waitMutex_);
    waitMutex_.unlock();
}

static std::unique_ptr<Data> rgb32toyuv400p(std::unique_ptr<Data> data)
{
    Q_ASSERT(data->dataSize % 4 == 0);

    uint32_t inputDataSize = data->dataSize;
    uint32_t outputDataSize = inputDataSize / 4;

    uint8_t *Y = new uint8_t[outputDataSize];

    // Main loop
    for(uint i = 0; i < inputDataSize; i += 4)
    {
        int32_t pixel = 0.299*data->data[i] + 0.587 * data->data[i+1]
                + 0.114 * data->data[i+2];
        Y[outputDataSize - i/4 - 1] = uint8_t(clamp(pixel));
    }

    data->data.reset(Y);
    data->dataSize = outputDataSize;

    return data;
}

static std::unique_ptr<Data> rgb32toyuv420p(std::unique_ptr<Data> data)
{
    Q_ASSERT(data->dataSize % 8 == 0);  // x/4*1.5 should be int.

    uint32_t inputDataSize = data->dataSize;
    uint32_t outputDataSize = static_cast<uint32_t>(inputDataSize/4*1.5);

    uint8_t* Y = new uint8_t[outputDataSize];
    uint8_t* U = &(Y[data->width*data->height]);
    uint8_t* V = &(Y[data->width*data->height + data->width*data->height/4]);

    // Main loop
    for (int row = 0; row < data->height; row = row+2) {
        for (int col = 0; col < data->bytesPerLine; col = col+8) {
            int32_t y_pixel = 0;
            int32_t u_pixel = 0;
            int32_t v_pixel = 0;

            y_pixel = 0.299*data->data[row*data->bytesPerLine+col] + 0.587 * data->data[row*data->bytesPerLine+col+1]
                    + 0.114 * data->data[row*data->bytesPerLine+col+2];
            u_pixel += (data->data[row*data->bytesPerLine+col+2] - y_pixel)*0.565;
            v_pixel += (data->data[row*data->bytesPerLine+col] - y_pixel)*0.713;
            Y[int((data->height-row)*data->width-col/4-1)] = uint8_t(clamp(y_pixel));

            y_pixel = 0.299*data->data[row*data->bytesPerLine+col+4] + 0.587 * data->data[row*data->bytesPerLine+col+1+4]
                    + 0.114 * data->data[row*data->bytesPerLine+col+2+4];
            u_pixel += (data->data[row*data->bytesPerLine+col+2+4] - y_pixel)*0.565;
            v_pixel += (data->data[row*data->bytesPerLine+col+4] - y_pixel)*0.713;
            Y[int((data->height-row)*data->width-col/4-1-1)] = uint8_t(clamp(y_pixel));

            y_pixel = 0.299*data->data[(row+1)*data->bytesPerLine+col] + 0.587 * data->data[(row+1)*data->bytesPerLine+col+1]
                    + 0.114 * data->data[(row+1)*data->bytesPerLine+col+2];
            u_pixel += (data->data[(row+1)*data->bytesPerLine+col+2] - y_pixel)*0.565;
            v_pixel += (data->data[(row+1)*data->bytesPerLine+col] - y_pixel)*0.713;
            Y[int((data->height-row-1)*data->width-col/4-1)] = uint8_t(clamp(y_pixel));

            y_pixel = 0.299*data->data[(row+1)*data->bytesPerLine+col+4] + 0.587 * data->data[(row+1)*data->bytesPerLine+col+1+4]
                    + 0.114 * data->data[(row+1)*data->bytesPerLine+col+2+4];
            u_pixel += (data->data[(row+1)*data->bytesPerLine+col+2+4] - y_pixel)*0.565;
            v_pixel += (data->data[(row+1)*data->bytesPerLine+col+4] - y_pixel)*0.713;
            Y[int((data->height-row-1)*data->width-col/4-1-1)] = uint8_t(clamp(y_pixel));

            U[(data->height-row)/2*data->width/2-col/8-1] = uint8_t(clamp(u_pixel/4+128));
            V[(data->height-row)/2*data->width/2-col/8-1] = uint8_t(clamp(v_pixel/4+128));
        }
    }

    data->data.reset(Y);
    data->dataSize = outputDataSize;

    return data;

}
