CREATE TABLE bot_already_beaten (
    "character" int,
    botfight_id int,
    PRIMARY KEY ("character", botfight_id)
);
CREATE TABLE character_forserver (
    "character" int PRIMARY KEY,
    map int,
    x int,
    y int,
    orientation int,
    rescue_map int,
    rescue_x int,
    rescue_y int,
    rescue_orientation int,
    unvalidated_rescue_map int,
    unvalidated_rescue_x int,
    unvalidated_rescue_y int,
    unvalidated_rescue_orientation int,
    date bigint,
    market_cash bigint
);
CREATE TABLE character_itemonmap (
    "character" int,
    "pointOnMap" int,
    PRIMARY KEY ("character", "pointOnMap")
);
CREATE TABLE city (
    city text PRIMARY KEY,
    clan int
);
CREATE TABLE dictionary_pointonmap (
    id int PRIMARY KEY,
    map int,
    x int,
    y int
);
CREATE TABLE dictionary_map (
    id int PRIMARY KEY,
    map text
);
CREATE TABLE factory (
    id int PRIMARY KEY,
    resources text,
    products text,
    last_update bigint
);
CREATE TABLE item_market (
    item int,
    "character" int,
    quantity int,
    market_price bigint,
    PRIMARY KEY ("character", item)
);
CREATE TABLE monster_market_price (
    id int PRIMARY KEY,
    market_price int
);
CREATE TABLE plant (
    "character" int,
    "pointOnMap" int,
    plant int,
    plant_timestamps int,
    PRIMARY KEY ("character", "pointOnMap")
);
CREATE TABLE quest (
    "character" int,
    quest int,
    finish_one_time boolean,
    step int,
    PRIMARY KEY ("character", quest)
);

CREATE INDEX ON plant ("character");
CREATE INDEX ON bot_already_beaten ("character");
CREATE INDEX ON character_forserver ("character");
CREATE INDEX ON character_itemonmap ("character");
CREATE INDEX ON quest ("character");
