#include <example/procmon/plot.h>

#include <gd.h>
#include <gdfonts.h>

#include <algorithm>
#include <math.h>

using namespace blink;

struct Plot::MyGdFont : public gdFont
{
};

const int Plot::kLeftMargin_;
const int Plot::kMarginY_;

Plot::Plot(int width, int height, int total_seconds, int sampling_period)
    : width_(width),
      height_(height),
      total_seconds_(total_seconds),
      sampling_period_(sampling_period),
      image_(gdImageCreate(width, height)),
      font_(static_cast<MyGdFont*>(gdFontGetSmall())),
      font_width_(font_->w),
      font_height_(font_->h),
      background_(gdImageColorAllocate(image_, 255, 255, 240)),
      black_(gdImageColorAllocate(image_, 0, 0, 0)),
      gray_(gdImageColorAllocate(image_, 200, 200, 200)),
      blue_(gdImageColorAllocate(image_, 128, 128, 255)),
      kRightMargin_(3 * font_width_ + 5),
      ratio_x_(static_cast<double>(sampling_period * (width - kLeftMargin_ - kRightMargin_)) / total_seconds)
{
    //gdImageSetAntiAliased(image_, black_);
}

Plot::~Plot()
{
    gdImageDestroy(image_);
}

string Plot::plotCpu(const std::vector<double>& data)
{
    gdImageFilledRectangle(image_, 0, 0, width_, height_, background_);
    if (data.size() > 1)
    {
        gdImageSetThickness(image_, 2);
        double max = *std::max_element(data.begin(), data.end());
        if (max >= 10.0)
        {
            max = ceil(max);
        }
        else
        {
            max = std::max(0.1, ceil(max * 10.0) / 10.0);
        }
        label(max);

        for (size_t i = 0; i < data.size() - 1; ++i)
        {
            gdImageLine(image_,
                        getX(i, data.size()),
                        getY(data[i] / max),
                        getX(i + 1, data.size()),
                        getY(data[i + 1] / max),
                        black_);
        }
    }

    int total = total_seconds_ / sampling_period_;
    gdImageSetThickness(image_, 1);
    gdImageLine(image_, getX(0, total), getY(0) + 2, getX(total, total), getY(0) + 2, gray_);
    gdImageLine(image_, getX(total, total), getY(0) + 2, getX(total, total), getY(1) + 2, gray_);
    return toPng();
}

void Plot::label(double max_value) const
{
    char buf[64];
    if (max_value >= 10.0)
    {
        snprintf(buf, sizeof(buf), "%.0f", max_value);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%.1f", max_value);
    }

    gdImageString(image_,
                  font_,
                  width_ - kRightMargin_ + 3,
                  kMarginY_ - 3,
                  reinterpret_cast<unsigned char*>(buf),
                  black_);

    snprintf(buf, sizeof(buf), "0");
    gdImageString(image_,
                  font_,
                  width_ - kRightMargin_ + 3,
                  height_ - kMarginY_ - 3 - font_height_ / 2,
                  reinterpret_cast<unsigned char*>(buf),
                  gray_);

    snprintf(buf, sizeof(buf), "-%ds", total_seconds_);
    gdImageString(image_,
                  font_,
                  kLeftMargin_,
                  height_ - kMarginY_ - font_height_,
                  reinterpret_cast<unsigned char*>(buf),
                  blue_);
}

int Plot::getX(ssize_t i, ssize_t total) const
{
    double x = (width_ - kLeftMargin_ - kRightMargin_) + static_cast<double>(i - total) * ratio_x_;
    return static_cast<int>(x + 0.5) + kLeftMargin_;
}

int Plot::getY(double value) const
{
    return static_cast<int>((1.0 - value) * (height_ - 2 * kMarginY_) + 0.5) + kMarginY_;
}

string Plot::toPng()
{
    int size = 0;
    void* png = gdImagePngPtr(image_, &size);
    string result(static_cast<char*>(png), size);
    gdFree(png);
    return result;
}
