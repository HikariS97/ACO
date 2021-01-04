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
    ~VideoInputStream();

    void init();
    void uninit();

    void start();
    void stop();

    void add_proc();
    void delete_proc();

    bool isrunning;  // TODO：设置友元。

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void close_proc();  // NOTE: Block Connection.
    void transfer_frame(const QVideoFrame &frame);

private slots:
    void handle_frame(const QVideoFrame &frame);

public slots:
    void test_print();

private:
    QCamera *camera_;
    VideoSurface *videoSurface_;
    QVideoFrame currFrame_;

    /* Current Processors List:
        1. videoEncoder;
        2. ?
    */
    QMutex numProcMutex_;
    int numProc_;
};

#endif // VIDEOINPUTSTREAM_H
