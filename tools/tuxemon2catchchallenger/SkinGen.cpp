#include "SkinGen.hpp"

#include <QImage>
#include <QString>
#include <QDir>
#include <QFileInfo>

namespace tuxemon {

SkinGen::SkinGen(const std::string &modRoot, const std::string &outRoot)
    : modRoot_(modRoot), outRoot_(outRoot), generated_(0)
{
}

static bool alphaBox(const QImage &img, int &x0, int &y0, int &x1, int &y1)
{
    x0 = img.width(); y0 = img.height(); x1 = -1; y1 = -1;
    int y = 0;
    while(y < img.height())
    {
        int x = 0;
        while(x < img.width())
        {
            if(qAlpha(img.pixel(x, y)) > 8)
            {
                if(x < x0) x0 = x; if(y < y0) y0 = y;
                if(x > x1) x1 = x; if(y > y1) y1 = y;
            }
            ++x;
        }
        ++y;
    }
    return x1 >= x0 && y1 >= y0;
}

static QImage cropAlpha(const QImage &in)
{
    int x0, y0, x1, y1;
    if(alphaBox(in, x0, y0, x1, y1))
        return in.copy(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
    return in;
}

// Scale src into a (w,h) transparent canvas keeping aspect, centered.
static QImage fit(const QImage &src, int w, int h)
{
    QImage canvas(w, h, QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    if(src.isNull())
        return canvas;
    QImage s = src.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const int ox = (w - s.width()) / 2, oy = (h - s.height()) / 2;
    int y = 0;
    while(y < s.height())
    {
        int x = 0;
        while(x < s.width()) { canvas.setPixel(ox + x, oy + y, s.pixel(x, y)); ++x; }
        ++y;
    }
    return canvas;
}

// Load a Tuxemon png as ARGB32 (empty image if missing).
static QImage loadArgb(const std::string &path)
{
    QImage img(QString::fromStdString(path));
    if(img.isNull())
        return img;
    if(img.format() != QImage::Format_ARGB32)
        img = img.convertToFormat(QImage::Format_ARGB32);
    return img;
}

bool SkinGen::ensure(const std::string &category, const std::string &skinName,
                     const std::string &overworldSprite, const std::string &battleSheet)
{
    const std::string key = category + "/" + skinName;
    if(done_.find(key) != done_.end())
        return true;
    done_.insert(key);

    // overworld sheet (fallback to adventurer)
    QImage ow = loadArgb(modRoot_ + "/sprites/" + overworldSprite + ".png");
    if(ow.isNull())
        ow = loadArgb(modRoot_ + "/sprites/adventurer.png");
    // battle sheet (fallback to the overworld sheet)
    QImage battle = loadArgb(modRoot_ + "/gfx/sprites/player/" + battleSheet + ".png");
    if(battle.isNull())
        battle = loadArgb(modRoot_ + "/gfx/sprites/player/adventurer.png");
    if(battle.isNull())
        battle = ow;

    const std::string dir = outRoot_ + "/skin/" + category + "/" + skinName;
    QDir().mkpath(QString::fromStdString(dir));
    const QString base = QString::fromStdString(dir) + "/";

    // trainer.png : a single overworld frame (top-left of a 4-col x 2-row grid
    // guess) tiled into the 3x4 (16x24) CatchChallenger walk-sheet.
    QImage owFrame;
    if(!ow.isNull())
    {
        const int cw = ow.width() >= 4 ? ow.width() / 4 : ow.width();
        const int ch = ow.height() >= 2 ? ow.height() / 2 : ow.height();
        owFrame = cropAlpha(ow.copy(0, 0, cw, ch));
    }
    const QImage cell = fit(owFrame, 16, 24);
    QImage trainer(48, 96, QImage::Format_ARGB32);
    trainer.fill(Qt::transparent);
    int r = 0;
    while(r < 4)
    {
        int c = 0;
        while(c < 3)
        {
            int y = 0;
            while(y < 24) { int x = 0; while(x < 16) { trainer.setPixel(c * 16 + x, r * 24 + y, cell.pixel(x, y)); ++x; } ++y; }
            ++c;
        }
        ++r;
    }
    trainer.save(base + "trainer.png", "PNG");
    trainer.save(base + "swim.png", "PNG");

    // front/back : a battle frame fit to 80x80.
    QImage bf = battle;
    if(!battle.isNull() && battle.width() >= battle.height() * 3 / 2)
        bf = battle.copy(0, 0, battle.width() / 2, battle.height());
    bf = cropAlpha(bf);
    const QImage front = fit(bf, 80, 80);
    front.save(base + "front.png", "PNG");
    front.mirrored(true, false).save(base + "back.png", "PNG");

    ++generated_;
    return true;
}

} // namespace tuxemon
