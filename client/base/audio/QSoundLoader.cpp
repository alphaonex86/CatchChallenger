/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
 * See AUTHORS file for a full list of contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>

#include "QSoundFile.h"
#include "QSoundLoader.h"
#include "QSoundPlayer.h"

QSoundJob::QSoundJob(QSoundFile *file)
    : m_file(file)
    , m_output(0)
{
    m_file->setParent(this);
}

void QSoundJob::start()
{
    QSoundPlayer *player = QSoundPlayer::instance();
    if (!player) {
        qWarning("Could not find sound player");
        emit finished();
        return;
    }

    m_output = new QAudioOutput(player->outputDevice(), m_file->format(), this);
    connect(m_output, SIGNAL(stateChanged(QAudio::State)),
            this, SLOT(_q_stateChanged()));
    m_output->start(m_file);
}

void QSoundJob::stop()
{
    if (m_output)
        m_output->stop();
}

void QSoundJob::_q_stateChanged()
{
    if (m_output && m_output->state() != QAudio::ActiveState) {
        m_output->deleteLater();
        m_output = 0;
        emit finished();
    }
}

