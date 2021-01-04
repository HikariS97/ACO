#include <QtDebug>
#include <QThread>
#include <QTime>
#include <QSize>
#include <iostream>

#include "videoinputstream.h"
#include "videoencoder.h"
#include "convertcolorspace.h"
#include "kvazaar.h"

enum RETURN_STATUS {C_SUCCESS = 0, C_FAILURE = -1};

VideoEncoder::VideoEncoder():
    isrunning_(true),
    api_(nullptr),
    config_(nullptr),
    enc_(nullptr),
    pts_(0),
    input_pic_(nullptr),
    framerate_num_(30),
    framerate_denom_(1)
  //    encodingFrames_()
{
    maxBufferSize_ = 3;
}

VideoEncoder::~VideoEncoder()
{
}

bool VideoEncoder::init()
{
    // input picture should not exist at this point
    if(!input_pic_ && !api_)
    {

        api_ = kvz_api_get(8);
        if(!api_)
        {
            fprintf(stderr, "Failed to get kvz_api.");
            return false;
        }
        config_ = api_->config_alloc();
        enc_ = nullptr;

        if(!config_)
        {
            fprintf(stderr, "Failed to allocate Kvazaar config.");
            return false;
        }

        api_->config_init(config_);
        api_->config_parse(config_, "preset", "ultrafast");  // TODO

        // TODO：随着传入的改变
        config_->width = 640;
        config_->height = 480;
        config_->framerate_num = 30;
        config_->input_format = KVZ_FORMAT_P420;
        config_->threads = 4;
        config_->qp = 21;
        config_->framerate_denom = 1;
        config_->hash = KVZ_HASH_NONE;

        // this does not work with kvzRTP at the moment. Avoid using slices.
        api_->config_parse(config_, "tiles", "2x2");
        config_->slices = KVZ_SLICES_TILES;

        // TODO: Maybe send parameter sets only when needed
        // config_->target_bitrate = target_bitrate;

        enc_ = api_->encoder_open(config_);

        if(!enc_)
        {
            fprintf(stderr, "Failed to open Kvazaar encoder.");
            return false;
        }

        input_pic_ = api_->picture_alloc(config_->width, config_->height);

        if(!input_pic_)
        {
            fprintf(stderr, "Could not allocate input picture.");
            return false;
        }

        qDebug() << "Kvazaar iniation succeeded.";
    }
    return true;
}
void VideoEncoder::close()
{
    while (feedInput()) {}

    if(api_)
    {
        api_->encoder_close(enc_);
        api_->config_destroy(config_);
        enc_ = nullptr;
        config_ = nullptr;

        api_->picture_free(input_pic_);
        input_pic_ = nullptr;
        api_ = nullptr;
    }
    qDebug() << "Kvazaar closed";

    pts_ = 0;
}


void VideoEncoder::feedInput(std::unique_ptr<Data> input)
{
    if(!input_pic_)
    {
        fprintf(stderr, "Input picture was not allocated correctly.");
    }
    kvz_picture *recon_pic = nullptr;
    kvz_frame_info frame_info;
    kvz_data_chunk *data_out = nullptr;
    uint32_t len_out = 0;

    if(config_->width != input->width
            || config_->height != input->height
            || config_->framerate_num != input->framerate)
    {
        qFatal("Input size and encoder's config size not match.");
    }

    // copy input to kvazaar picture

    memcpy(input_pic_->y,
           input->data.get(),
           input->width*input->height);

    memcpy(input_pic_->u,
           &(input->data.get()[input->width*input->height]),
            input->width*input->height/4);
    memcpy(input_pic_->v,
           &(input->data.get()[input->width*input->height + input->width*input->height/4]),
            input->width*input->height/4);

    input_pic_->pts = pts_;
    ++pts_;

    encodingFrames_.push_front(std::move(input));

    api_->encoder_encode(enc_, input_pic_,
                         &data_out, &len_out,
                         &recon_pic, nullptr,
                         &frame_info );

    if(data_out != nullptr)
    {
        parseEncodedFrame(data_out, len_out, recon_pic);
    }
}
bool VideoEncoder::feedInput()
{
    // 代码重复。但是我不想改了。
    if(!input_pic_)
    {
        fprintf(stderr, "Input picture was not allocated correctly.");
    }
    kvz_picture *recon_pic = nullptr;
    kvz_frame_info frame_info;
    kvz_data_chunk *data_out = nullptr;
    uint32_t len_out = 0;

    input_pic_->y = nullptr;
    input_pic_->u = nullptr;
    input_pic_->v = nullptr;

    input_pic_->pts = pts_;
    ++pts_;

    api_->encoder_encode(enc_, input_pic_,
                         &data_out, &len_out,
                         &recon_pic, nullptr,
                         &frame_info );

    if(data_out != nullptr)
    {
        parseEncodedFrame(data_out, len_out, recon_pic);
        return true;
    }
    else {
        return false;
    }
}

