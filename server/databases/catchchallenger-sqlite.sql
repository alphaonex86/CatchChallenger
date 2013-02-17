PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE "item" (
    "item_id" INTEGER,
    "player_id" INTEGER,
    "quantity" INTEGER
);
CREATE TABLE plant (
    "map" TEXT,
    "x" INTEGER,
    "y" INTEGER,
    "plant" INTEGER,
    "player_id" INTEGER
, "plant_timestamps" INTEGER);
CREATE TABLE "recipes" (
    "player" INTEGER,
    "recipe" INTEGER
);
CREATE TABLE player (
    "id" INTEGER,
    "login" TEXT NOT NULL,
    "password" TEXT NOT NULL,
    "pseudo" TEXT NOT NULL,
    "skin" TEXT NOT NULL,
    "position_x" INTEGER,
    "position_y" INTEGER,
    "orientation" TEXT NOT NULL,
    "map_name" TEXT NOT NULL,
    "type" TEXT NOT NULL,
    "clan" INTEGER,
    "cash" INTEGER,
    "rescue_map" TEXT,
    "rescue_x" INTEGER,
    "rescue_y" INTEGER
, "rescue_orientation" TEXT);
CREATE TABLE "monster_buff" (
    "monster" INTEGER PRIMARY KEY,
    "buff" INTEGER,
    "level" INTEGER
);
CREATE TABLE "monster_skill" (
    "monster" INTEGER PRIMARY KEY,
    "skill" INTEGER,
    "level" INTEGER
);
CREATE TABLE reputation (
    "player" INTEGER,
    "type" TEXT,
    "point" INTEGER
, "level" INTEGER);
CREATE TABLE monster (
    "id" INTEGER,
    "hp" INTEGER,
    "player" INTEGER,
    "monster" INTEGER,
    "level" INTEGER,
    "xp" INTEGER,
    "sp" INTEGER,
    "captured_with" INTEGER,
    "gender" TEXT,
    "egg_step" INTEGER
, "player_origin" INTEGER);
CREATE INDEX "player_item_index" on item (player_id ASC);
CREATE UNIQUE INDEX "player_item_unique_index" on item (item_id ASC, player_id ASC);
CREATE UNIQUE INDEX "plant_index_map" on plant (map ASC, x ASC, y ASC);
CREATE UNIQUE INDEX "player_recipe" on recipes (player ASC, recipe ASC);
CREATE INDEX "player_recipe_list" on recipes (player ASC);
CREATE UNIQUE INDEX "id" on player (id ASC);
CREATE UNIQUE INDEX "login/pseudo" on player (login ASC, password ASC);
CREATE UNIQUE INDEX "monster_buff_2" on monster_buff (monster ASC, buff ASC);
CREATE UNIQUE INDEX "monster_skill_2" on monster_skill (monster ASC, skill ASC);
CREATE UNIQUE INDEX "reputation_index" on reputation (player ASC, type ASC);
CREATE INDEX "monster_by_player" on monster (player ASC);
COMMIT;
