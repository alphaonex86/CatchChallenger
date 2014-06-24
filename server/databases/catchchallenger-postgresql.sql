--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET lock_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

--
-- Name: postgres; Type: SCHEMA; Schema: -; Owner: postgres
--

CREATE SCHEMA postgres;


ALTER SCHEMA postgres OWNER TO postgres;

--
-- Name: SCHEMA postgres; Type: COMMENT; Schema: -; Owner: postgres
--

COMMENT ON SCHEMA postgres IS 'catchchallenger MMORPG';


SET search_path = postgres, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: account; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE account (
    id integer NOT NULL,
    login character varying(56),
    password character varying(56),
    date integer,
    email text
);


ALTER TABLE postgres.account OWNER TO postgres;

--
-- Name: account_register; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE account_register (
    id integer NOT NULL,
    login character varying(128),
    password character varying(128),
    email character varying(64),
    key text,
    date integer
);


ALTER TABLE postgres.account_register OWNER TO postgres;

--
-- Name: bot_already_beaten; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE bot_already_beaten (
    "character" integer,
    botfight_id integer
);


ALTER TABLE postgres.bot_already_beaten OWNER TO postgres;

--
-- Name: character; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE "character" (
    id integer NOT NULL,
    account integer,
    pseudo character varying(20),
    skin character varying(16),
    x smallint,
    y smallint,
    orientation public.orientation,
    map text,
    type public.player_type,
    clan integer,
    rescue_map text,
    rescue_x smallint,
    rescue_y smallint,
    rescue_orientation public.orientation,
    unvalidated_rescue_map text,
    unvalidated_rescue_x smallint,
    unvalidated_rescue_y smallint,
    unvalidated_rescue_orientation public.orientation,
    allow text,
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


ALTER TABLE postgres."character" OWNER TO postgres;

--
-- Name: city; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE city (
    city character varying(64) NOT NULL,
    clan integer
);


ALTER TABLE postgres.city OWNER TO postgres;

--
-- Name: clan; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE clan (
    id integer NOT NULL,
    name text,
    cash bigint,
    date integer
);


ALTER TABLE postgres.clan OWNER TO postgres;

--
-- Name: factory; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE factory (
    id integer NOT NULL,
    resources text,
    products text,
    last_update integer
);


ALTER TABLE postgres.factory OWNER TO postgres;

--
-- Name: item; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE item (
    item integer,
    "character" integer,
    quantity integer,
    place public.place,
    market_price bigint
);


ALTER TABLE postgres.item OWNER TO postgres;

--
-- Name: monster; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE monster (
    id integer,
    hp smallint,
    "character" integer,
    monster smallint,
    level smallint,
    xp integer,
    sp integer,
    captured_with integer,
    gender public.gender,
    egg_step integer,
    character_origin integer,
    place public.place,
    "position" smallint,
    market_price bigint
);


ALTER TABLE postgres.monster OWNER TO postgres;

--
-- Name: monster_buff; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE monster_buff (
    monster integer,
    buff smallint,
    level smallint
);


ALTER TABLE postgres.monster_buff OWNER TO postgres;

--
-- Name: monster_skill; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE monster_skill (
    monster integer,
    skill smallint,
    level smallint,
    endurance smallint
);


ALTER TABLE postgres.monster_skill OWNER TO postgres;

--
-- Name: plant; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE plant (
    id integer NOT NULL,
    map text,
    x smallint,
    y smallint,
    plant smallint,
    "character" integer,
    plant_timestamps integer
);


ALTER TABLE postgres.plant OWNER TO postgres;

--
-- Name: quest; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE quest (
    "character" integer,
    quest smallint,
    finish_one_time boolean,
    step smallint
);


ALTER TABLE postgres.quest OWNER TO postgres;

--
-- Name: recipe; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE recipe (
    "character" integer,
    recipe smallint
);


ALTER TABLE postgres.recipe OWNER TO postgres;

--
-- Name: reputation; Type: TABLE; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE TABLE reputation (
    "character" integer,
    type character varying(16),
    point bigint,
    level smallint
);


ALTER TABLE postgres.reputation OWNER TO postgres;

--
-- Name: account_login_key; Type: CONSTRAINT; Schema: postgres; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY account
    ADD CONSTRAINT account_login_key UNIQUE (login);


--
-- Name: account_pkey; Type: CONSTRAINT; Schema: postgres; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY account
    ADD CONSTRAINT account_pkey PRIMARY KEY (id);


--
-- Name: account_register_login_key; Type: CONSTRAINT; Schema: postgres; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY account_register
    ADD CONSTRAINT account_register_login_key UNIQUE (login);


--
-- Name: account_register_pkey; Type: CONSTRAINT; Schema: postgres; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY account_register
    ADD CONSTRAINT account_register_pkey PRIMARY KEY (id);


--
-- Name: character_pkey; Type: CONSTRAINT; Schema: postgres; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY "character"
    ADD CONSTRAINT character_pkey PRIMARY KEY (id);


--
-- Name: city_pkey; Type: CONSTRAINT; Schema: postgres; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY city
    ADD CONSTRAINT city_pkey PRIMARY KEY (city);


--
-- Name: clan_pkey; Type: CONSTRAINT; Schema: postgres; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY clan
    ADD CONSTRAINT clan_pkey PRIMARY KEY (id);


--
-- Name: factory_pkey; Type: CONSTRAINT; Schema: postgres; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY factory
    ADD CONSTRAINT factory_pkey PRIMARY KEY (id);


--
-- Name: plant_pkey; Type: CONSTRAINT; Schema: postgres; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY plant
    ADD CONSTRAINT plant_pkey PRIMARY KEY (id);


--
-- Name: bot_already_beaten_by_character; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX bot_already_beaten_by_character ON bot_already_beaten USING btree ("character");


--
-- Name: bot_already_beaten_unique; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX bot_already_beaten_unique ON bot_already_beaten USING btree ("character", botfight_id);


--
-- Name: character_account; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX character_account ON "character" USING btree (account);


--
-- Name: character_bypseudo; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX character_bypseudo ON "character" USING btree (pseudo);


--
-- Name: character_bypseudoandclan; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX character_bypseudoandclan ON "character" USING btree (pseudo, clan);


--
-- Name: character_clan; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX character_clan ON "character" USING btree (clan);


--
-- Name: index_by_place; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX index_by_place ON item USING btree (place);


--
-- Name: item_by_char; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX item_by_char ON item USING btree ("character");


--
-- Name: item_uniqueindex; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX item_uniqueindex ON item USING btree (item, "character", place);


--
-- Name: monster_bychar; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX monster_bychar ON monster USING btree ("character");


--
-- Name: monster_byplace; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX monster_byplace ON monster USING btree (place);


--
-- Name: monster_unique; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX monster_unique ON monster USING btree (id);


--
-- Name: monsterbuff_bymonster; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX monsterbuff_bymonster ON monster_buff USING btree (monster);


--
-- Name: monsterbuff_bymonsterandbuff; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX monsterbuff_bymonsterandbuff ON monster_buff USING btree (monster, buff);


--
-- Name: monsterskill_bymonster; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX monsterskill_bymonster ON monster_skill USING btree (monster);


--
-- Name: monsterskill_unique; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX monsterskill_unique ON monster_skill USING btree (monster, skill);


--
-- Name: quest_bychar; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX quest_bychar ON quest USING btree ("character");


--
-- Name: quest_unique; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX quest_unique ON quest USING btree ("character", quest);


--
-- Name: recipe_bychar; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX recipe_bychar ON recipe USING btree ("character");


--
-- Name: recipe_unique; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX recipe_unique ON recipe USING btree ("character", recipe);


--
-- Name: reputation_bychar; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE INDEX reputation_bychar ON reputation USING btree ("character");


--
-- Name: reputation_unique; Type: INDEX; Schema: postgres; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX reputation_unique ON reputation USING btree ("character", type);


--
-- PostgreSQL database dump complete
--
