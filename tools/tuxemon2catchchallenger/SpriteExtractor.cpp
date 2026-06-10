#include "SpriteExtractor.hpp"

#include <QImage>
#include <QString>
#include <QDir>

namespace tuxemon {

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

// Scale src to fit within (maxW,maxH) keeping aspect, centered on a transparent
// canvas of exactly (maxW,maxH).
static QImage fitCanvas(const QImage &src, int maxW, int maxH)
{
    QImage scaled = src.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QImage canvas(maxW, maxH, QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    const int ox = (maxW - scaled.width()) / 2;
    const int oy = (maxH - scaled.height()) / 2;
    int y = 0;
    while(y < scaled.height())
    {
        int x = 0;
        while(x < scaled.width())
        {
            canvas.setPixel(ox + x, oy + y, scaled.pixel(x, y));
            ++x;
        }
        ++y;
    }
    return canvas;
}

bool SpriteExtractor::extract(const std::string &sheetPath, const std::string &outDir)
{
    QImage sheet(QString::fromStdString(sheetPath));
    if(sheet.isNull())
        return false;
    if(sheet.format() != QImage::Format_ARGB32)
        sheet = sheet.convertToFormat(QImage::Format_ARGB32);

    // Tuxemon battle sheets pack 2 columns (idle bob); take the left column as
    // the front pose.  Square / single-column sheets are used whole.
    QImage front;
    if(sheet.width() >= sheet.height() * 3 / 2)
        front = sheet.copy(0, 0, sheet.width() / 2, sheet.height());
    else
        front = sheet;

    // Alpha-crop to the actual sprite.
    int x0, y0, x1, y1;
    if(alphaBounds(front, x0, y0, x1, y1))
        front = front.copy(x0, y0, x1 - x0 + 1, y1 - y0 + 1);

    QDir().mkpath(QString::fromStdString(outDir));
    const QString base = QString::fromStdString(outDir) + "/";

    // front.png : fit into 80x80 (the size the official datapack uses).
    const QImage frontOut = fitCanvas(front, 80, 80);
    // back.png : front mirrored horizontally (Tuxemon has no back sprite).
    const QImage backOut = frontOut.mirrored(true, false);
    // small.png : icon.
    const QImage smallOut = fitCanvas(front, 48, 48);

    bool ok = true;
    ok = frontOut.save(base + "front.png", "PNG") && ok;
    ok = backOut.save(base + "back.png", "PNG") && ok;
    ok = smallOut.save(base + "small.png", "PNG") && ok;
    return ok;
}

} // namespace tuxemon
