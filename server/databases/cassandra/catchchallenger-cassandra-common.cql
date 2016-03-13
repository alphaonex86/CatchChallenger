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
    "character" int,
    "allow" int,
    PRIMARY KEY ("character", "allow")
);
CREATE TABLE clan (
    id int PRIMARY KEY,
    name text,
    cash bigint,
    date bigint
);
CREATE TABLE item (
    item int,
    "character" int,
    quantity int,
    PRIMARY KEY ("character", "item")
);
CREATE TABLE item_warehouse (
    item int,
    "character" int,
    quantity int,
    PRIMARY KEY ("character", "item")
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
    monster int,
    buff int,
    level int,
    PRIMARY KEY ("monster", "buff")
);
CREATE TABLE monster_skill (
    monster int,
    skill int,
    level int,
    endurance int,
    PRIMARY KEY ("monster", "skill")
);
CREATE TABLE recipe (
    "character" int,
    recipe int,
    PRIMARY KEY ("character", "recipe")
);
CREATE TABLE reputation (
    "character" int,
    type int,
    point bigint,
    level int,
    PRIMARY KEY ("character", type)
);
CREATE TABLE server_time (
    server int,
    account int,
    played_time int,
    last_connect int,
    PRIMARY KEY (server, account)
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
