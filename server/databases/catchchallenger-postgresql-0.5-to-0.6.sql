CREATE TABLE "dictionary_itemonmap" (
    id integer NOT NULL,
    map text,
    x smallint,
    y smallint
);
ALTER TABLE ONLY "dictionary_itemonmap" ADD CONSTRAINT "dictionary_itemOnMap_pkey" PRIMARY KEY (id);
CREATE TABLE "character_itemonmap" (
    "character" integer,
    "itemOnMap" smallint
);
CREATE INDEX "character_itemOnMap_index" ON "character_itemonmap" USING btree ("character");