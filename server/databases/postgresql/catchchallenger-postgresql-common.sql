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
-- Name: character; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE "character" (
    id integer NOT NULL,
    account integer NOT NULL,
    pseudo character varying(20) NOT NULL,
    skin smallint NOT NULL,
    type smallint NOT NULL,
    clan integer NOT NULL,
    clan_leader boolean NOT NULL,
    date bigint NOT NULL,
    cash bigint NOT NULL,
    warehouse_cash bigint NOT NULL,
    time_to_delete integer NOT NULL,
    played_time integer NOT NULL,
    last_connect integer NOT NULL,
    starter smallint NOT NULL,
    allow bytea,
    item bytea,
    item_warehouse bytea,
    recipes bytea,
    reputations bytea,
    encyclopedia_monster bytea,
    encyclopedia_item bytea,
    achievements bytea,
    blob_version smallint NOT NULL
);


--
-- Name: clan; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE clan (
    id integer NOT NULL,
    name text NOT NULL,
    cash bigint NOT NULL,
    date bigint NOT NULL
);


CREATE TABLE monster (
    id integer NOT NULL,
    "character" integer NOT NULL,
    place smallint NOT NULL,
    hp smallint NOT NULL,
    monster smallint NOT NULL,
    level smallint NOT NULL,
    xp integer NOT NULL,
    sp integer,
    captured_with smallint NOT NULL,
    gender smallint NOT NULL,
    egg_step integer NOT NULL,
    character_origin integer NOT NULL,
    "position" smallint NOT NULL,
    buffs bytea NOT NULL,
    skills bytea NOT NULL,
    skills_endurance bytea NOT NULL
);

--
-- Name: server_time; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE server_time (
    server smallint NOT NULL,
    account integer NOT NULL,
    played_time integer NOT NULL,
    last_connect integer NOT NULL
);


--
-- Name: character_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY "character"
    ADD CONSTRAINT character_pkey PRIMARY KEY (id);


--
-- Name: clan_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY clan
    ADD CONSTRAINT clan_pkey PRIMARY KEY (id);

ALTER TABLE ONLY monster ADD CONSTRAINT monster_pkey PRIMARY KEY (id);


--
-- Name: server_time_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY server_time
    ADD CONSTRAINT server_time_pkey PRIMARY KEY (server, account);


--
-- Name: server_time_by_account; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX server_time_by_account ON server_time USING btree (account);


--
-- Name: character_account; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX character_account ON "character" USING btree (account);


-- Name: character_bypseudo; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX character_bypseudo ON "character" USING btree (pseudo);


--
-- Name: character_clan; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX character_clan ON "character" USING btree ("clan") WHERE clan>0;

--
-- Name: monster_unique; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX monster_by_character ON monster USING btree ("character");

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
