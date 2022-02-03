CREATE TABLE "globalchat" (
    "chat_type" INTEGER NOT NULL,
    "text" TEXT NOT NULL,
    "pseudo" TEXT NOT NULL,
    "player_type" TEXT
);
CREATE TABLE "note" (
    "player" TEXT NOT NULL,
    "text" TEXT
);
CREATE UNIQUE INDEX "note_byplayer" on note (player ASC);
CREATE TABLE privatechat (
    "text" TEXT NOT NULL,
    "player_type" INTEGER,
    "player" TEXT,
    "theotherplayer" TEXT
, "fromplayer" INTEGER);
CREATE INDEX "playerchat_byplayer" on privatechat (player ASC);
CREATE INDEX "playerchat_byplayerandother" on privatechat (player ASC, theotherplayer ASC);
CREATE TABLE otherchat (
    "chat_type" INTEGER,
    "text" TEXT,
    "player_type" INTEGER,
    "player" TEXT
, "theotherplayer" TEXT);
CREATE INDEX "otherchat_byplayer" on otherchat (player ASC);
CREATE INDEX "otherchat_byplayerchat" on otherchat (chat_type ASC, player ASC);
CREATE TABLE "preferences" (
    "pseudo" TEXT,
    "plant" INTEGER,
    "item" INTEGER,
    "fight" INTEGER,
    "shop" INTEGER,
    "wild" INTEGER
);
CREATE UNIQUE INDEX "pref_by_pseudo" on preferences (pseudo ASC);
