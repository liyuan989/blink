#ifndef __EXAMPLE_PROCMON_PLOT_H__
#define __EXAMPLE_PROCMON_PLOT_H__

#include <blink/Nocopyable.h>
#include <blink/Types.h>

#include <sys/types.h>  // ssize_t

#include <vector>

typedef struct gdImageStruct* gdImagePtr;

class Plot : blink::Nocopyable
{
public:
    Plot(int width, int height, int total_seconds, int sampling_period);
    ~Plot();

    blink::string plotCpu(const std::vector<double>& data);

private:
    blink::string toPng();
    int getX(ssize_t i, ssize_t total) const;
    int getY(double value) const;
    void label(double max_value) const;

    // gdFont is a typedef of unnamed struct, cannot be foward declered
    // wordaround suggested int http://stackoverflow.com/questions/7256436/forward-declarations-of-unnamed-struct
    struct MyGdFont;
    typedef struct MyGdFont* MyGdFontPtr;

    const int          width_;
    const int          height_;
    const int          total_seconds_;
    const int          sampling_period_;
    const gdImagePtr   image_;
    const MyGdFontPtr  font_;
    const int          font_width_;
    const int          font_height_;
    const int          background_;
    const int          black_;
    const int          gray_;
    const int          blue_;
    const int          kRightMargin_;

    static const int kLeftMargin_ = 5;
    static const int kMarginY_ = 5;

    const double       ratio_x_;
};

#endif
