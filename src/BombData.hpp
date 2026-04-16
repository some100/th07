#pragma once

#include "Player.hpp"

struct BombData
{
    static void DarkenViewport(Player *player);
    static void SpawnBombInvulnEffect(Player *player);
    static void ComputeBombCherryDrain(Player *player, i32 minCost, f32 scale);
    static void BombReimuACalc(Player *player);
    static void BombReimuADraw(Player *player);
    static void BombReimuACalcFocus(Player *player);
    static void BombReimuADrawFocus(Player *player);
    static void BombReimuBCalc(Player *player);
    static void BombReimuBDraw(Player *player);
    static void BombReimuBCalcFocus(Player *player);
    static void BombReimuBDrawFocus(Player *player);
    static void BombMarisaACalc(Player *player);
    static void BombMarisaADraw(Player *player);
    static void BombMarisaACalcFocus(Player *player);
    static void BombMarisaADrawFocus(Player *player);
    static void BombMarisaBCalc(Player *player);
    static void BombMarisaBDraw(Player *player);
    static void BombMarisaBCalcFocus(Player *player);
    static void BombMarisaBDrawFocus(Player *player);
    static void BombSakuyaACalc(Player *player);
    static void BombSakuyaADraw(Player *player);
    static void BombSakuyaACalcFocus(Player *player);
    static void BombSakuyaADrawFocus(Player *player);
    static void BombSakuyaBCalc(Player *player);
    static void BombSakuyaBDraw(Player *player);
    static void BombSakuyaBCalcFocus(Player *player);
    static void BombSakuyaBDrawFocus(Player *player);

    BombCallback calc;
    BombCallback draw;
    BombCallback calcFocus;
    BombCallback drawFocus;
};

extern BombData g_BombData[6];
