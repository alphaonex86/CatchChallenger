#include "SpriteExtractor.hpp"

#include <QImage>
#include <QString>
#include <QDir>

namespace tuxemon {

SheetRect::SheetRect() : x(0), y(0), w(0), h(0) {}
SheetRect::SheetRect(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}

// Bounding box of pixels with alpha above a small threshold.  Returns false if
// the image is fully transparent.
static bool alphaBounds(const QImage &img, int &x0, int &y0, int &x1, int &y1)
{
    x0 = img.width();
    y0 = img.height();
    x1 = -1;
    y1 = -1;
    int y = 0;
    while(y < img.height())
    {
        int x = 0;
        while(x < img.width())
        {
            if(qAlpha(img.pixel(x, y)) > 8)
            {
                if(x < x0) x0 = x;
                if(y < y0) y0 = y;
                if(x > x1) x1 = x;
                if(y > y1) y1 = y;
            }
            ++x;
        }
        ++y;
    }
    return x1 >= x0 && y1 >= y0;
}

// Cut one region off the sheet (clamped to the sheet bounds), alpha-cropped.
// A region outside the sheet or fully transparent returns a null image.
static QImage cutRegion(const QImage &sheet, const SheetRect &r)
{
    int x = r.x, y = r.y, w = r.w, h = r.h;
    if(x < 0) { w += x; x = 0; }
    if(y < 0) { h += y; y = 0; }
    if(x + w > sheet.width()) w = sheet.width() - x;
    if(y + h > sheet.height()) h = sheet.height() - y;
    if(w <= 0 || h <= 0)
        return QImage();
    QImage part = sheet.copy(x, y, w, h);
    int x0, y0, x1, y1;
    if(!alphaBounds(part, x0, y0, x1, y1))
        return QImage();
    return part.copy(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}

// Place src centered on a transparent (w,h) canvas: pixel art stays CRISP —
// integer x2 upscale (nearest) when it fits, smooth downscale only when too
// big, otherwise unscaled.
static QImage centerCanvas(const QImage &src, int w, int h)
{
    QImage canvas(w, h, QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    if(src.isNull())
        return canvas;
    QImage s = src;
    if(s.width() > w || s.height() > h)
        s = s.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    else if(s.width() * 2 <= w && s.height() * 2 <= h)
        s = s.scaled(s.width() * 2, s.height() * 2, Qt::KeepAspectRatio, Qt::FastTransformation);
    const int ox = (w - s.width()) / 2;
    const int oy = (h - s.height()) / 2;
    int y = 0;
    while(y < s.height())
    {
        int x = 0;
        while(x < s.width())
        {
            canvas.setPixel(ox + x, oy + y, s.pixel(x, y));
            ++x;
        }
        ++y;
    }
    return canvas;
}

bool SpriteExtractor::extract(const std::string &sheetPath, const std::string &outDir,
                              const SheetRect &front, const SheetRect &back,
                              const SheetRect &menu1)
{
    QImage sheet(QString::fromStdString(sheetPath));
    if(sheet.isNull())
        return false;
    if(sheet.format() != QImage::Format_ARGB32)
        sheet = sheet.convertToFormat(QImage::Format_ARGB32);

    QImage frontImg = cutRegion(sheet, front);
    QImage backImg = cutRegion(sheet, back);
    QImage menuImg = cutRegion(sheet, menu1);
    if(frontImg.isNull()) // odd sheet: fall back to the whole image
    {
        SheetRect whole(0, 0, sheet.width(), sheet.height());
        frontImg = cutRegion(sheet, whole);
        if(frontImg.isNull())
            return false;
    }
    if(backImg.isNull())
        backImg = frontImg.mirrored(true, false);
    if(menuImg.isNull())
        menuImg = frontImg;

    QDir().mkpath(QString::fromStdString(outDir));
    const QString base = QString::fromStdString(outDir) + "/";

    bool ok = true;
    ok = centerCanvas(frontImg, 80, 80).save(base + "front.png", "PNG") && ok;
    ok = centerCanvas(backImg, 80, 80).save(base + "back.png", "PNG") && ok;
    // small.png : the menu icon, official datapack size 32x32.
    ok = centerCanvas(menuImg, 32, 32).save(base + "small.png", "PNG") && ok;
    return ok;
}

} // namespace tuxemon
