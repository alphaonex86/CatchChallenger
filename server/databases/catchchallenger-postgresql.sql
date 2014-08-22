SET statement_timeout = 0;
SET lock_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

SET search_path = public, pg_catalog;

--
-- Name: gender; Type: TYPE; Schema: public; Owner: root
--

CREATE TYPE gender AS ENUM (
    'unknown',
    'male',
    'female'
);

--
-- Name: orientation; Type: TYPE; Schema: public; Owner: root
--

CREATE TYPE orientation AS ENUM (
    'top',
    'bottom',
    'left',
    'right'
);


--
-- Name: place; Type: TYPE; Schema: public; Owner: root
--

CREATE TYPE place AS ENUM (
    'wear',
    'warehouse',
    'market'
);

--
-- Name: player_type; Type: TYPE; Schema: public; Owner: root
--

CREATE TYPE player_type AS ENUM (
    'normal',
    'premium',
    'gm',
    'dev'
);


SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: account; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE account (
    id integer NOT NULL,
    login character varying(56),
    password character varying(56),
    date integer,
    email text
);


--
-- Name: account_register; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE account_register (
    login character varying(128),
    password character varying(128),
    email character varying(64),
    key text,
    date integer
);


--
-- Name: bot_already_beaten; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE bot_already_beaten (
    "character" integer,
    botfight_id smallint
);


--
-- Name: character; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE "character" (
    id integer NOT NULL,
    account integer,
    pseudo character varying(20),
    skin smallint,
    x smallint,
    y smallint,
    orientation smallint,
    map smallint,
    type smallint,
    clan integer,
    rescue_map smallint,
    rescue_x smallint,
    rescue_y smallint,
    rescue_orientation smallint,
    unvalidated_rescue_map smallint,
    unvalidated_rescue_x smallint,
    unvalidated_rescue_y smallint,
    unvalidated_rescue_orientation smallint,
    clan_leader boolean,
    date integer,
    cash bigint,
    warehouse_cash bigint,
    market_cash bigint,
    time_to_delete integer,
    played_time integer,
    last_connect integer,
    starter smallint
);


--
-- Name: character_allow; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE character_allow (
    "character" integer,
    allow smallint
);


--
-- Name: character_itemOnMap; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE "character_itemonmap" (
    "character" integer,
    "itemOnMap" smallint
);


--
-- Name: city; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE city (
    city character varying(64) NOT NULL,
    clan integer
);


--
-- Name: clan; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE clan (
    id integer NOT NULL,
    name text,
    cash bigint,
    date integer
);


--
-- Name: dictionary_allow; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE dictionary_allow (
    id integer NOT NULL,
    allow text
);


--
-- Name: dictionary_itemOnMap; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE "dictionary_itemonmap" (
    id integer NOT NULL,
    map text,
    x smallint,
    y smallint
);


--
-- Name: dictionary_map; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE dictionary_map (
    id integer NOT NULL,
    map text
);


--
-- Name: dictionary_reputation; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE dictionary_reputation (
    id integer NOT NULL,
    reputation text
);


--
-- Name: dictionary_skin; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE dictionary_skin (
    id integer NOT NULL,
    skin text
);


--
-- Name: factory; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE factory (
    id integer NOT NULL,
    resources text,
    products text,
    last_update integer
);


--
-- Name: item; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE item (
    item smallint,
    "character" integer,
    quantity integer
);


--
-- Name: item_market; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE item_market (
    item smallint,
    "character" integer,
    quantity integer,
    market_price bigint
);


--
-- Name: item_warehouse; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE item_warehouse (
    item smallint,
    "character" integer,
    quantity integer
);


--
-- Name: monster; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE monster (
    id integer,
    hp smallint,
    "character" integer,
    monster smallint,
    level smallint,
    xp integer,
    sp integer,
    captured_with smallint,
    gender smallint,
    egg_step integer,
    character_origin integer,
    "position" smallint
);


--
-- Name: monster_buff; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE monster_buff (
    monster integer,
    buff smallint,
    level smallint
);


--
-- Name: monster_market; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE monster_market (
    id integer,
    hp smallint,
    "character" integer,
    monster smallint,
    level smallint,
    xp integer,
    sp integer,
    captured_with smallint,
    gender smallint,
    egg_step integer,
    character_origin integer,
    market_price bigint
);


--
-- Name: monster_skill; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE monster_skill (
    monster integer,
    skill smallint,
    level smallint,
    endurance smallint
);


--
-- Name: monster_warehouse; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE monster_warehouse (
    id integer,
    hp smallint,
    "character" integer,
    monster smallint,
    level smallint,
    xp integer,
    sp integer,
    captured_with smallint,
    gender smallint,
    egg_step integer,
    character_origin integer,
    "position" smallint
);


--
-- Name: plant; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE plant (
    id integer NOT NULL,
    map smallint,
    x smallint,
    y smallint,
    plant smallint,
    "character" integer,
    plant_timestamps integer
);


--
-- Name: quest; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE quest (
    "character" integer,
    quest smallint,
    finish_one_time boolean,
    step smallint
);


