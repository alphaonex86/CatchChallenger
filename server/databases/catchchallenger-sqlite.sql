CREATE TABLE sqlite_sequence(name,seq);
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
CREATE TABLE bitcoin_history (
    "character" INTEGER,
    "date" INTEGER,
    "change" REAL,
    "reason" TEXT
);
CREATE TABLE bot_already_beaten (
    "character" INTEGER,
    "botfight_id" INTEGER
);
CREATE TABLE item (
    "item" INTEGER,
    "character" INTEGER,
    "quantity" INTEGER,
    "place" TEXT,
    "market_price" INTEGER,
    "market_bitcoin" REAL
);
CREATE INDEX "item_place" on item (place ASC);
CREATE TABLE plant (
    "map" TEXT,
    "x" INTEGER,
    "y" INTEGER,
    "plant" INTEGER,
    "character" INTEGER,
    "plant_timestamps" INTEGER
);
CREATE UNIQUE INDEX "plant_index_map" on plant (map ASC, x ASC, y ASC);
CREATE TABLE quest (
    "character" INTEGER,
    "quest" INTEGER,
    "finish_one_time" INTEGER,
    "step" INTEGER
);
CREATE TABLE recipes (
    "character" INTEGER,
    "recipe" INTEGER
);
CREATE TABLE reputation (
    "character" INTEGER,
    "type" TEXT,
    "point" INTEGER,
    "level" INTEGER
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
    "gender" TEXT,
    "egg_step" INTEGER,
    "character_origin" INTEGER,
    "place" TEXT,
    "position" INTEGER,
    "market_price" INTEGER,
    "market_bitcoin" REAL
);
CREATE UNIQUE INDEX "monster_index_key" on monster (id ASC);
CREATE INDEX "monster_place" on monster (place ASC);
CREATE TABLE character (
    "id" INTEGER,
    "pseudo" TEXT NOT NULL,
    "skin" TEXT NOT NULL,
    "x" INTEGER,
    "y" INTEGER,
    "orientation" TEXT NOT NULL,
    "map" TEXT NOT NULL,
    "type" TEXT NOT NULL,
    "clan" INTEGER,
    "rescue_map" TEXT,
    "rescue_x" INTEGER,
    "rescue_y" INTEGER,
    "rescue_orientation" TEXT,
    "unvalidated_rescue_map" TEXT,
    "unvalidated_rescue_x" INTEGER,
    "unvalidated_rescue_y" INTEGER,
    "unvalidated_rescue_orientation" TEXT,
    "allow" TEXT,
    "clan_leader" INTEGER,
    "date" INTEGER,
    "account" INTEGER,
    "bitcoin_offset" REAL,
    "market_bitcoin" REAL,
    "cash" INTEGER,
    "warehouse_cash" INTEGER,
    "time_to_delete" INTEGER,
    "played_time" INTEGER,
    "last_connect" INTEGER,
    "market_cash" INTEGER
, "starter" INTEGER);
CREATE UNIQUE INDEX "id" on "character" (id ASC);
CREATE UNIQUE INDEX "bypseudoandclan" on "character" (pseudo ASC, clan ASC);
CREATE INDEX "byclan" on "character" (clan ASC);
CREATE UNIQUE INDEX "player_unique_pseudo" on "character" (pseudo ASC);
CREATE INDEX "player_link_account" on "character" (account ASC);