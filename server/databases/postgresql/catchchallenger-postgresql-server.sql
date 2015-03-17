--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET lock_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

SET search_path = public, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: bot_already_beaten; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE bot_already_beaten (
    "character" integer,
    botfight_id smallint
);



--
-- Name: character_forserver; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE character_forserver (
    "character" integer NOT NULL,
    x smallint,
    y smallint,
    orientation smallint,
    map smallint,
    rescue_map smallint,
    rescue_x smallint,
    rescue_y smallint,
    rescue_orientation smallint,
    unvalidated_rescue_map smallint,
    unvalidated_rescue_x smallint,
    unvalidated_rescue_y smallint,
    unvalidated_rescue_orientation smallint,
    date integer,
    market_cash bigint,
    played_time integer,
    last_connect integer
);



--
-- Name: character_itemonmap; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE character_itemonmap (
    "character" integer,
    "itemOnMap" smallint
);



--
-- Name: city; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE city (
    city character varying(64) NOT NULL,
    clan integer
);



--
-- Name: dictionary_itemonmap; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE dictionary_itemonmap (
    id integer NOT NULL,
    map text,
    x smallint,
    y smallint
);



--
-- Name: dictionary_map; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE dictionary_map (
    id integer NOT NULL,
    map text
);



--
-- Name: factory; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE factory (
    id integer NOT NULL,
    resources text,
    products text,
    last_update integer
);



--
-- Name: item_market; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE item_market (
    item smallint,
    "character" integer,
    quantity integer,
    market_price bigint
);



--
-- Name: monster_market_price; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE monster_market_price (
    id integer NOT NULL,
    market_price integer
);



--
-- Name: plant; Type: TABLE; Schema: public; Owner: root; Tablespace: 
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
-- Name: quest; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE quest (
    "character" integer,
    quest smallint,
    finish_one_time boolean,
    step smallint
);



--
-- Name: character_forserver_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY character_forserver
    ADD CONSTRAINT character_forserver_pkey PRIMARY KEY ("character");


--
-- Name: dictionary_itemOnMap_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY dictionary_itemonmap
    ADD CONSTRAINT "dictionary_itemOnMap_pkey" PRIMARY KEY (id);


--
-- Name: dictionary_map_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY dictionary_map
    ADD CONSTRAINT dictionary_map_pkey PRIMARY KEY (id);


--
-- Name: factory_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY factory
    ADD CONSTRAINT factory_pkey PRIMARY KEY (id);


--
-- Name: monster_market_price_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY monster_market_price
    ADD CONSTRAINT monster_market_price_pkey PRIMARY KEY (id);


--
-- Name: plant_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY plant
    ADD CONSTRAINT plant_pkey PRIMARY KEY (id);


--
-- Name: bot_already_beaten_by_character; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX bot_already_beaten_by_character ON bot_already_beaten USING btree ("character");


--
-- Name: bot_already_beaten_unique; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX bot_already_beaten_unique ON bot_already_beaten USING btree ("character", botfight_id);


--
-- Name: character_character_forserver_index; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX character_character_forserver_index ON character_forserver USING btree ("character");


--
-- Name: character_itemOnMap_index; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX "character_itemOnMap_index" ON character_itemonmap USING btree ("character");


--
-- Name: item_market_uniqueindex; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX item_market_uniqueindex ON item_market USING btree (item, "character");


--
-- Name: quest_bychar; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX quest_bychar ON quest USING btree ("character");


--
-- Name: quest_unique; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX quest_unique ON quest USING btree ("character", quest);


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
