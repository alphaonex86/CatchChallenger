CREATE TABLE "city" (
    "city" TEXT,
    "clan" INTEGER
);
CREATE UNIQUE INDEX "cityindex" on city (city ASC);
CREATE TABLE factory (
    "id" INTEGER,
    "resources" TEXT,
    "products" TEXT,
    "last_update" INTEGER
);
CREATE TABLE clan (
    "id" INTEGER,
    "name" TEXT,
    "cash" INTEGER
, "date" INTEGER);
CREATE UNIQUE INDEX "clan_index" on clan (id ASC);
CREATE TABLE dictionary_allow (
    "id" INTEGER,
    "allow" TEXT
);
CREATE TABLE dictionary_map (
    "id" INTEGER,
    "map" TEXT
);
CREATE TABLE dictionary_reputation (
    "id" INTEGER,
    "reputation" TEXT
);
CREATE TABLE dictionary_skin (
    "id" INTEGER,
    "skin" TEXT
);
CREATE TABLE "dictionary_starter" (
    "id" INTEGER PRIMARY KEY NOT NULL,
    "starter" TEXT
);
CREATE TABLE "monster_market_price" (
    "id" INTEGER PRIMARY KEY,
    "market_price" INTEGER
);
CREATE TABLE "dictionary_server" (
    "id" INTEGER PRIMARY KEY,
    "key" TEXT
);
CREATE TABLE "server_time" (
    "server" INTEGER,
    "account" INTEGER,
    "played_time" INTEGER,
    "last_connect" INTEGER
);
CREATE UNIQUE INDEX "server_time_index" on server_time (server ASC, account ASC);
CREATE INDEX "server_time_by_account" on server_time (account ASC);
CREATE TABLE dictionary_pointonmap (
    "id" INTEGER,
    "map" INTEGER,
    "x" INTEGER,
    "y" INTEGER
);
CREATE TABLE plant (
    "pointOnMap" INTEGER,
    "plant" INTEGER,
    "character" INTEGER,
    "plant_timestamps" INTEGER
);
CREATE UNIQUE INDEX "plant_unqiue" on plant (pointOnMap ASC, character ASC);
CREATE INDEX "plant_by_char" on plant (character ASC);
CREATE TABLE "item_market" (
    "item" INTEGER,
    "character" INTEGER,
    "quantity" INTEGER,
    "market_price" INTEGER
);
CREATE TABLE account (
    "id" INTEGER NOT NULL,
    "login" TEXT,
    "password" TEXT,
    "date" INTEGER
, "blob_version" INTEGER);
CREATE UNIQUE INDEX "index_account" on account (id ASC);
CREATE UNIQUE INDEX "login_account" on account (login ASC);
CREATE TABLE character_forserver (
    "character" INTEGER,
    "x" INTEGER,
    "y" INTEGER,
    "orientation" INTEGER,
    "map" INTEGER,
    "rescue_map" INTEGER,
    "rescue_x" INTEGER,
    "rescue_y" INTEGER,
    "rescue_orientation" INTEGER,
    "unvalidated_rescue_map" INTEGER,
    "unvalidated_rescue_x" INTEGER,
    "unvalidated_rescue_y" INTEGER,
    "unvalidated_rescue_orientation" INTEGER,
    "date" INTEGER,
    "market_cash" INTEGER
, "botfight_id" BLOB, "itemonmap" BLOB, "plants" BLOB, "blob_version" INTEGER);
CREATE UNIQUE INDEX "idcharacter_forserver" on "character_forserver" (character ASC);
CREATE TABLE character (
    "id" INTEGER,
    "pseudo" TEXT NOT NULL,
    "skin" INTEGER,
    "type" INTEGER,
    "clan" INTEGER,
    "clan_leader" INTEGER,
    "date" INTEGER,
    "account" INTEGER,
    "cash" INTEGER,
    "warehouse_cash" INTEGER,
    "time_to_delete" INTEGER,
    "played_time" INTEGER,
    "last_connect" INTEGER,
    "starter" INTEGER
, "allow" BLOB, "item" BLOB, "item_warehouse" BLOB, "recipes" BLOB, "reputations" BLOB, "encyclopedia_monster" BLOB, "encyclopedia_item" BLOB, "achievements" BLOB, "monster" BLOB, "monster_warehouse" BLOB, "monster_market" BLOB);
CREATE UNIQUE INDEX "id" on "character" (id ASC);
CREATE UNIQUE INDEX "bypseudoandclan" on "character" (pseudo ASC, clan ASC);
CREATE INDEX "byclan" on "character" (clan ASC);
CREATE UNIQUE INDEX "player_unique_pseudo" on "character" (pseudo ASC);
CREATE INDEX "player_link_account" on "character" (account ASC);
CREATE TABLE monster (
    "id" INTEGER,
    "hp" INTEGER,
    "character" INTEGER,
    "monster" INTEGER,
    "level" INTEGER,
    "xp" INTEGER,
    "sp" INTEGER,
    "captured_with" INTEGER,
    "gender" INTEGER,
    "egg_step" INTEGER,
    "character_origin" INTEGER,
    "position" INTEGER,
    "place" INTEGER
, "buffs" BLOB, "skills" BLOB, "skills_endurance" BLOB);
CREATE UNIQUE INDEX "monster_index_key" on monster (id ASC);
