#include "CraftingAnimation.h"

CraftingAnimation::CraftingAnimation(const QStringList &mIngredients,
                                     const QString &mRecipe,
                                     const QString &mProduct,
                                     const QString &mBackPlayer,
                                     QObject *parent) :
    QObject(parent),
    mIngredients(mIngredients),
    mRecipe(mRecipe),
    mProduct(mProduct),
    mBackPlayer(mBackPlayer)
{
}

QStringList CraftingAnimation::ingredients()
{
    return mIngredients;
}

QString CraftingAnimation::recipe()
{
    return mRecipe;
}

QString CraftingAnimation::product()
{
    return mProduct;
}

QString CraftingAnimation::backPlayer()
{
    return mBackPlayer;
}
