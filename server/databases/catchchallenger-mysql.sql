SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

--
-- Database: `catchchallenger`
--

-- --------------------------------------------------------

--
-- Table structure for table `account`
--

CREATE TABLE IF NOT EXISTS `account` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `login` varbinary(28) NOT NULL,
  `password` varbinary(28) NOT NULL,
  `date` int(11) unsigned NOT NULL,
  `email` varchar(64) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `login` (`login`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `account_register`
--

CREATE TABLE IF NOT EXISTS `account_register` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `login` varchar(128) NOT NULL,
  `password` varchar(128) NOT NULL,
  `email` varchar(64) NOT NULL,
  `key` text NOT NULL,
  `date` int(11) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `login_2` (`login`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `bot_already_beaten`
--

CREATE TABLE IF NOT EXISTS `bot_already_beaten` (
  `character` int(11) NOT NULL,
  `botfight_id` int(11) NOT NULL,
  PRIMARY KEY (`character`,`botfight_id`),
  KEY `player_id` (`character`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `character`
--

CREATE TABLE IF NOT EXISTS `character` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `account` int(11) NOT NULL,
  `pseudo` varchar(20) NOT NULL,
  `skin` int(11) NOT NULL,
  `x` tinyint(3) unsigned NOT NULL,
  `y` tinyint(3) unsigned NOT NULL,
  `orientation` smallint(6) NOT NULL,
  `map` text NOT NULL,
  `type` smallint(6) NOT NULL,
  `clan` int(11) NOT NULL,
  `rescue_map` text NOT NULL,
  `rescue_x` tinyint(3) unsigned NOT NULL,
  `rescue_y` tinyint(3) unsigned NOT NULL,
  `rescue_orientation` smallint(6) NOT NULL,
  `unvalidated_rescue_map` text NOT NULL,
  `unvalidated_rescue_x` tinyint(3) unsigned NOT NULL,
  `unvalidated_rescue_y` tinyint(3) unsigned NOT NULL,
  `unvalidated_rescue_orientation` smallint(6) NOT NULL,
  `clan_leader` tinyint(1) NOT NULL,
  `date` int(11) unsigned NOT NULL,
  `cash` bigint(20) unsigned NOT NULL,
  `warehouse_cash` bigint(20) unsigned NOT NULL,
  `market_cash` bigint(20) unsigned NOT NULL,
  `time_to_delete` int(11) unsigned NOT NULL,
  `played_time` int(11) unsigned NOT NULL,
  `last_connect` int(11) unsigned NOT NULL,
  `starter` tinyint(4) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `pseudo` (`pseudo`,`clan`),
  UNIQUE KEY `pseudo_2` (`pseudo`),
  KEY `clan` (`clan`),
  KEY `account` (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `character_allow`
--

CREATE TABLE IF NOT EXISTS `character_allow` (
  `character` int(11) NOT NULL,
  `allow` smallint(6) NOT NULL,
  KEY `character` (`character`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `character_itemonmap`
--

CREATE TABLE IF NOT EXISTS `character_itemonmap` (
  `character` int(11) NOT NULL,
  `itemOnMap` smallint(6) NOT NULL,
  KEY `character` (`character`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `city`
--

CREATE TABLE IF NOT EXISTS `city` (
  `city` varchar(64) NOT NULL,
  `clan` int(11) NOT NULL,
  PRIMARY KEY (`city`)
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
-- Table structure for table `dictionary_allow`
--

CREATE TABLE IF NOT EXISTS `dictionary_allow` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `allow` text NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `dictionary_itemonmap`
--

CREATE TABLE IF NOT EXISTS `dictionary_itemonmap` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `map` text NOT NULL,
  `x` smallint(6) NOT NULL,
  `y` smallint(6) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `dictionary_map`
--

CREATE TABLE IF NOT EXISTS `dictionary_map` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `map` text NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `dictionary_reputation`
--

CREATE TABLE IF NOT EXISTS `dictionary_reputation` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `reputation` text NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `dictionary_skin`
--

CREATE TABLE IF NOT EXISTS `dictionary_skin` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `skin` text NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `factory`
--

CREATE TABLE IF NOT EXISTS `factory` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `resources` text NOT NULL,
  `products` text NOT NULL,
  `last_update` int(11) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `item`
--

CREATE TABLE IF NOT EXISTS `item` (
  `item` int(11) unsigned NOT NULL,
  `character` int(11) NOT NULL,
  `quantity` int(11) unsigned NOT NULL,
  PRIMARY KEY (`item`,`character`),
  KEY `player_id` (`character`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `item_market`
--

CREATE TABLE IF NOT EXISTS `item_market` (
  `item` smallint(6) NOT NULL,
  `character` int(11) NOT NULL,
  `quantity` int(11) NOT NULL,
  `market_price` bigint(20) NOT NULL,
  UNIQUE KEY `item` (`item`,`character`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `item_warehouse`
--

CREATE TABLE IF NOT EXISTS `item_warehouse` (
  `item` smallint(6) NOT NULL,
  `character` int(11) NOT NULL,
  `quantity` int(11) NOT NULL,
  UNIQUE KEY `item` (`item`,`character`),
  KEY `character` (`character`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `monster`
--

CREATE TABLE IF NOT EXISTS `monster` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `hp` smallint(5) unsigned NOT NULL,
  `character` int(11) NOT NULL,
  `monster` smallint(5) unsigned NOT NULL,
  `level` tinyint(3) unsigned NOT NULL,
  `xp` int(11) unsigned NOT NULL,
  `sp` int(11) unsigned NOT NULL,
  `captured_with` smallint(5) unsigned NOT NULL,
  `gender` smallint(6) NOT NULL,
  `egg_step` int(11) unsigned NOT NULL,
  `character_origin` int(11) NOT NULL,
  `place` smallint(6) NOT NULL,
  `position` tinyint(3) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  KEY `player` (`character`),
  KEY `place` (`place`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `monster_buff`
--

CREATE TABLE IF NOT EXISTS `monster_buff` (
  `monster` int(11) NOT NULL,
  `buff` smallint(5) unsigned NOT NULL,
  `level` tinyint(3) unsigned NOT NULL,
  PRIMARY KEY (`monster`,`buff`),
  KEY `monster` (`monster`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `monster_market`
--

CREATE TABLE IF NOT EXISTS `monster_market` (
  `id` int(11) NOT NULL,
  `hp` smallint(6) NOT NULL,
  `character` int(11) NOT NULL,
  `monster` smallint(6) NOT NULL,
  `level` smallint(6) NOT NULL,
  `xp` int(11) NOT NULL,
  `sp` int(11) NOT NULL,
  `captured_with` smallint(6) NOT NULL,
  `gender` smallint(6) NOT NULL,
  `egg_step` int(11) NOT NULL,
  `character_origin` int(11) NOT NULL,
  `market_price` bigint(20) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `monster_skill`
--

CREATE TABLE IF NOT EXISTS `monster_skill` (
  `monster` int(11) NOT NULL,
  `skill` smallint(6) NOT NULL,
  `level` tinyint(4) NOT NULL,
  `endurance` tinyint(4) NOT NULL,
  PRIMARY KEY (`monster`,`skill`),
  KEY `monster` (`monster`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `monster_warehouse`
--

CREATE TABLE IF NOT EXISTS `monster_warehouse` (
  `id` int(11) NOT NULL,
  `hp` smallint(6) NOT NULL,
  `character` int(11) NOT NULL,
  `monster` smallint(6) NOT NULL,
  `level` smallint(6) NOT NULL,
  `xp` int(11) NOT NULL,
  `sp` int(11) NOT NULL,
  `captured_with` smallint(6) NOT NULL,
  `gender` smallint(6) NOT NULL,
  `egg_step` int(11) NOT NULL,
  `character_origin` int(11) NOT NULL,
  `position` smallint(6) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `character` (`character`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `plant`
--

CREATE TABLE IF NOT EXISTS `plant` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `map` text NOT NULL,
  `x` tinyint(3) unsigned NOT NULL,
  `y` tinyint(3) unsigned NOT NULL,
  `plant` tinyint(3) unsigned NOT NULL,
  `character` int(11) NOT NULL,
  `plant_timestamps` int(11) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `quest`
--

CREATE TABLE IF NOT EXISTS `quest` (
  `character` int(11) NOT NULL,
  `quest` smallint(5) unsigned NOT NULL,
  `finish_one_time` tinyint(1) NOT NULL,
  `step` tinyint(3) unsigned NOT NULL,
  PRIMARY KEY (`character`,`quest`),
  KEY `player` (`character`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `recipe`
--

CREATE TABLE IF NOT EXISTS `recipe` (
  `character` int(11) NOT NULL,
  `recipe` smallint(5) unsigned NOT NULL,
  PRIMARY KEY (`character`,`recipe`),
  KEY `player` (`character`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `reputation`
--

CREATE TABLE IF NOT EXISTS `reputation` (
  `character` int(11) NOT NULL,
  `type` varchar(16) NOT NULL,
  `point` bigint(20) NOT NULL,
  `level` tinyint(4) NOT NULL,
  PRIMARY KEY (`character`,`type`),
  KEY `character` (`character`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
