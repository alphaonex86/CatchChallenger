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
-- Name: character_forserver; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE character_forserver (
    "character" integer NOT NULL,
    map smallint,
    x smallint,
    y smallint,
    orientation smallint,
    rescue_map smallint,
    rescue_x smallint,
    rescue_y smallint,
    rescue_orientation smallint,
    unvalidated_rescue_map smallint,
    unvalidated_rescue_x smallint,
    unvalidated_rescue_y smallint,
    unvalidated_rescue_orientation smallint,
    date bigint,
    market_cash bigint,
    botfight_id bytea,
    itemonmap bytea,
    plants bytea,
    quest bytea,
    blob_version smallint
);


--
-- Name: city; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE city (
    city character varying(64) NOT NULL,
    clan integer
);



--
-- Name: dictionary_pointonmap; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE dictionary_pointonmap (
    id integer NOT NULL,
    map integer,
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
    last_update bigint
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
    market_price integer,
    "character" integer
);



--
-- Name: plant; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE plant (
    "character" integer,
    "pointOnMap" smallint,
    plant smallint,
    plant_timestamps integer
);

--
-- Name: character_forserver_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY character_forserver
    ADD CONSTRAINT character_forserver_pkey PRIMARY KEY ("character");


--
-- Name: dictionary_itemOnMap_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY dictionary_pointonmap
    ADD CONSTRAINT "dictionary_pointOnMap_pkey" PRIMARY KEY (id);


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
-- Name: plant_by_char; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX plant_by_char ON plant USING btree ("character");


--
-- Name: plant_unique; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX plant_unique ON plant USING btree ("character", "pointOnMap");

--
-- Name: character_character_forserver_index; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX character_character_forserver_index ON character_forserver USING btree ("character");

--
-- Name: item_market_uniqueindex; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX item_market_uniqueindex ON item_market USING btree ("character", item);


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
