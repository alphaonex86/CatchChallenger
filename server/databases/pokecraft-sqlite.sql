CREATE TABLE "player" (    "id" INTEGER PRIMARY KEY AUTOINCREMENT,    "login" TEXT NOT NULL,    "password" TEXT NOT NULL,    "pseudo" TEXT NOT NULL,    "skin" TEXT NOT NULL,    "position_x" INTEGER,    "position_y" INTEGER,    "orientation" TEXT NOT NULL,    "map_name" TEXT NOT NULL,    "type" TEXT NOT NULL,    "clan" INTEGER);
CREATE TABLE sqlite_sequence(name,seq);
CREATE UNIQUE INDEX "id" on player (id ASC);
CREATE UNIQUE INDEX "login/pseudo" on player (login ASC, password ASC);