--
-- Name: recipe; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE recipe (
    "character" integer,
    recipe smallint
);


--
-- Name: reputation; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE reputation (
    "character" integer,
    type smallint,
    point bigint,
    level smallint
);


--
-- Name: account_login_key; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY account
    ADD CONSTRAINT account_login_key UNIQUE (login);


--
-- Name: account_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY account
    ADD CONSTRAINT account_pkey PRIMARY KEY (id);


--
-- Name: account_register_login_key; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY account_register
    ADD CONSTRAINT account_register_login_key UNIQUE (login);


--
-- Name: character_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY "character"
    ADD CONSTRAINT character_pkey PRIMARY KEY (id);


--
-- Name: city_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY city
    ADD CONSTRAINT city_pkey PRIMARY KEY (city);


--
-- Name: clan_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY clan
    ADD CONSTRAINT clan_pkey PRIMARY KEY (id);


--
-- Name: dictionary_allow_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY dictionary_allow
    ADD CONSTRAINT dictionary_allow_pkey PRIMARY KEY (id);


--
-- Name: dictionary_itemOnMap_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY "dictionary_itemonmap"
    ADD CONSTRAINT "dictionary_itemOnMap_pkey" PRIMARY KEY (id);


--
-- Name: dictionary_map_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY dictionary_map
    ADD CONSTRAINT dictionary_map_pkey PRIMARY KEY (id);


--
-- Name: dictionary_reputation_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY dictionary_reputation
    ADD CONSTRAINT dictionary_reputation_pkey PRIMARY KEY (id);


--
-- Name: dictionary_skin_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY dictionary_skin
    ADD CONSTRAINT dictionary_skin_pkey PRIMARY KEY (id);


--
-- Name: factory_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY factory
    ADD CONSTRAINT factory_pkey PRIMARY KEY (id);


--
-- Name: plant_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY plant
    ADD CONSTRAINT plant_pkey PRIMARY KEY (id);


--
-- Name: bot_already_beaten_by_character; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX bot_already_beaten_by_character ON bot_already_beaten USING btree ("character");


--
-- Name: bot_already_beaten_unique; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX bot_already_beaten_unique ON bot_already_beaten USING btree ("character", botfight_id);


--
-- Name: character_account; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX character_account ON "character" USING btree (account);


--
-- Name: character_allow_by_character; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX character_allow_by_character ON character_allow USING btree ("character");


--
-- Name: character_bypseudo; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX character_bypseudo ON "character" USING btree (pseudo);


--
-- Name: character_bypseudoandclan; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX character_bypseudoandclan ON "character" USING btree (pseudo, clan);


--
-- Name: character_clan; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX character_clan ON "character" USING btree (clan);


--
-- Name: character_itemOnMap_index; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX "character_itemOnMap_index" ON "character_itemonmap" USING btree ("character");


--
-- Name: item_by_char; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX item_by_char ON item USING btree ("character");


--
-- Name: item_market_uniqueindex; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX item_market_uniqueindex ON item_market USING btree (item, "character");


--
-- Name: item_uniqueindex; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX item_uniqueindex ON item USING btree (item, "character");


--
-- Name: item_warehouse_by_char; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX item_warehouse_by_char ON item_warehouse USING btree ("character");


--
-- Name: item_warehouse_uniqueindex; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX item_warehouse_uniqueindex ON item_warehouse USING btree (item, "character");


--
-- Name: monster_bychar; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX monster_bychar ON monster USING btree ("character");


--
-- Name: monster_market_uniqueid; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX monster_market_uniqueid ON monster_market USING btree (id);


--
-- Name: monster_unique; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX monster_unique ON monster USING btree (id);


--
-- Name: monster_warehouse_bychar; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX monster_warehouse_bychar ON monster_warehouse USING btree ("character");


--
-- Name: monster_warehouse_unique; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX monster_warehouse_unique ON monster_warehouse USING btree (id);


--
-- Name: monsterbuff_bymonster; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX monsterbuff_bymonster ON monster_buff USING btree (monster);


--
-- Name: monsterbuff_bymonsterandbuff; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX monsterbuff_bymonsterandbuff ON monster_buff USING btree (monster, buff);


--
-- Name: monsterskill_bymonster; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX monsterskill_bymonster ON monster_skill USING btree (monster);


--
-- Name: monsterskill_unique; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX monsterskill_unique ON monster_skill USING btree (monster, skill);


--
-- Name: quest_bychar; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX quest_bychar ON quest USING btree ("character");


--
-- Name: quest_unique; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX quest_unique ON quest USING btree ("character", quest);


--
-- Name: recipe_bychar; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX recipe_bychar ON recipe USING btree ("character");


--
-- Name: recipe_unique; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX recipe_unique ON recipe USING btree ("character", recipe);


--
-- Name: reputation_bychar; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX reputation_bychar ON reputation USING btree ("character");


--
-- Name: reputation_unique; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX reputation_unique ON reputation USING btree ("character", type);


--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--
