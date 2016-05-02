-- phpMyAdmin SQL Dump
-- version 3.4.7.1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: May 02, 2016 at 06:26 PM
-- Server version: 5.6.28
-- PHP Version: 5.5.32-pl0-gentoo

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `catchchallenger_login`
--

-- --------------------------------------------------------

--
-- Table structure for table `account`
--

CREATE TABLE IF NOT EXISTS `account` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `login` varbinary(28) NOT NULL,
  `password` varbinary(28) NOT NULL COMMENT 'sha224 + 4Byte salt',
  `date` bigint(20) unsigned NOT NULL,
  `email` varchar(64) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `login` (`login`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `account_register`
--

CREATE TABLE IF NOT EXISTS `account_register` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `login` varbinary(28) NOT NULL,
  `password` varbinary(28) NOT NULL COMMENT 'sha224 + 4Byte salt',
  `email` text NOT NULL,
  `key` text NOT NULL,
  `date` int(11) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `login_2` (`login`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
