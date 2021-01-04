#include "convertcolorspace.h"

#include "videoinputstream.h"  // 主要是为了 Data


/* 以下两个都是内存安全的 */
template <typename T>
static inline T clamp(T x)
{
    if (x < 0)
        x = 0;
    else if (x > 255)
        x = 255;
    return x;
}

std::unique_ptr<Data> rgb32toyuv400p(std::unique_ptr<Data> frame)
{
    Q_ASSERT(frame->dataSize % 4 == 0);

    uint32_t inputDataSize = frame->dataSize;
    uint32_t outputDataSize = inputDataSize / 4;

    uint8_t *Y = new uint8_t[outputDataSize];

    // Main loop
    for(uint i = 0; i < inputDataSize; i += 4)
    {
        int32_t pixel = 0.299*frame->data[i] + 0.587 * frame->data[i+1]
                + 0.114 * frame->data[i+2];
        Y[outputDataSize - i/4 - 1] = uint8_t(clamp(pixel));
    }

    frame->data.reset(Y);
    frame->dataSize = outputDataSize;

    return frame;
}
std::unique_ptr<Data> rgb32toyuv420p(std::unique_ptr<Data> frame)
{
    Q_ASSERT(frame->dataSize % 8 == 0);  // x/4*1.5 should be int.

    uint32_t inputDataSize = frame->dataSize;
    uint32_t outputDataSize = static_cast<uint32_t>(inputDataSize/4*1.5);

    uint8_t* Y = new uint8_t[outputDataSize];
    uint8_t* U = &(Y[frame->width*frame->height]);
    uint8_t* V = &(Y[frame->width*frame->height + frame->width*frame->height/4]);

    // Main loop
    for (int row = 0; row < frame->height; row = row+2) {
        for (int col = 0; col < frame->bytesPerLine; col = col+8) {
            int32_t y_pixel = 0;
            int32_t u_pixel = 0;
            int32_t v_pixel = 0;

            y_pixel = 0.299*frame->data[row*frame->bytesPerLine+col] + 0.587 * frame->data[row*frame->bytesPerLine+col+1]
                    + 0.114 * frame->data[row*frame->bytesPerLine+col+2];
            u_pixel += (frame->data[row*frame->bytesPerLine+col+2] - y_pixel)*0.565;
            v_pixel += (frame->data[row*frame->bytesPerLine+col] - y_pixel)*0.713;
            Y[int((frame->height-row)*frame->width-col/4-1)] = uint8_t(clamp(y_pixel));

            y_pixel = 0.299*frame->data[row*frame->bytesPerLine+col+4] + 0.587 * frame->data[row*frame->bytesPerLine+col+1+4]
                    + 0.114 * frame->data[row*frame->bytesPerLine+col+2+4];
            u_pixel += (frame->data[row*frame->bytesPerLine+col+2+4] - y_pixel)*0.565;
            v_pixel += (frame->data[row*frame->bytesPerLine+col+4] - y_pixel)*0.713;
            Y[int((frame->height-row)*frame->width-col/4-1-1)] = uint8_t(clamp(y_pixel));

            y_pixel = 0.299*frame->data[(row+1)*frame->bytesPerLine+col] + 0.587 * frame->data[(row+1)*frame->bytesPerLine+col+1]
                    + 0.114 * frame->data[(row+1)*frame->bytesPerLine+col+2];
            u_pixel += (frame->data[(row+1)*frame->bytesPerLine+col+2] - y_pixel)*0.565;
            v_pixel += (frame->data[(row+1)*frame->bytesPerLine+col] - y_pixel)*0.713;
            Y[int((frame->height-row-1)*frame->width-col/4-1)] = uint8_t(clamp(y_pixel));

            y_pixel = 0.299*frame->data[(row+1)*frame->bytesPerLine+col+4] + 0.587 * frame->data[(row+1)*frame->bytesPerLine+col+1+4]
                    + 0.114 * frame->data[(row+1)*frame->bytesPerLine+col+2+4];
            u_pixel += (frame->data[(row+1)*frame->bytesPerLine+col+2+4] - y_pixel)*0.565;
            v_pixel += (frame->data[(row+1)*frame->bytesPerLine+col+4] - y_pixel)*0.713;
            Y[int((frame->height-row-1)*frame->width-col/4-1-1)] = uint8_t(clamp(y_pixel));

            U[(frame->height-row)/2*frame->width/2-col/8-1] = uint8_t(clamp(u_pixel/4+128));
            V[(frame->height-row)/2*frame->width/2-col/8-1] = uint8_t(clamp(v_pixel/4+128));
        }
    }

    frame->data.reset(Y);
    frame->dataSize = outputDataSize;

    return frame;
}
