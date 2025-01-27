BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "account" (
	"id"	INTEGER NOT NULL,
	"login"	TEXT,
	"password"	TEXT,
	"date"	INTEGER
);
CREATE TABLE IF NOT EXISTS "character" (
	"id"	INTEGER,
	"pseudo"	TEXT NOT NULL,
	"skin"	INTEGER,
	"type"	INTEGER,
	"clan"	INTEGER,
	"clan_leader"	INTEGER,
	"date"	INTEGER,
	"account"	INTEGER,
	"cash"	INTEGER,
	"warehouse_cash"	INTEGER,
	"time_to_delete"	INTEGER,
	"played_time"	INTEGER,
	"last_connect"	INTEGER,
	"starter"	INTEGER,
	"allow"	BLOB,
	"item"	BLOB,
	"item_warehouse"	BLOB,
	"recipes"	BLOB,
	"reputations"	BLOB,
	"encyclopedia_monster"	BLOB,
	"encyclopedia_item"	BLOB,
	"achievements"	BLOB,
	"lastdaillygift"	INTEGER
);
CREATE TABLE IF NOT EXISTS "character_bymap" (
	"character"	INTEGER,
	"map"	INTEGER,
	"plants"	BLOB,
	"items"	BLOB,
	"fights"	BLOB
);
CREATE TABLE IF NOT EXISTS "character_forserver" (
	"character"	INTEGER,
	"x"	INTEGER,
	"y"	INTEGER,
	"orientation"	INTEGER,
	"map"	INTEGER,
	"rescue_map"	INTEGER,
	"rescue_x"	INTEGER,
	"rescue_y"	INTEGER,
	"rescue_orientation"	INTEGER,
	"unvalidated_rescue_map"	INTEGER,
	"unvalidated_rescue_x"	INTEGER,
	"unvalidated_rescue_y"	INTEGER,
	"unvalidated_rescue_orientation"	INTEGER,
	"date"	INTEGER,
	"market_cash"	INTEGER,
	"quest"	BLOB
);
CREATE TABLE IF NOT EXISTS "city" (
	"city"	TEXT,
	"clan"	INTEGER
);
CREATE TABLE IF NOT EXISTS "clan" (
	"id"	INTEGER,
	"name"	TEXT,
	"cash"	INTEGER,
	"date"	INTEGER
);
CREATE TABLE IF NOT EXISTS "dictionary_allow" (
	"id"	INTEGER,
	"allow"	TEXT
);
CREATE TABLE IF NOT EXISTS "dictionary_map" (
	"id"	INTEGER,
	"map"	TEXT
);
CREATE TABLE IF NOT EXISTS "dictionary_reputation" (
	"id"	INTEGER,
	"reputation"	TEXT
);
CREATE TABLE IF NOT EXISTS "dictionary_server" (
	"id"	INTEGER,
	"key"	TEXT,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "dictionary_skin" (
	"id"	INTEGER,
	"skin"	TEXT
);
CREATE TABLE IF NOT EXISTS "dictionary_starter" (
	"id"	INTEGER NOT NULL,
	"starter"	TEXT,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "factory" (
	"id"	INTEGER,
	"resources"	TEXT,
	"products"	TEXT,
	"last_update"	INTEGER
);
CREATE TABLE IF NOT EXISTS "monster" (
	"id"	INTEGER,
	"hp"	INTEGER,
	"monster"	INTEGER,
	"level"	INTEGER,
	"xp"	INTEGER,
	"sp"	INTEGER,
	"captured_with"	INTEGER,
	"gender"	INTEGER,
	"egg_step"	INTEGER,
	"character_origin"	INTEGER,
	"position"	INTEGER,
	"place"	INTEGER,
	"buffs"	BLOB,
	"skills"	BLOB,
	"skills_endurance"	BLOB,
	"character"	INTEGER
);
CREATE TABLE IF NOT EXISTS "monster_market_price" (
	"id"	INTEGER,
	"market_price"	INTEGER,
	"character"	INTEGER
);
CREATE TABLE IF NOT EXISTS "server_time" (
	"server"	INTEGER,
	"account"	INTEGER,
	"played_time"	INTEGER,
	"last_connect"	INTEGER
);
CREATE INDEX IF NOT EXISTS "byclan" ON "character" (
	"clan"	ASC
);
CREATE UNIQUE INDEX IF NOT EXISTS "bypseudoandclan" ON "character" (
	"pseudo"	ASC,
	"clan"	ASC
);
CREATE INDEX IF NOT EXISTS "characterdata_bymap" ON "character_bymap" (
	"character"
);
CREATE UNIQUE INDEX IF NOT EXISTS "characterdata_bymapunique" ON "character_bymap" (
	"character",
	"map"
);
CREATE UNIQUE INDEX IF NOT EXISTS "cityindex" ON "city" (
	"city"	ASC
);
CREATE UNIQUE INDEX IF NOT EXISTS "clan_index" ON "clan" (
	"id"	ASC
);
CREATE UNIQUE INDEX IF NOT EXISTS "id" ON "character" (
	"id"	ASC
);
CREATE UNIQUE INDEX IF NOT EXISTS "idcharacter_forserver" ON "character_forserver" (
	"character"	ASC
);
CREATE UNIQUE INDEX IF NOT EXISTS "index_account" ON "account" (
	"id"	ASC
);
CREATE UNIQUE INDEX IF NOT EXISTS "login_account" ON "account" (
	"login"	ASC
);
CREATE UNIQUE INDEX IF NOT EXISTS "monster_index_key" ON "monster" (
	"id"	ASC
);
CREATE INDEX IF NOT EXISTS "player_link_account" ON "character" (
	"account"	ASC
);
CREATE UNIQUE INDEX IF NOT EXISTS "player_unique_pseudo" ON "character" (
	"pseudo"	ASC
);
CREATE INDEX IF NOT EXISTS "server_time_by_account" ON "server_time" (
	"account"	ASC
);
CREATE UNIQUE INDEX IF NOT EXISTS "server_time_index" ON "server_time" (
	"server"	ASC,
	"account"	ASC
);
COMMIT;
