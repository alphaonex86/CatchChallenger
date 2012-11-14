PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE "player" (    "id" INTEGER PRIMARY KEY AUTOINCREMENT,    "login" TEXT NOT NULL,    "password" TEXT NOT NULL,    "pseudo" TEXT NOT NULL,    "skin" TEXT NOT NULL,    "position_x" INTEGER,    "position_y" INTEGER,    "orientation" TEXT NOT NULL,    "map_name" TEXT NOT NULL,    "type" TEXT NOT NULL,    "clan" INTEGER);
CREATE TABLE "item" (
    "item_id" INTEGER,
    "player_id" INTEGER,
    "quantity" INTEGER
);
CREATE UNIQUE INDEX "id" on player (id ASC);
CREATE UNIQUE INDEX "login/pseudo" on player (login ASC, password ASC);
CREATE INDEX "player_item_index" on item (player_id ASC);
CREATE UNIQUE INDEX "player_item_unique_index" on item (item_id ASC, player_id ASC);
COMMIT;
