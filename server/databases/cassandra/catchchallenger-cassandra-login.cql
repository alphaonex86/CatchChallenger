CREATE TABLE account (
    id int,
    login blob PRIMARY KEY,
    password blob,
    date bigint,
    email text
);
CREATE TABLE account_register (
    login blob PRIMARY KEY,
    password blob,
    email text,
    key text,
    date bigint
);
CREATE INDEX ON account (id);
