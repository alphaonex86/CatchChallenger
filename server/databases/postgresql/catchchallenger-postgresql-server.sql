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
    map smallint NOT NULL,
    x smallint NOT NULL,
    y smallint NOT NULL,
    orientation smallint NOT NULL,
    rescue_map smallint NOT NULL,
    rescue_x smallint NOT NULL,
    rescue_y smallint NOT NULL,
    rescue_orientation smallint NOT NULL,
    unvalidated_rescue_map smallint NOT NULL,
    unvalidated_rescue_x smallint NOT NULL,
    unvalidated_rescue_y smallint NOT NULL,
    unvalidated_rescue_orientation smallint NOT NULL,
    date bigint NOT NULL,
    botfight_id bytea NOT NULL,
    itemonmap bytea NOT NULL,
    plants bytea,
    quest bytea NOT NULL
);


CREATE TABLE character_bymap (
    "character" integer NOT NULL,
    "map" smallint NOT NULL,
    "plants" bytea NOT NULL,
    "items" bytea NOT NULL,
    "fights" bytea NOT NULL
);


--
-- Name: city; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE city (
    city character varying(64) NOT NULL,
    clan integer NOT NULL
);



--
-- Name: dictionary_map; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE dictionary_map (
    id integer NOT NULL,
    map text NOT NULL
);



--
-- Name: factory; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE factory (
    id integer NOT NULL,
    resources text NOT NULL,
    products text NOT NULL,
    last_update bigint NOT NULL
);


--
-- Name: character_forserver_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY character_forserver
    ADD CONSTRAINT character_forserver_pkey PRIMARY KEY ("character");



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
-- Name: character_character_forserver_index; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX character_character_forserver_index ON character_forserver USING btree ("character");
CREATE INDEX character_character_bymap_index ON character_bymap USING btree ("character");


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
