#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <QObject>
#include <stdint.h>
#include <deque>
#include <memory>
#include <QWaitCondition>


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
  bool init();
  void close();

  // copy the frame data to kvazaar input in suitable format.
  void feedInput(std::unique_ptr<Data> input);

  QWaitCondition waitDataBuffer_;  // TODO：以后写成函数调用的形式，写成模板类

public slots:
  void encodeLoop(VideoInputStream *videoInputstream);

private:
  // parse the encoded frame and send it forward.
  void parseEncodedFrame(kvz_data_chunk *data_out, uint32_t len_out,
                         kvz_picture *recon_pic);


  int maxBufferSize_;

  const kvz_api *api_;
  kvz_config *config_;
  kvz_encoder *enc_;

  int64_t pts_;

  kvz_picture *input_pic_;

  int32_t framerate_num_;
  int32_t framerate_denom_;

  std::deque<std::unique_ptr<Data>> encodingFrames_;
};

#endif // VIDEOENCODER_H
