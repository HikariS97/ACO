#ifndef VIDEOCONTROLLER_H
#define VIDEOCONTROLLER_H

#include <QWaitCondition>
#include <QMutex>
#include <QThread>
#include <QVideoFrame>
#include <memory>
#include <deque>

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

class VideoController : public QThread
{
    Q_OBJECT;

public:
    VideoController();
    ~VideoController();

private slots:
    void push_in_buffer(const QVideoFrame &frame);

private:
    void run() override;
    void process();
    void wakeUp();
    void waitForInput();
    std::unique_ptr<Data> getInput();

    bool running_ = true;  // TODO
    std::deque<std::unique_ptr<Data>> dataBuffer_;
    QWaitCondition hasInput_;
    QMutex bufferMutex_;
    QMutex waitMutex_;
};

#endif // VIDEOCONTROLLER_H
