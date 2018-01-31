--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;
SET row_security = off;

SET search_path = public, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: character; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE "character" (
    id integer NOT NULL,
    account integer,
    pseudo character varying(20),
    skin smallint,
    type smallint,
    clan integer,
    clan_leader boolean,
    date bigint,
    cash bigint,
    warehouse_cash bigint,
    time_to_delete integer,
    played_time integer,
    last_connect integer,
    starter smallint,
    allow bytea,
    item bytea,
    item_warehouse bytea,
    recipes bytea,
    reputations bytea,
    encyclopedia_monster bytea,
    encyclopedia_item bytea,
    achievements bytea,
    blob_version smallint,
    lastdaillygift bigint
);


ALTER TABLE "character" OWNER TO postgres;

--
-- Name: clan; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE clan (
    id integer NOT NULL,
    name text,
    cash bigint,
    date bigint
);


ALTER TABLE clan OWNER TO postgres;

--
-- Name: monster; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE monster (
    id integer NOT NULL,
    "character" integer,
    place smallint,
    hp smallint,
    monster smallint,
    level smallint,
    xp integer,
    sp integer,
    captured_with smallint,
    gender smallint,
    egg_step integer,
    character_origin integer,
    "position" smallint,
    buffs bytea,
    skills bytea,
    skills_endurance bytea
);


ALTER TABLE monster OWNER TO postgres;

--
-- Name: server_time; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE server_time (
    server smallint NOT NULL,
    account integer NOT NULL,
    played_time integer,
    last_connect integer
);


ALTER TABLE server_time OWNER TO postgres;

--
-- Name: character character_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY "character"
    ADD CONSTRAINT character_pkey PRIMARY KEY (id);


--
-- Name: clan clan_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY clan
    ADD CONSTRAINT clan_pkey PRIMARY KEY (id);


--
-- Name: monster monster_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY monster
    ADD CONSTRAINT monster_pkey PRIMARY KEY (id);


--
-- Name: server_time server_time_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY server_time
    ADD CONSTRAINT server_time_pkey PRIMARY KEY (server, account);


--
-- Name: character_account; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX character_account ON "character" USING btree (account);


--
-- Name: character_bypseudo; Type: INDEX; Schema: public; Owner: postgres
--

CREATE UNIQUE INDEX character_bypseudo ON "character" USING btree (pseudo);


--
-- Name: character_clan; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX character_clan ON "character" USING btree (clan) WHERE (clan > 0);


--
-- Name: monster_by_character; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX monster_by_character ON monster USING btree ("character");


--
-- Name: server_time_by_account; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX server_time_by_account ON server_time USING btree (account);


--
-- PostgreSQL database dump complete
--