FILE *output = fopen("output.h265", "wb+");  // TODO
void VideoEncoder::parseEncodedFrame(kvz_data_chunk *data_out,
                                     uint32_t len_out, kvz_picture *recon_pic)
{
    //    if (encodingFrames_.empty()) {
    //        auto encodedFrame = std::move(encodingFrames_.back());
    //        encodingFrames_.pop_back();
    //    }

    std::unique_ptr<uchar[]> hevc_frame(new uchar[len_out]);
    uint8_t* writer = hevc_frame.get();
    uint32_t dataWritten = 0;

    for (kvz_data_chunk *chunk = data_out; chunk != nullptr; chunk = chunk->next)
    {

        //        if(chunk->len > 3 && chunk->data[0] == 0 && chunk->data[1] == 0
        //                &&( chunk->data[2] == 1 || (chunk->data[2] == 0 && chunk->data[3] == 1) )
        //                && dataWritten != 0 && config_->slices != KVZ_SLICES_NONE)
        //        {
        //            std::unique_ptr<Data> slice = std::move(encodedFrame);
        //            fwrite(hevc_frame.get(), sizeof (uchar), dataWritten, output);

        //            hevc_frame = std::unique_ptr<uint8_t[]>(new uint8_t[len_out - dataWritten]);
        //            writer = hevc_frame.get();
        //            dataWritten = 0;
        //        }

        memcpy(writer, chunk->data, chunk->len);
        writer += chunk->len;
        dataWritten += chunk->len;
    }

    api_->chunk_free(data_out);
    api_->picture_free(recon_pic);

    fwrite(hevc_frame.get(), sizeof (uchar), dataWritten, output);
}

void VideoEncoder::close_proc()
{
    if (isrunning_) {
        close();
        isrunning_ = false;
    }
    else {
        qWarning("Logic Error! closing Encoder when it's closed.");
    }
}

void VideoEncoder::receive_frame(const QVideoFrame &frame)
{
    pushInBuffer(frame);
    encode();
}
void VideoEncoder::pushInBuffer(const QVideoFrame &frame)
{
    auto tmp_frame = static_cast<QVideoFrame>(frame);
    tmp_frame.map(QAbstractVideoBuffer::ReadOnly);
    QVideoFrame input = QVideoFrame(QImage(tmp_frame.bits(),            \
                                           tmp_frame.width(),           \
                                           tmp_frame.height(),          \
                                           tmp_frame.bytesPerLine(),    \
                                           QVideoFrame::imageFormatFromPixelFormat(tmp_frame.pixelFormat())\
                                           ).convertToFormat(QImage::Format_RGBX8888));
    tmp_frame.unmap();
    if (!input.isValid()) {
        qFatal("Input frame's format error.");
        abort();
    }

    // Copy.
    std::unique_ptr<Data> newFrame(new Data);
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

    // Push into buffer.
    bufferMutex_.lock();
    dataBuffer_.push_back(std::move(newFrame));
    bufferMutex_.unlock();
}
void VideoEncoder::encode()
{
    bufferMutex_.lock();
    auto frame = std::move(dataBuffer_.front());
    dataBuffer_.pop_front();
    bufferMutex_.unlock();
    feedInput(rgb32toyuv420p(std::move(frame)));  // TODO
}

// NOTE：应该用不到，所有的 stop 都应该处理完 buffer 里的数据了。
void VideoEncoder::emptyBuffer()
{
    bufferMutex_.lock();
    std::deque<std::unique_ptr<Data>> tmp;
    std::swap(dataBuffer_, tmp);
    bufferMutex_.unlock();
}
