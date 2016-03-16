CREATE TABLE "character" (
    id int PRIMARY KEY,
    account int,
    pseudo text,
    skin int,
    type int,
    clan int,
    clan_leader boolean,
    date bigint,
    cash bigint,
    warehouse_cash bigint,
    time_to_delete int,
    played_time int,
    last_connect bigint,
    starter int
);
CREATE TABLE character_allow (
    id int PRIMARY KEY,
    "character" int,
    "allow" int
);
CREATE TABLE clan (
    id int PRIMARY KEY,
    name text,
    cash bigint,
    date bigint
);
CREATE TABLE item (
    id int PRIMARY KEY,
    item int,
    "character" int,
    quantity int
);
CREATE TABLE item_warehouse (
    id int PRIMARY KEY,
    item int,
    "character" int,
    quantity int
);
CREATE TABLE monster (
    id int PRIMARY KEY,
    hp int,
    "character" int,
    monster int,
    level int,
    xp int,
    sp int,
    captured_with int,
    gender int,
    egg_step int,
    character_origin int,
    "position" int,
    place int
);
CREATE TABLE monster_buff (
    id int PRIMARY KEY,
    monster int,
    buff int,
    level int
);
CREATE TABLE monster_skill (
    id int PRIMARY KEY,
    monster int,
    skill int,
    level int,
    endurance int
);
CREATE TABLE recipe (
    id int PRIMARY KEY,
    "character" int,
    recipe int
);
CREATE TABLE reputation (
    id int PRIMARY KEY,
    "character" int,
    type int,
    point bigint,
    level int
);
CREATE TABLE server_time (
    id int PRIMARY KEY,
    server int,
    account int,
    played_time int,
    last_connect int
);

CREATE INDEX ON server_time (account);
CREATE INDEX ON "character" (account);
CREATE INDEX ON "character" (pseudo);
CREATE INDEX ON "character" (clan);

CREATE INDEX ON item ("character");
CREATE INDEX ON item_warehouse ("character");
CREATE INDEX ON character_allow ("character");
CREATE INDEX ON monster_buff ("monster");
CREATE INDEX ON monster_skill ("monster");
CREATE INDEX ON recipe ("character");
CREATE INDEX ON reputation ("character");

CREATE CUSTOM INDEX ON "character" (pseudo,clan);
CREATE CUSTOM INDEX ON monster ("character", place);

CREATE CUSTOM INDEX ON "character_allow" ("character", "allow");
CREATE CUSTOM INDEX ON item ("character", "item");
CREATE CUSTOM INDEX ON item_warehouse ("character", "item");
CREATE CUSTOM INDEX ON monster_buff ("monster", "buff");
CREATE CUSTOM INDEX ON monster_skill ("monster", "skill");
CREATE CUSTOM INDEX ON recipe ("character", "recipe");
CREATE CUSTOM INDEX ON reputation ("character", type);
CREATE CUSTOM INDEX ON server_time (server, account);
