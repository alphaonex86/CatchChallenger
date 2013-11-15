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
  `login` varchar(128) NOT NULL,
  `password` varchar(128) NOT NULL,
  `date` int(11) NOT NULL,
  `email` varchar(64) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `login` (`login`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

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
  `date` int(11) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `login_2` (`login`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `bitcoin_history`
--

CREATE TABLE IF NOT EXISTS `bitcoin_history` (
  `character` int(11) NOT NULL,
  `date` int(11) NOT NULL,
  `change` double NOT NULL,
  `reason` text NOT NULL
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
  `skin` varchar(16) NOT NULL,
  `x` int(11) NOT NULL,
  `y` int(11) NOT NULL,
  `orientation` enum('top','bottom','left','right') NOT NULL,
  `map` text NOT NULL,
  `type` enum('normal','premium','gm','dev') NOT NULL,
  `clan` int(11) NOT NULL,
  `rescue_map` text NOT NULL,
  `rescue_x` int(11) NOT NULL,
  `rescue_y` int(11) NOT NULL,
  `rescue_orientation` enum('top','bottom','left','right') NOT NULL,
  `unvalidated_rescue_map` text NOT NULL,
  `unvalidated_rescue_x` int(11) NOT NULL,
  `unvalidated_rescue_y` int(11) NOT NULL,
  `unvalidated_rescue_orientation` enum('top','bottom','left','right') NOT NULL,
  `allow` text NOT NULL,
  `clan_leader` tinyint(1) NOT NULL,
  `date` int(11) NOT NULL,
  `bitcoin_offset` double NOT NULL,
  `market_bitcoin` double NOT NULL,
  `cash` bigint(20) NOT NULL,
  `warehouse_cash` bigint(20) NOT NULL,
  `market_cash` bigint(20) NOT NULL,
  `time_to_delete` int(11) NOT NULL,
  `played_time` int(11) NOT NULL,
  `last_connect` int(11) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `pseudo` (`pseudo`,`clan`),
  UNIQUE KEY `pseudo_2` (`pseudo`),
  KEY `clan` (`clan`),
  KEY `account` (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

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
  `cash` bigint(20) NOT NULL,
  `date` int(11) NOT NULL,
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
  `last_update` int(11) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `item`
--

CREATE TABLE IF NOT EXISTS `item` (
  `item` int(11) NOT NULL,
  `character` int(11) NOT NULL,
  `quantity` int(11) NOT NULL,
  `place` enum('wear','warehouse','market') NOT NULL,
  `market_price` bigint(20) NOT NULL,
  `market_bitcoin` double NOT NULL,
  PRIMARY KEY (`item`,`character`,`place`),
  KEY `player_id` (`character`),
  KEY `place` (`place`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `monster`
--

CREATE TABLE IF NOT EXISTS `monster` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `hp` int(11) NOT NULL,
  `character` int(11) NOT NULL,
  `monster` int(11) NOT NULL,
  `level` int(11) NOT NULL,
  `xp` int(11) NOT NULL,
  `sp` int(11) NOT NULL,
  `captured_with` int(11) NOT NULL,
  `gender` enum('unknown','male','female') NOT NULL,
  `egg_step` int(11) NOT NULL,
  `character_origin` int(11) NOT NULL,
  `place` enum('wear','warehouse','market') NOT NULL,
  `position` int(11) NOT NULL,
  `market_price` bigint(20) NOT NULL,
  `market_bitcoin` double NOT NULL,
  PRIMARY KEY (`id`),
  KEY `player` (`character`),
  KEY `place` (`place`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `monster_buff`
--

CREATE TABLE IF NOT EXISTS `monster_buff` (
  `monster` int(11) NOT NULL,
  `buff` int(11) NOT NULL,
  `level` int(11) NOT NULL,
  PRIMARY KEY (`monster`,`buff`),
  KEY `monster` (`monster`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `monster_skill`
--

CREATE TABLE IF NOT EXISTS `monster_skill` (
  `monster` int(11) NOT NULL,
  `skill` int(11) NOT NULL,
  `level` int(11) NOT NULL,
  `endurance` int(11) NOT NULL,
  PRIMARY KEY (`monster`,`skill`),
  KEY `monster` (`monster`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `plant`
--

CREATE TABLE IF NOT EXISTS `plant` (
  `map` varchar(64) NOT NULL,
  `x` int(11) NOT NULL,
  `y` int(11) NOT NULL,
  `plant` int(11) NOT NULL,
  `character` int(11) NOT NULL,
  `plant_timestamps` int(11) NOT NULL,
  PRIMARY KEY (`map`,`x`,`y`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `quest`
--

CREATE TABLE IF NOT EXISTS `quest` (
  `character` int(11) NOT NULL,
  `quest` int(11) NOT NULL,
  `finish_one_time` tinyint(1) NOT NULL,
  `step` int(11) NOT NULL,
  PRIMARY KEY (`character`,`quest`),
  KEY `player` (`character`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `recipes`
--

CREATE TABLE IF NOT EXISTS `recipes` (
  `character` int(11) NOT NULL,
  `recipe` int(11) NOT NULL,
  PRIMARY KEY (`character`,`recipe`),
  KEY `player` (`character`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `reputation`
--

CREATE TABLE IF NOT EXISTS `reputation` (
  `character` int(11) NOT NULL,
  `type` varchar(32) NOT NULL,
  `point` bigint(20) NOT NULL,
  `level` int(11) NOT NULL,
  PRIMARY KEY (`character`,`type`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
