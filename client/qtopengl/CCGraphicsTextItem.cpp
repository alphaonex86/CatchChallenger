#include "CCGraphicsTextItem.hpp"
#include <QStyleOptionGraphicsItem>
#include <QPainter>

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

    // You can use these to decide when you draw
    bool placeHolder=false;
    bool textEditingMode = (textInteractionFlags() & Qt::TextEditorInteraction);
    bool isSelected = (option->state & QStyle::State_Selected);
    if(!textEditingMode && !isSelected && toPlainText().isEmpty() && !m_placeholder.isEmpty())
    {
        setHtml("<span style=\"color:grey;\">"+m_placeholder+"</span>");
        placeHolder=true;
    }

    // Call the parent to do the actual text drawing
    QGraphicsTextItem::paint(painter, &opt, widget);

    if(placeHolder)
        setHtml("");

    // Draw your rectangle - can be different in selected mode or editing mode if you wish

    if (option->state & (QStyle::State_Selected))
    {
        // You can change pen thickness for the selection outline if you like
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::magenta);
        painter->drawRect(0,0,widget->width(),widget->height());
    }
}
