#ifndef Login_H
#define Login_H

class Login : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit Login();
    ~Login();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated) override;
private:
    std::vector<ConnexionInfo> temp_customConnexionInfoList,temp_xmlConnexionInfoList,mergedConnexionInfoList;
    QList<MultiItem *> serverConnexion;
    SelectedServer selectedServer;//no selected if unique_code empty

    CustomButton *server_add;
    CustomButton *server_remove;
    CustomButton *server_edit;
    CustomButton *server_select;
    CustomButton *server_refresh;
    CustomButton *back;
    QGraphicsTextItem *warning;

    CCWidget *wdialog;
    QGraphicsTextItem *serverEmpty;
signals:
    void backMulti();
    void setAbove(ScreenInput *widget);//first plan popup
};

#endif // MULTI_H
