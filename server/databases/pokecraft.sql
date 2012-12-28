-- phpMyAdmin SQL Dump
-- version 3.4.7.1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Dec 28, 2012 at 04:53 PM
-- Server version: 5.1.66
-- PHP Version: 5.4.9--pl0-gentoo

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `pokecraft`
--

-- --------------------------------------------------------

--
-- Table structure for table `item`
--

CREATE TABLE IF NOT EXISTS `item` (
  `item_id` int(11) NOT NULL,
  `player_id` int(11) NOT NULL,
  `quantity` int(11) NOT NULL,
  PRIMARY KEY (`item_id`,`player_id`),
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
  `remaining_xp` int(11) NOT NULL,
  `sp` int(11) NOT NULL,
  `captured_with` int(11) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `player` (`player`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=2 ;

-- --------------------------------------------------------

--
-- Table structure for table `monster_buff`
--

CREATE TABLE IF NOT EXISTS `monster_buff` (
  `monster` int(11) NOT NULL,
  `buff` int(11) NOT NULL,
  `level` int(11) NOT NULL,
  PRIMARY KEY (`monster`,`buff`),
  UNIQUE KEY `monster` (`monster`)
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
  `cash` int(11) NOT NULL,
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
-- Table structure for table `recipes`
--

CREATE TABLE IF NOT EXISTS `recipes` (
  `player` int(11) NOT NULL,
  `recipe` int(11) NOT NULL,
  PRIMARY KEY (`player`,`recipe`),
  KEY `player` (`player`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
