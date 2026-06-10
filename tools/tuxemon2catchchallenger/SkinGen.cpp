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

    // trainer.png : convert the Tuxemon walk sheet into the CatchChallenger one.
    // Tuxemon (tuxemon/map/view.py): 3 columns = walk1, idle, walk2 and 4 rows =
    // front, left, right, back (frames 16x32 in a 48x128 sheet).  CC
    // (MapVisualiserPlayer, 16x24 cells, idle = middle tile 1/4/7/10): rows =
    // up, right, down, left.  Columns match, only the rows are remapped.  The
    // frames are cropped with ONE shared alpha box (a per-frame box would make
    // the walk cycle wobble) and bottom-anchored in the 16x24 cell.
    QImage trainer(48, 96, QImage::Format_ARGB32);
    trainer.fill(Qt::transparent);
    if(!ow.isNull() && ow.width() >= 3 && ow.height() >= 4)
    {
        const int fw = ow.width() / 3;
        const int fh = ow.height() / 4;
        // shared alpha bounding box over the whole sheet (frame-relative)
        int bx0 = fw, by0 = fh, bx1 = -1, by1 = -1;
        int r = 0;
        while(r < 4)
        {
            int c = 0;
            while(c < 3)
            {
                int x0, y0, x1, y1;
                if(alphaBox(ow.copy(c * fw, r * fh, fw, fh), x0, y0, x1, y1))
                {
                    if(x0 < bx0) bx0 = x0; if(y0 < by0) by0 = y0;
                    if(x1 > bx1) bx1 = x1; if(y1 > by1) by1 = y1;
                }
                ++c;
            }
            ++r;
        }
        if(bx1 < bx0) { bx0 = 0; by0 = 0; bx1 = fw - 1; by1 = fh - 1; }
        // Tuxemon row (front,left,right,back) -> CC row (up,right,down,left)
        const int ccRowOfTux[4] = {2, 3, 1, 0};
        r = 0;
        while(r < 4)
        {
            int c = 0;
            while(c < 3)
            {
                QImage frame = ow.copy(c * fw + bx0, r * fh + by0, bx1 - bx0 + 1, by1 - by0 + 1);
                QImage s = frame;
                if(s.width() > 16 || s.height() > 24)
                    s = s.scaled(16, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                const int ox = (16 - s.width()) / 2;
                const int oy = 24 - s.height(); // feet on the tile
                const int rr = ccRowOfTux[r];
                int y = 0;
                while(y < s.height())
                {
                    int x = 0;
                    while(x < s.width())
                    {
                        trainer.setPixel(c * 16 + ox + x, rr * 24 + oy + y, s.pixel(x, y));
                        ++x;
                    }
                    ++y;
                }
                ++c;
            }
            ++r;
        }
    }
    trainer.save(base + "trainer.png", "PNG");
    trainer.save(base + "swim.png", "PNG");

    // front/back : the Tuxemon combat sheet is two frames side by side,
    // LEFT = back, RIGHT = front (tuxemon/entity/sheet.py CombatSheet).
    QImage frontSrc = battle;
    QImage backSrc = battle;
    if(!battle.isNull() && battle.width() >= battle.height() * 3 / 2)
    {
        backSrc = battle.copy(0, 0, battle.width() / 2, battle.height());
        frontSrc = battle.copy(battle.width() / 2, 0, battle.width() - battle.width() / 2, battle.height());
    }
    fit(cropAlpha(frontSrc), 80, 80).save(base + "front.png", "PNG");
    fit(cropAlpha(backSrc), 80, 80).save(base + "back.png", "PNG");

    ++generated_;
    return true;
}

} // namespace tuxemon
