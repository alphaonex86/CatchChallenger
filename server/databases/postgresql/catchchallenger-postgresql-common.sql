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
    account integer,
    pseudo character varying(20),
    skin smallint,
    type smallint,
    clan integer,
    clan_leader boolean,
    date integer,
    cash bigint,
    warehouse_cash bigint,
    time_to_delete integer,
    played_time integer,
    last_connect integer,
    starter smallint
);


ALTER TABLE public."character" OWNER TO root;

--
-- Name: character_allow; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE character_allow (
    "character" integer,
    allow smallint
);


ALTER TABLE public.character_allow OWNER TO root;

--
-- Name: clan; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE clan (
    id integer NOT NULL,
    name text,
    cash bigint,
    date integer
);


ALTER TABLE public.clan OWNER TO root;

--
-- Name: dictionary_server; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE dictionary_server (
    id integer NOT NULL,
    key text
);


ALTER TABLE public.dictionary_server OWNER TO root;

--
-- Name: item; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE item (
    item smallint,
    "character" integer,
    quantity integer
);


ALTER TABLE public.item OWNER TO root;

--
-- Name: item_warehouse; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE item_warehouse (
    item smallint,
    "character" integer,
    quantity integer
);


ALTER TABLE public.item_warehouse OWNER TO root;

--
-- Name: monster; Type: TABLE; Schema: public; Owner: root; Tablespace: 
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
    "position" smallint,
    place smallint
);


ALTER TABLE public.monster OWNER TO root;

--
-- Name: monster_buff; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE monster_buff (
    monster integer,
    buff smallint,
    level smallint
);


ALTER TABLE public.monster_buff OWNER TO root;

--
-- Name: monster_skill; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE monster_skill (
    monster integer,
    skill smallint,
    level smallint,
    endurance smallint
);


ALTER TABLE public.monster_skill OWNER TO root;

--
-- Name: recipe; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE recipe (
    "character" integer,
    recipe smallint
);


ALTER TABLE public.recipe OWNER TO root;

--
-- Name: reputation; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE reputation (
    "character" integer,
    type smallint,
    point bigint,
    level smallint
);


ALTER TABLE public.reputation OWNER TO root;

--
-- Name: server_time; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE server_time (
    server smallint NOT NULL,
    account integer NOT NULL,
    played_time integer,
    last_connect integer
);


ALTER TABLE public.server_time OWNER TO root;

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


--
-- Name: dictionary_server_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY dictionary_server
    ADD CONSTRAINT dictionary_server_pkey PRIMARY KEY (id);


--
-- Name: server_time_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY server_time
    ADD CONSTRAINT server_time_pkey PRIMARY KEY (server, account);


--
-- Name: character_account; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX character_account ON "character" USING btree (account);


--
-- Name: character_allow_by_character; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX character_allow_by_character ON character_allow USING btree ("character");


--
-- Name: character_bypseudo; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX character_bypseudo ON "character" USING btree (pseudo);


--
-- Name: character_bypseudoandclan; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX character_bypseudoandclan ON "character" USING btree (pseudo, clan);


--
-- Name: character_clan; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX character_clan ON "character" USING btree (clan);


--
-- Name: item_by_char; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX item_by_char ON item USING btree ("character");


--
-- Name: item_uniqueindex; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX item_uniqueindex ON item USING btree (item, "character");


--
-- Name: item_warehouse_by_char; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX item_warehouse_by_char ON item_warehouse USING btree ("character");


--
-- Name: item_warehouse_uniqueindex; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX item_warehouse_uniqueindex ON item_warehouse USING btree (item, "character");


--
-- Name: monster_bychar; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX monster_bychar ON monster USING btree ("character");


--
-- Name: monster_unique; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX monster_unique ON monster USING btree (id);


--
-- Name: monsterbuff_bymonster; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX monsterbuff_bymonster ON monster_buff USING btree (monster);


--
-- Name: monsterbuff_bymonsterandbuff; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX monsterbuff_bymonsterandbuff ON monster_buff USING btree (monster, buff);


--
-- Name: monsterskill_bymonster; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX monsterskill_bymonster ON monster_skill USING btree (monster);


--
-- Name: monsterskill_unique; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX monsterskill_unique ON monster_skill USING btree (monster, skill);


--
-- Name: recipe_bychar; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX recipe_bychar ON recipe USING btree ("character");


--
-- Name: recipe_unique; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX recipe_unique ON recipe USING btree ("character", recipe);


--
-- Name: reputation_bychar; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX reputation_bychar ON reputation USING btree ("character");


--
-- Name: reputation_unique; Type: INDEX; Schema: public; Owner: root; Tablespace: 
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
