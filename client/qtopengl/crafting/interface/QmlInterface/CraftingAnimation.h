#ifndef CraftingAnimation_H
#define CraftingAnimation_H

#include <QObject>
#include <QStringList>

class CraftingAnimation : public QObject
{
    Q_OBJECT
public:
    explicit CraftingAnimation(
    const QStringList &mIngredients,
    const QString &mRecipe,
    const QString &mProduct,
    const QString &mBackPlayer,
    QObject *parent = 0);
private:
    const QStringList mIngredients;
    const QString mRecipe;
    const QString mProduct;
    const QString mBackPlayer;
public slots:
    QStringList ingredients();
    QString recipe();
    QString product();
    QString backPlayer();
};

#endif // CraftingAnimation_H
