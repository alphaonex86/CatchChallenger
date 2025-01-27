SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

--
-- Database: `catchchallenger_server`
--

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
  `by_map` blob NOT NULL,
  `quest` blob NOT NULL,
  PRIMARY KEY (`character`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_bymap` (
  `character` int(11) NOT NULL,
  `map` int(11) NOT NULL,
  `plants` blob NOT NULL,
  `items` blob NOT NULL,
  `fights` blob NOT NULL
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
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

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
