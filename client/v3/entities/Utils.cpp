// Copyright 2021 CatchChallenger
#include "Utils.hpp"

#include <QDir>
#include <QDirIterator>
#include <iostream>

#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../core/SceneManager.hpp"
#include "../libraries/gifimage/qgifimage.h"
#include "../ui/Label.hpp"
#include "../ui/ThemedButton.hpp"

Node *Utils::GetCurrentScene(Node *node) {
  Node *parent = node;
  while (parent != nullptr) {
    if (parent->ClassName() == "Scene") {
      break;
    }
    parent = parent->Parent();
  }
  return parent;
}

NodeParsed Utils::HTML2Node(const QString &html) {
  return Utils::HTML2Node(html, nullptr);
}

NodeParsed Utils::HTML2Node(const QString &html,
                            std::function<void(Node *)> on_click) {
  QRegExp new_line("<br(\\s*)(\\/|)>");
  QRegExp anchor("<a\\s+href=\"(.+)\">(.+)</a>");
  QString tmp = RemoveHTMLEntities(html);
  auto lines = tmp.split(new_line);
  auto records = std::vector<Node *>();
  NodeParsed response;
  for (auto iter = lines.begin(); iter != lines.end(); ++iter) {
    auto line = (*iter);
    line = line.trimmed();
    if (line.startsWith("<a")) {
      if (anchor.exactMatch(line)) {
        auto node = UI::GreenButton::Create();
        node->SetHeight(30);
        node->SetPixelSize(12);
        node->SetText(anchor.cap(2));
        node->SetData(99, anchor.cap(1).toStdString());
        if (on_click) {
          node->SetOnClick(on_click);
        }
        records.push_back(node);
      }
    } else {
      auto node = UI::Label::Create();
      node->SetText(line);
      node->SetPixelSize(12);
      records.push_back(node);
    }
  }
  response.nodes = records;
  return response;
}

QPixmap Utils::B64Png2Pixmap(const QString &b64) {
  QByteArray by = QByteArray::fromBase64(b64.toUtf8());
  QImage image = QImage::fromData(by, "PNG");
  return QPixmap::fromImage(image);
}

QString Utils::RemoveHTMLEntities(const QString &html) {
  QString tmp = html;
  tmp.replace(QString("<ul>"), QString(""))
      .replace(QString("</ul>"), QString(""))
      .replace(QString("<li>"), QString(""))
      .replace(QString("</li>"), QString(""))
      .replace(QString("<i>"), QString(""))
      .replace(QString("</i>"), QString(""))
      .replace(QString("<b>"), QString(""))
      .replace(QString("</b>"), QString(""));
  return tmp;
}

QImage Utils::CropToContent(const QImage &buffer, QColor stop) {
  int pivot = 0;
  QColor aux;
  do {
    pivot++;
    aux = buffer.pixel(10, pivot);
  } while (aux != stop && pivot < buffer.height());
  return buffer.copy(0, 0, buffer.width(), pivot);
}

bool Utils::IsActiveInScene(Node *node) {
  auto root = node->Root();

  if (root == nullptr) return false;
  return root == SceneManager::GetInstance()->CurrentScene();
}

void Utils::MoveDatapackFolder(const QString &origin, const QString &dest) {
  QDirIterator it(origin, QDirIterator::Subdirectories);
  QDir dir(origin);
  const int absSourcePathLength = dir.absoluteFilePath(origin).length();

  while (it.hasNext()) {
    it.next();
    const auto fileInfo = it.fileInfo();
    if (!fileInfo.isHidden()) {
      const QString subPathStructure =
          fileInfo.absoluteFilePath().mid(absSourcePathLength);
      const QString constructedAbsolutePath = dest + subPathStructure;

      if (fileInfo.isDir()) {
        dir.mkpath(constructedAbsolutePath);
      } else if (fileInfo.isFile()) {
        QFile::remove(constructedAbsolutePath);
        QFile::copy(fileInfo.absoluteFilePath(), constructedAbsolutePath);
      }
    }
  }
}

void Utils::MoveFile(const QString &origin, const QString &folder,
                     const QString &name) {
  QFile file(origin);
}

std::vector<QPixmap> Utils::GetSkillAnimation(const uint16_t &id) {
  QString file_animation =
      QString::fromStdString(
          QtDatapackClientLoader::GetInstance()->getDatapackPath()) +
      DATAPACK_BASE_PATH_SKILLANIMATION + QStringLiteral("%1.gif").arg(id);

  std::vector<QPixmap> frames;
  if (!QFile(file_animation).exists()) {
    file_animation =
        QString::fromStdString(
            QtDatapackClientLoader::GetInstance()->getDatapackPath()) +
        DATAPACK_BASE_PATH_SKILLANIMATION + QStringLiteral("0.gif");
  }
  QGifImage gif(file_animation);
  int width = 80;
  int height = 80;
  for (int i = 0; i < gif.frameCount(); ++i) {
    QImage image = gif.frame(i);
    QPixmap buffer(width, height);
    buffer.fill(Qt::transparent);
    QPainter *painter = new QPainter(&buffer);
    painter->drawImage(image.offset(), image);
    delete painter;
    frames.push_back(buffer);
  }
  return frames;
}
