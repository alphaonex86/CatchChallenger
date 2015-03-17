CREATE TABLE "monster_buff" (
    "monster" INTEGER,
    "buff" INTEGER,
    "level" INTEGER
);
CREATE UNIQUE INDEX "monster_buff_2" on monster_buff (monster ASC, buff ASC);
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
CREATE TABLE monster_skill (
    "monster" INTEGER,
    "skill" INTEGER,
    "level" INTEGER
, "endurance" INTEGER);
CREATE UNIQUE INDEX "monster_skill_2" on monster_skill (monster ASC, skill ASC);
CREATE TABLE account (
    "id" INTEGER NOT NULL,
    "login" TEXT,
    "password" TEXT,
    "date" INTEGER
);
CREATE UNIQUE INDEX "index_account" on account (id ASC);
CREATE UNIQUE INDEX "login_account" on account (login ASC);
CREATE TABLE bot_already_beaten (
    "character" INTEGER,
    "botfight_id" INTEGER
);
CREATE TABLE item (
    "item" INTEGER,
    "character" INTEGER,
    "quantity" INTEGER
);
CREATE TABLE item_warehouse (
    "item" INTEGER,
    "character" INTEGER,
    "quantity" INTEGER
);
CREATE TABLE item_market (
    "item" INTEGER,
    "character" INTEGER,
    "quantity" INTEGER,
    "market_price" INTEGER
);
CREATE TABLE quest (
    "character" INTEGER,
    "quest" INTEGER,
    "finish_one_time" INTEGER,
    "step" INTEGER
);
CREATE TABLE "recipe" (
    "character" INTEGER,
    "recipe" INTEGER
);
CREATE TABLE reputation (
    "character" INTEGER,
    "type" INTEGER,
    "point" INTEGER,
    "level" INTEGER
);
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
    "starter" INTEGER);
CREATE UNIQUE INDEX "id" on "character" (id ASC);
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
    "played_time" INTEGER,
    "last_connect" INTEGER,
    "market_cash" INTEGER);
CREATE UNIQUE INDEX "idcharacter_forserver" on "character_forserver" (character ASC);
CREATE UNIQUE INDEX "bypseudoandclan" on "character" (pseudo ASC, clan ASC);
CREATE INDEX "byclan" on "character" (clan ASC);
CREATE UNIQUE INDEX "player_unique_pseudo" on "character" (pseudo ASC);
CREATE INDEX "player_link_account" on "character" (account ASC);
CREATE TABLE plant (
    "map" INTEGER,
    "x" INTEGER,
    "y" INTEGER,
    "plant" INTEGER,
    "character" INTEGER,
    "plant_timestamps" INTEGER
, "id" INTEGER);
CREATE UNIQUE INDEX "plant_primarykey" on plant (id ASC);
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
CREATE TABLE character_allow (
    "character" INTEGER,
    "allow" INTEGER
);
CREATE TABLE "character_itemOnMap" (
    "character" INTEGER,
    "itemOnMap" INTEGER
);
CREATE INDEX "character_itemOnMap_index" on character_itemonmap (character ASC);
CREATE TABLE dictionary_itemonmap (
    "id" INTEGER,
    "map" TEXT,
    "x" INTEGER,
    "y" INTEGER
);
CREATE TABLE "dictionary_starter" (
    "id" INTEGER PRIMARY KEY NOT NULL,
    "starter" TEXT
);
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
    "position" INTEGER
, "place" INTEGER);
CREATE UNIQUE INDEX "monster_index_key" on monster (id ASC);
CREATE TABLE "monster_market_price" (
    "id" INTEGER PRIMARY KEY,
    "market_price" INTEGER
);
