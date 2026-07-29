#include "CCGraphicsTextItem.hpp"
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QTextDocument>
#include <QFontMetricsF>

CCGraphicsTextItem::CCGraphicsTextItem(QGraphicsItem *parent) :
    QGraphicsTextItem(parent)
{
}

CCGraphicsTextItem::CCGraphicsTextItem(const QString &text, QGraphicsItem *parent) :
    QGraphicsTextItem(text,parent)
{
}

void CCGraphicsTextItem::setPlaceholderText(const QString &text)
{
    this->m_placeholder=text;
}

void CCGraphicsTextItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    QStyleOptionGraphicsItem opt(*option);

    // Remove the selection style state, to prevent the dotted line from being drawn.
    opt.state = QStyle::State_None;

    // Draw your fill on the item rectangle (or as big as you require) before drawing the text
    // This is where you can use your calculated values (as member variables) from what you did with the slider
    /*painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::green);*/
    //painter->drawRect(whateverRectangle());

    // Call the parent to do the actual text drawing
    QGraphicsTextItem::paint(painter, &opt, widget);

    // The placeholder is DRAWN, never written into the document: setHtml() here
    // dirtied the item from inside its own paint(), so Qt scheduled another paint
    // and the item repainted for ever (and modifying an item while it is being
    // painted is not allowed). Same grey as the old "<span style=color:grey>".
    const bool textEditingMode = (textInteractionFlags() & Qt::TextEditorInteraction);
    const bool isSelected = (option->state & QStyle::State_Selected);
    if(!textEditingMode && !isSelected && toPlainText().isEmpty() && !m_placeholder.isEmpty())
    {
        const qreal margin=document()->documentMargin();
        const QFontMetricsF fm(font());
        painter->save();
        painter->setPen(QColor(128,128,128));
        painter->setFont(font());
        //baseline of the first line, where the document would have drawn it
        painter->drawText(QPointF(margin,margin+fm.ascent()),m_placeholder);
        painter->restore();
    }

    // Draw your rectangle - can be different in selected mode or editing mode if you wish

    if (option->state & (QStyle::State_Selected))
    {
        // You can change pen thickness for the selection outline if you like
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::transparent);
        //painter->drawRect(0,0,widget->width(),widget->height());
    }
}
