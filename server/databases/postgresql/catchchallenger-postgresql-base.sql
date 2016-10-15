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
-- Name: dictionary_allow; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE dictionary_allow (
    id integer NOT NULL,
    allow text NOT NULL
);


--
-- Name: dictionary_reputation; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE dictionary_reputation (
    id integer NOT NULL,
    reputation text NOT NULL
);


--
-- Name: dictionary_skin; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE dictionary_skin (
    id integer NOT NULL,
    skin text NOT NULL
);


--
-- Name: dictionary_starter; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE dictionary_starter (
    id integer NOT NULL,
    starter text NOT NULL
);

--
-- Name: dictionary_allow_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY dictionary_allow
    ADD CONSTRAINT dictionary_allow_pkey PRIMARY KEY (id);


--
-- Name: dictionary_reputation_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY dictionary_reputation
    ADD CONSTRAINT dictionary_reputation_pkey PRIMARY KEY (id);

--
-- Name: dictionary_skin_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY dictionary_skin
    ADD CONSTRAINT dictionary_skin_pkey PRIMARY KEY (id);


--
-- Name: dictionary_starter_pkey; Type: CONSTRAINT; Schema: public; Owner: root; Tablespace: 
--

ALTER TABLE ONLY dictionary_starter
    ADD CONSTRAINT dictionary_starter_pkey PRIMARY KEY (id);


REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--
