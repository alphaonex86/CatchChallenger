// tuxemon2catchchallenger — convert the Tuxemon game data into a CatchChallenger
// datapack.  Usage:
//   tuxemon2catchchallenger <mods/tuxemon dir> <output datapack dir>
//
// Reads the Tuxemon YAML db + sprites + gettext catalogues and writes the
// CatchChallenger types/skills/buffs/monsters/items XML and monster sprites.

#include "TuxemonDb.hpp"
#include "Localization.hpp"
#include "DatapackWriter.hpp"
#include "MapConverter.hpp"
#include "SkinGen.hpp"
#include "WorldWriter.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QThread>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// One PNG's optimisation pipeline: an ordered list of `nice` argv to run on the
// SAME file (pngquant then zopflipng), `step` = which one is running, `proc` =
// its child (NULL between steps).  Steps run STRICTLY in order: a file's
// zopflipng only starts once that same file's pngquant has finished.
struct PngJob
{
    std::vector<QStringList> steps;
    size_t step;
    QProcess *proc;
};

// A PNG path paired with its byte size, so jobs can be ordered largest-first.
struct PngBySize
{
    QString path;
    qint64 size;
};

// Descending by size: biggest (slowest to compress) files start first, so they
// don't linger as a serial tail when the pool drains at the end.
static bool pngLargerFirst(const PngBySize &a, const PngBySize &b)
{
    return a.size > b.size;
}

// Run every job's steps to completion, keeping up to `slotCount` files in flight so
// all CPU cores are used (pngquant/zopflipng are single-threaded per file — the
// parallelism is ACROSS files; the two steps WITHIN a file stay sequential).  No
// lambdas/signals/event-loop: poll every in-flight child with a NON-BLOCKING
// waitForFinished(0) — this reaps a just-exited child (QProcess::state() alone
// would never leave Running without an event loop, leaving zombies and zero
// progress).  Advance each finished one to its next step (or retire it) and refill
// the pool IMMEDIATELY, so a slot freed by a quick small file is reused at once
// even while a slow large tileset still occupies another slot (no head-of-line
// blocking on the oldest job).  Sleep briefly only when a full pool had nothing
// finish.  Child stdout/stderr stays buffered in the QProcess (SeparateChannels).
static void runJobsParallel(std::vector<PngJob> &jobs)
{
    int slotCount = QThread::idealThreadCount();
    if(slotCount < 1)
        slotCount = 1;
    std::vector<PngJob*> active;
    size_t nextJob = 0;
    while(nextJob < jobs.size() || !active.empty())
    {
        while((int)active.size() < slotCount && nextJob < jobs.size())
        {
            PngJob *j = &jobs[nextJob];
            nextJob++;
            j->step = 0;
            j->proc = new QProcess();
            j->proc->start("/usr/bin/nice", j->steps[0]);
            active.push_back(j);
        }
        bool progressed = false;
        size_t i = 0;
        while(i < active.size())
        {
            PngJob *j = active[i];
            // waitForFinished(0) returns immediately: true (and reaps) if the child
            // has already exited, false if it is still running.
            if(j->proc->state() == QProcess::NotRunning || j->proc->waitForFinished(0))
            {
                progressed = true;
                delete j->proc;
                j->proc = NULL;
                j->step++;
                if(j->step < j->steps.size())
                {
                    // this file's pngquant finished — now its zopflipng
                    j->proc = new QProcess();
                    j->proc->start("/usr/bin/nice", j->steps[j->step]);
                    i++;
                }
                else
                    active.erase(active.begin() + i);
            }
            else
                i++;
        }
        // Nothing finished this sweep and the pool is full → wait a little before
        // re-polling instead of spinning the CPU on the poll loop.
        if(!progressed && !active.empty())
            QThread::msleep(50);
    }
}

