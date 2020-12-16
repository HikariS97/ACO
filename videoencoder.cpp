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
        api_->config_parse(config_, "preset", "veryfast");  // TODO

        // TODO：随着传入的改变
        config_->width = 640;
        config_->height = 480;
        config_->framerate_num = 30;
        config_->input_format = KVZ_FORMAT_P400;
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

void VideoEncoder::encodeLoop(VideoInputStream *videoInputStream)
{
    std::cout << "encode thread ID is " << QThread::currentThreadId() << std::endl;

    while (true)
    {
        videoInputStream->bufferMutex_.lock();
        if (!videoInputStream->dataBuffer_.empty())
        {
            auto frame = std::move(videoInputStream->dataBuffer_.front());
            videoInputStream->dataBuffer_.pop_front();
            videoInputStream->bufferMutex_.unlock();
            feedInput(rgb32toyuv400p(std::move(frame)));
        }
        else
        {
            waitDataBuffer_.wait(&(videoInputStream->bufferMutex_));  // 避免 while 消耗太多资源。
            videoInputStream->bufferMutex_.unlock();
        }

    }
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
        // TODO: Alert something.
    }

    // copy input to kvazaar picture
    memcpy(input_pic_->y,
           input->data.get(),
           input->width*input->height);

    //    memcpy(input_pic_->u,
    //           &(input.get()[videodata_->width*videodata_->height]),
    //            videodata_->width*videodata_->height/4);
    //    memcpy(input_pic_->v,
    //           &(input.get()[videodata_->width*videodata_->height + videodata_->width*videodata_->height/4]),
    //            videodata_->width*videodata_->height/4);

    input_pic_->pts = pts_;
    ++pts_;

//    encodingFrames_.push_front(*videodata_);
    if(!api_->encoder_encode(enc_, input_pic_,
                             &data_out, &len_out,
                             &recon_pic, nullptr,
                             &frame_info )) {
        qDebug() << "Encode process failed";
    }

    if(data_out != nullptr)
    {
        qDebug() << "encoded";
        parseEncodedFrame(data_out, len_out, recon_pic);
    }
    else
    {
        // TODO: Something.
    }
}

FILE *output = fopen("output.h265", "wb+");

void VideoEncoder::parseEncodedFrame(kvz_data_chunk *data_out,
                                       uint32_t len_out, kvz_picture *recon_pic)
{
//    auto encodedFrame = encodingFrames_.back();
//    encodingFrames_.pop_back();

    // TODO: This part is to cancel delay. The delay is caused MAYBE by encoding deque.
    // Note: Convert to Microsecond.
    //    uint32_t delay = QDateTime::currentMSecsSinceEpoch() -
    //            (encodedFrame->presentationTime.tv_sec * 1000 + encodedFrame->presentationTime.tv_usec/1000);
    //    getStats()->sendDelay("video", delay);
    //    getStats()->addEncodedPacket("video", len_out);

    // Free the memory before a new set.
//    delete [] videodata_->data_;
//    videodata_->data_ = new uint8_t[len_out];
//    uint8_t* writer = videodata_->data_;
//    videodata_->data_size_ = 0;
int i = 0;
    for (kvz_data_chunk *chunk = data_out; chunk != nullptr; chunk = chunk->next)
    {
        // 这个地方先观察是否与存HEVC至本地有关。
        //        if(chunk->len > 3 && chunk->data[0] == 0 && chunk->data[1] == 0
        //                &&( chunk->data[2] == 1 || (chunk->data[2] == 0 && chunk->data[3] == 1) )
        //                && dataWritten != 0 && config_->slices != KVZ_SLICES_NONE)
        //        {
        //            // send previous packet if this is not the first

        //            // TODO: put delayes into deque, and set timestamp accordingly to get more accurate latency.
        //            std::unique_ptr<Data> slice(shallowDataCopy(encodedFrame.get()));
        //            sendEncodedFrame(std::move(slice), std::move(hevc_frame), dataWritten);

        //            hevc_frame = std::unique_ptr<uint8_t[]>(new uint8_t[len_out - dataWritten]);
        //            writer = hevc_frame.get();
        //            dataWritten = 0;
        //        }
        fwrite(chunk->data, sizeof (chunk->data[0]), chunk->len, output);
        i++;
//        memcpy(writer, chunk->data, chunk->len);
//        writer += chunk->len;
//        videodata_->data_size_ += chunk->len;
    }

    api_->chunk_free(data_out);
    api_->picture_free(recon_pic);

    std::cout << "The value of i is: " << i << std::endl;

    // send last packet reusing input structure
    //    sendEncodedFrame(std::move(encodedFrame), std::move(hevc_frame), dataWritten);
}

////void VideoEncoder::sendEncodedFrame(std::unique_ptr<uint8_t[]> input,
////                                      std::unique_ptr<uint8_t[]> hevc_frame, uint32_t dataWritten)
////{
////    input->data_size = dataWritten;
////    input->data = std::move(hevc_frame);
////    sendOutput(std::move(input));
////}
