#include "videosurface.h"

#include <iostream>

VideoSurface::VideoSurface()
{
}

bool VideoSurface::present(const QVideoFrame &frame)
{
    if (frame.isValid()) {
        emit frameAvailable(frame);
        return true;
    }
    std::cerr << "Not a valid frame" << std::endl;
    return false;
}

QList<QVideoFrame::PixelFormat> VideoSurface::supportedPixelFormats
(QAbstractVideoBuffer::HandleType handleType) const
{
  Q_UNUSED(handleType);
  return QList<QVideoFrame::PixelFormat>()
      << QVideoFrame::Format_YUV420P
      << QVideoFrame::Format_YUYV
      << QVideoFrame::Format_NV12
      << QVideoFrame::Format_YV12
      << QVideoFrame::Format_RGB32;
}
