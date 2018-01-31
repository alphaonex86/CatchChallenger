SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

ALTER TABLE `character` ADD `lastdaillygift` BIGINT NOT NULL AFTER `blob_version`;
