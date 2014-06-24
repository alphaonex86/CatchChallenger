ALTER TABLE  `account` CHANGE  `login`  `login` VARBINARY(28) NOT NULL ,CHANGE  `password`  `password` VARBINARY(28) NOT NULL;

ALTER TABLE  `plant` CHANGE  `map`  `map` VARCHAR( 64 ) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL;