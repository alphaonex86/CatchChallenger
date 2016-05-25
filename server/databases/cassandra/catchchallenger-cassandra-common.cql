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
    starter int,
    allow bytea,
    item bytea,
    item_warehouse bytea,
    recipes bytea,
    reputations bytea,
    encyclopedia_monster bytea,
    encyclopedia_item bytea,
    achievements bytea,
    blob_version smallint
);
CREATE TABLE clan (
    id int PRIMARY KEY,
    name text,
    cash bigint,
    date bigint
);
CREATE TABLE monster (
    id int PRIMARY KEY,
    "character" int,
    place int,
    hp int,
    monster int,
    level int,
    xp int,
    sp int,
    captured_with int,
    gender int,
    egg_step int,
    character_origin int,
    "position" int,
    buffs bytea,
    skills bytea,
    skills_endurance bytea
);
CREATE TABLE server_time (
    id int PRIMARY KEY,
    server int,
    account int,
    played_time int,
    last_connect int
);

CREATE INDEX ON "character" (id);
CREATE INDEX ON "clan" (id);
CREATE INDEX ON "monster" (id);
-- CREATE CUSTOM INDEX ON server_time (server, account);

CREATE INDEX ON server_time (account);
CREATE INDEX ON "character" (account);
CREATE INDEX ON "character" (pseudo);
--do partial
CREATE INDEX ON "character" (clan);
CREATE INDEX ON monster ("character");
