SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

--
-- Database: `catchchallenger`
--

-- --------------------------------------------------------

--
-- Table structure for table `bot_already_beaten`
--

CREATE TABLE IF NOT EXISTS `bot_already_beaten` (
  `player_id` int(11) NOT NULL,
  `botfight_id` int(11) NOT NULL,
  PRIMARY KEY (`player_id`,`botfight_id`),
  KEY `player_id` (`player_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `item`
--

CREATE TABLE IF NOT EXISTS `item` (
  `item_id` int(11) NOT NULL,
  `player_id` int(11) NOT NULL,
  `quantity` int(11) NOT NULL,
  `warehouse` tinyint(1) NOT NULL COMMENT 'true if into warehouse',
  PRIMARY KEY (`item_id`,`player_id`,`warehouse`),
  KEY `player_id` (`player_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `monster`
--

CREATE TABLE IF NOT EXISTS `monster` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `hp` int(11) NOT NULL,
  `player` int(11) NOT NULL,
  `monster` int(11) NOT NULL,
  `level` int(11) NOT NULL,
  `xp` int(11) NOT NULL,
  `sp` int(11) NOT NULL,
  `captured_with` int(11) NOT NULL,
  `gender` enum('unknown','male','female') NOT NULL,
  `egg_step` int(11) NOT NULL,
  `player_origin` int(11) NOT NULL,
  `warehouse` tinyint(1) NOT NULL COMMENT 'true if into warehouse',
  PRIMARY KEY (`id`),
  KEY `player` (`player`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=4 ;

-- --------------------------------------------------------

--
-- Table structure for table `monster_buff`
--

CREATE TABLE IF NOT EXISTS `monster_buff` (
  `monster` int(11) NOT NULL,
  `buff` int(11) NOT NULL,
  `level` int(11) NOT NULL,
  PRIMARY KEY (`monster`,`buff`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `monster_skill`
--

CREATE TABLE IF NOT EXISTS `monster_skill` (
  `monster` int(11) NOT NULL,
  `skill` int(11) NOT NULL,
  `level` int(11) NOT NULL,
  PRIMARY KEY (`monster`,`skill`),
  KEY `monster` (`monster`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `plant`
--

CREATE TABLE IF NOT EXISTS `plant` (
  `map` varchar(255) NOT NULL,
  `x` int(11) NOT NULL,
  `y` int(11) NOT NULL,
  `plant` int(11) NOT NULL,
  `player_id` int(11) NOT NULL,
  `plant_timestamps` int(11) NOT NULL,
  PRIMARY KEY (`map`,`x`,`y`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `player`
--

CREATE TABLE IF NOT EXISTS `player` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `login` varchar(32) NOT NULL,
  `password` varchar(64) NOT NULL,
  `pseudo` varchar(255) NOT NULL,
  `email` text NOT NULL,
  `skin` text NOT NULL,
  `position_x` int(11) NOT NULL,
  `position_y` int(11) NOT NULL,
  `orientation` enum('top','bottom','left','right') NOT NULL,
  `map_name` text NOT NULL,
  `type` enum('normal','premium','gm','dev') NOT NULL,
  `clan` int(11) NOT NULL,
  `cash` bigint(20) NOT NULL,
  `rescue_map` text NOT NULL,
  `rescue_x` int(11) NOT NULL,
  `rescue_y` int(11) NOT NULL,
  `rescue_orientation` enum('top','bottom','left','right') NOT NULL,
  `unvalidated_rescue_map` text NOT NULL,
  `unvalidated_rescue_x` int(11) NOT NULL,
  `unvalidated_rescue_y` int(11) NOT NULL,
  `unvalidated_rescue_orientation` text NOT NULL,
  `warehouse_cash` bigint(20) NOT NULL,
  `allow` text NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `login` (`login`,`password`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=7 ;

-- --------------------------------------------------------

--
-- Table structure for table `player_skill`
--

CREATE TABLE IF NOT EXISTS `player_skill` (
  `player` int(11) NOT NULL,
  `skill` int(11) NOT NULL,
  `level` int(11) NOT NULL,
  PRIMARY KEY (`player`,`skill`),
  KEY `player` (`player`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `quest`
--

CREATE TABLE IF NOT EXISTS `quest` (
  `player` int(11) NOT NULL,
  `quest` int(11) NOT NULL,
  `finish_one_time` tinyint(1) NOT NULL,
  `step` int(11) NOT NULL,
  PRIMARY KEY (`player`,`quest`),
  KEY `player` (`player`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `recipes`
--

CREATE TABLE IF NOT EXISTS `recipes` (
  `player` int(11) NOT NULL,
  `recipe` int(11) NOT NULL,
  PRIMARY KEY (`player`,`recipe`),
  KEY `player` (`player`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `reputation`
--

CREATE TABLE IF NOT EXISTS `reputation` (
  `player` int(11) NOT NULL,
  `type` varchar(32) NOT NULL,
  `point` bigint(20) NOT NULL,
  `level` int(11) NOT NULL,
  PRIMARY KEY (`player`,`type`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
