SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

--
-- Database: `catchchallenger_server`
--

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
-- Table structure for table `character_forserver`
--

CREATE TABLE IF NOT EXISTS `character_forserver` (
  `character` int(11) NOT NULL,
  `x` tinyint(3) unsigned NOT NULL,
  `y` tinyint(3) unsigned NOT NULL,
  `orientation` smallint(6) NOT NULL,
  `map` int(11) NOT NULL,
  `rescue_map` int(11) NOT NULL,
  `rescue_x` tinyint(3) unsigned NOT NULL,
  `rescue_y` tinyint(3) unsigned NOT NULL,
  `rescue_orientation` smallint(6) NOT NULL,
  `unvalidated_rescue_map` int(11) NOT NULL,
  `unvalidated_rescue_x` tinyint(3) unsigned NOT NULL,
  `unvalidated_rescue_y` tinyint(3) unsigned NOT NULL,
  `unvalidated_rescue_orientation` smallint(6) NOT NULL,
  `date` int(11) unsigned NOT NULL,
  `market_cash` bigint(20) unsigned NOT NULL,
  PRIMARY KEY (`character`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `character_itemonmap`
--

CREATE TABLE IF NOT EXISTS `character_itemonmap` (
  `character` int(11) NOT NULL,
  `pointOnMap` smallint(6) NOT NULL,
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
-- Table structure for table `dictionary_map`
--

CREATE TABLE IF NOT EXISTS `dictionary_map` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `map` text NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `dictionary_pointonmap`
--

CREATE TABLE IF NOT EXISTS `dictionary_pointonmap` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `map` int(11) NOT NULL,
  `x` smallint(6) NOT NULL,
  `y` smallint(6) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

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
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

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
-- Table structure for table `monster_market_price`
--

CREATE TABLE IF NOT EXISTS `monster_market_price` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `market_price` int(11) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `plant`
--

CREATE TABLE IF NOT EXISTS `plant` (
  `character` int(11) NOT NULL,
  `pointOnMap` int(11) NOT NULL,
  `plant` tinyint(3) unsigned NOT NULL,
  `plant_timestamps` int(11) unsigned NOT NULL,
  PRIMARY KEY (`character`,`pointOnMap`),
  KEY `character` (`character`)
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
