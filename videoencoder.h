#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <QObject>
#include <stdint.h>
#include <deque>
#include <memory>
#include <QVideoFrame>

#include <QMutex>


struct kvz_api;
struct kvz_config;
struct kvz_encoder;
struct kvz_picture;
struct kvz_data_chunk;

struct Data;
class VideoInputStream;
class VideoEncoder : public QObject
{
    Q_OBJECT;

public:
    VideoEncoder();
    ~VideoEncoder();

    bool init();
    void close();

private slots:
    void close_proc();
    void receive_frame(const QVideoFrame &frame);

private:
    void pushInBuffer(const QVideoFrame &frame);
    void emptyBuffer();
    void encode();

    void feedInput(std::unique_ptr<Data> input);
    bool feedInput();

    void parseEncodedFrame(kvz_data_chunk *data_out, uint32_t len_out,
                           kvz_picture *recon_pic);

    bool isrunning_;

    int maxBufferSize_;

    const kvz_api *api_;
    kvz_config *config_;
    kvz_encoder *enc_;

    int64_t pts_;

    kvz_picture *input_pic_;

    int32_t framerate_num_;
    int32_t framerate_denom_;

    std::deque<std::unique_ptr<Data>> encodingFrames_;

    std::deque<std::unique_ptr<Data>> dataBuffer_;
    QMutex bufferMutex_;
};

#endif // VIDEOENCODER_H