// Post-process every PNG transferred into the output datapack: pngquant (lossy
// palette) then zopflipng (lossless deflate, 100 iterations) on the SAME file,
// in that order.  Run AFTER all writers/guards so it never affects them.  No
// shell — explicit argv via QProcess, each tool launched through nice(1) so this
// one-time but slow pass stays out of the way of interactive work (nice -n 19 =
// lowest priority).  Files are processed in parallel across all CPU cores; each
// file's zopflipng waits for that file's pngquant.  Skips a tool when missing.
static void optimizePngs(const std::string &dir)
{
    std::vector<PngBySize> pngs;
    QDirIterator it(QString::fromStdString(dir), QStringList() << "*.png",
                    QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        PngBySize e;
        e.path = it.next();
        e.size = it.fileInfo().size();
        pngs.push_back(e);
    }
    if(pngs.empty())
        return;
    std::sort(pngs.begin(), pngs.end(), pngLargerFirst);
    const bool haveQuant = QFile::exists("/usr/bin/pngquant");
    const bool haveZopfli = QFile::exists("/usr/bin/zopflipng");
    if(!haveQuant)
        std::cout << "no /usr/bin/pngquant — install it for smaller PNGs" << std::endl;
    if(!haveZopfli)
        std::cout << "no /usr/bin/zopflipng — install zopfli for smaller PNGs" << std::endl;
    if(!haveQuant && !haveZopfli)
        return;
    std::cout << "Optimizing " << pngs.size() << " PNGs (pngquant+zopflipng, all cores)..." << std::flush;
    std::vector<PngJob> jobs;
    size_t i=0;
    while(i<pngs.size())
    {
        const QString path = pngs.at(i).path;
        PngJob j;
        j.step = 0;
        j.proc = NULL;
        if(haveQuant)
        {
            QStringList a;
            a << "-n" << "19" << "/usr/bin/pngquant"
              << "--force" << "--skip-if-larger" << "--ext" << ".png"
              << "--quality" << "65-95" << path;
            j.steps.push_back(a);
        }
        if(haveZopfli)
        {
            QStringList a;
            // --iterations=100: maximum deflate effort (default 5-15).  Slow but
            // one-time; -y overwrites the input in place.  Runs only after this
            // same file's pngquant step has completed.
            a << "-n" << "19" << "/usr/bin/zopflipng"
              << "-y" << "--iterations=100" << path << path;
            j.steps.push_back(a);
        }
        jobs.push_back(j);
        i++;
    }
    runJobsParallel(jobs);
    std::cout << " done" << std::endl;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if(argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <mods/tuxemon dir> <output datapack dir>" << std::endl;
        std::cerr << "  e.g. " << argv[0]
                  << " /home/user/Desktop/CatchChallenger/old/Tuxemon/mods/tuxemon"
                  << " /home/user/Desktop/CatchChallenger/Tuxemon-datapack" << std::endl;
        return 1;
    }

    const std::string modRoot = QDir(QString::fromUtf8(argv[1])).absolutePath().toStdString();
    const std::string outRoot = QDir(QString::fromUtf8(argv[2])).absolutePath().toStdString();

    if(!QDir(QString::fromStdString(modRoot)).exists())
    {
        std::cerr << "Input mod directory does not exist: " << modRoot << std::endl;
        return 1;
    }
    if(!QDir(QString::fromStdString(modRoot + "/db")).exists())
    {
        std::cerr << "Input directory has no db/ subfolder: " << modRoot << std::endl;
        return 1;
    }

    tuxemon::TuxemonDb db;
    if(!db.load(modRoot))
    {
        std::cerr << "Failed to load the Tuxemon db." << std::endl;
        return 1;
    }

    tuxemon::Localization l10n;
    l10n.load(modRoot);

    if(!QDir().mkpath(QString::fromStdString(outRoot)))
    {
        std::cerr << "Cannot create the output directory: " << outRoot << std::endl;
        return 1;
    }
    tuxemon::DatapackWriter writer(db, l10n, modRoot, outRoot);
    if(!writer.writeAll())
    {
        std::cerr << "Failed to write the datapack." << std::endl;
        return 1;
    }

    tuxemon::SkinGen skins(modRoot, outRoot);
    tuxemon::MapConverter maps(modRoot, outRoot, db, writer, l10n, skins);
    if(maps.convertAll() != 0)
    {
        std::cerr << "Failed to convert one or more maps (I/O error)." << std::endl;
        return 1;
    }

    tuxemon::WorldWriter world(db, l10n, modRoot, outRoot, skins, writer,
                               maps.startMap(), maps.startX(), maps.startY());
    if(!world.writeAll())
    {
        std::cerr << "Failed to write the world/meta datapack files." << std::endl;
        return 1;
    }
    std::cerr << "Skins generated: " << skins.count() << std::endl;

    // Shrink every transferred PNG (pngquant + zopflipng) — last, after the guards.
    optimizePngs(outRoot);

    std::cout << "Done." << std::endl;
    return 0;
}
