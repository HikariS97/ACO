#ifndef VIDEOSURFACE_H
#define VIDEOSURFACE_H

#include <QAbstractVideoSurface>


class VideoSurface : public  QAbstractVideoSurface
{
    Q_OBJECT
public:
    VideoSurface();

    bool present(const QVideoFrame &frame) override;
    QList<QVideoFrame::PixelFormat> supportedPixelFormats
    (QAbstractVideoBuffer::HandleType handleType) const override;

signals:
    void frameAvailable(const QVideoFrame &frame);
};

#endif // VIDEOSURFACE_H
