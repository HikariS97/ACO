// 该类独立于主线程
// 该类中除了显示部分以外的，比如说：
//
// 编码
// 保存
//
// 都分出至另一个线程


#ifndef VIDEOINPUTSTREAM_H
#define VIDEOINPUTSTREAM_H

#include <QWidget>
#include <QCamera>
#include <QWaitCondition>
#include <QMutex>
#include <QVideoFrame>
#include <memory>
#include <deque>

#include "videosurface.h"


class VideoEncoder;
// TODO: Move to abstract class.
struct Data
{
  std::unique_ptr<uchar[]> data;
  int dataSize;
  int width;
  int height;
  int bytesPerLine;
  int framerate;
};


class VideoInputStream : public QWidget
{
    Q_OBJECT;

public:
    explicit VideoInputStream(QWidget *parent = nullptr);

    void init();
    void uninit();

    // 需要开放给其它的，进行处理的线程使用。
    std::deque<std::unique_ptr<Data>> dataBuffer_;
    QMutex bufferMutex_;
protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void startEncodeLoop(VideoInputStream *videoInputStream);

private slots:
    void pushInBuffer();
    void frameHandler(const QVideoFrame &frame);
    void initEncoder();

private:
    void emptyBuffer();

    QCamera *camera_;
    VideoSurface *videoSurface_;
    VideoEncoder *videoEncoder_;

    bool onlyDisplay_ = false;  // TODO
    QVideoFrame currFrame_;
    QThread *encoderThread_;
};

#endif // VIDEOINPUTSTREAM_H
