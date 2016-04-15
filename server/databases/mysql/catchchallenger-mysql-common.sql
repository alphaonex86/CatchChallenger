SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

--
-- Database: `catchchallenger_common`
--

-- --------------------------------------------------------

--
-- Table structure for table `character`
--

CREATE TABLE IF NOT EXISTS `character` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `account` int(11) NOT NULL,
  `pseudo` varchar(20) NOT NULL,
  `skin` int(11) NOT NULL,
  `type` smallint(6) NOT NULL,
  `clan` int(11) NOT NULL,
  `clan_leader` tinyint(1) NOT NULL,
  `date` int(11) unsigned NOT NULL,
  `cash` bigint(20) unsigned NOT NULL,
  `warehouse_cash` bigint(20) unsigned NOT NULL,
  `time_to_delete` int(11) unsigned NOT NULL,
  `played_time` int(11) unsigned NOT NULL,
  `last_connect` int(11) unsigned NOT NULL,
  `starter` tinyint(4) unsigned NOT NULL,
  `allow` tinyblob NOT NULL,
  `item` blob NOT NULL,
  `item_warehouse` mediumblob NOT NULL,
  `recipes` blob NOT NULL,
  `reputations` blob NOT NULL,
  `encyclopedia_monster` mediumblob NOT NULL,
  `encyclopedia_item` mediumblob NOT NULL,
  `achievements` tinyblob NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `pseudo` (`pseudo`,`clan`),
  UNIQUE KEY `pseudo_2` (`pseudo`),
  KEY `clan` (`clan`),
  KEY `account` (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `clan`
--

CREATE TABLE IF NOT EXISTS `clan` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` text NOT NULL,
  `cash` bigint(20) unsigned NOT NULL,
  `date` int(11) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `monster`
--

CREATE TABLE IF NOT EXISTS `monster` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `character` int(11) unsigned NOT NULL,
  `place` smallint(5) unsigned NOT NULL,
  `hp` smallint(5) unsigned NOT NULL,
  `monster` smallint(5) unsigned NOT NULL,
  `level` tinyint(3) unsigned NOT NULL,
  `xp` int(11) unsigned NOT NULL,
  `sp` int(11) unsigned NOT NULL,
  `captured_with` smallint(5) unsigned NOT NULL,
  `gender` smallint(6) NOT NULL,
  `egg_step` int(11) unsigned NOT NULL,
  `character_origin` int(11) NOT NULL,
  `position` tinyint(3) unsigned NOT NULL,
  `buffs` blob NOT NULL,
  `skills` tinyblob NOT NULL,
  `skills_endurance` tinyblob NOT NULL,
  PRIMARY KEY (`id`),
  KEY `character` (`character`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `server_time`
--

CREATE TABLE IF NOT EXISTS `server_time` (
  `server` smallint(6) NOT NULL,
  `account` int(11) NOT NULL,
  `played_time` int(11) NOT NULL,
  `last_connect` int(11) NOT NULL,
  PRIMARY KEY (`server`,`account`),
  KEY `account` (`account`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
