#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define WIDTH  1920
#define HEIGHT 1080
#define CLAMP(x) ((x) < 0 ? 0 : (x) > 255 ? 255 : (x))

static void yuyv2rgb(unsigned char* yuyv, unsigned char* rgb, int w, int h) {
    for(int i = 0; i < w * h; i += 2) {
        int y0 = yuyv[0], u = yuyv[1], y1 = yuyv[2], v = yuyv[3];
        yuyv += 4;
        int c = y0 - 16, d = u - 128, e = v - 128;
        int r = (298 * c + 409 * e + 128) >> 8;
        int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
        int b = (298 * c + 516 * d + 128) >> 8;
        *rgb++ = CLAMP(r); *rgb++ = CLAMP(g); *rgb++ = CLAMP(b);
        c = y1 - 16;
        r = (298 * c + 409 * e + 128) >> 8;
        g = (298 * c - 100 * d - 208 * e + 128) >> 8;
        b = (298 * c + 516 * d + 128) >> 8;
        *rgb++ = CLAMP(r); *rgb++ = CLAMP(g); *rgb++ = CLAMP(b);
    }
}

static void write_bmp(const char* fn, unsigned char* rgb, int w, int h) {
    FILE* f = fopen(fn, "wb");
    int row_size = (w * 3 + 3) & ~3;
    unsigned char hdr[54] = {0};
    hdr[0] = 0x42; hdr[1] = 0x4D;
    int fsize = 54 + row_size * h;
    memcpy(hdr+2, &fsize, 4);
    hdr[10] = 54; hdr[14] = 40;
    memcpy(hdr+18, &w, 4); memcpy(hdr+22, &h, 4);
    hdr[26] = 1; hdr[28] = 24;
    fwrite(hdr, 1, 54, f);
    unsigned char* row = (unsigned char*)malloc(row_size);
    for(int y = h-1; y >= 0; y--) {
        memcpy(row, rgb + y * w * 3, w * 3);
        fwrite(row, 1, row_size, f);
    }
    free(row); fclose(f);
}

int main(int argc, char* argv[]) {
    const char* out = argc > 1 ? argv[1] : "/tmp/camera.bmp";
    int fd = open("/dev/video-camera0", O_RDWR);
    if(fd < 0) { perror("open"); return 1; }

    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH; fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    if(ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) { perror("fmt"); close(fd); return 1; }

    struct v4l2_requestbuffers req = {0};
    req.count = 1; req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; req.memory = V4L2_MEMORY_MMAP;
    ioctl(fd, VIDIOC_REQBUFS, &req);

    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; buf.memory = V4L2_MEMORY_MMAP; buf.index = 0;
    ioctl(fd, VIDIOC_QUERYBUF, &buf);
    unsigned char* ptr = (unsigned char*)mmap(NULL, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMON, &type);
    ioctl(fd, VIDIOC_QBUF, &buf);
    ioctl(fd, VIDIOC_DQBUF, &buf);

    unsigned char* rgb = (unsigned char*)malloc(WIDTH * HEIGHT * 3);
    yuyv2rgb(ptr, rgb, WIDTH, HEIGHT);
    write_bmp(out, rgb, WIDTH, HEIGHT);

    ioctl(fd, VIDIOC_STREAMOFF, &type);
    munmap(ptr, buf.length); close(fd); free(rgb);
    printf("Captured: %s (%dx%d)\n", out, WIDTH, HEIGHT);
    return 0;
}
