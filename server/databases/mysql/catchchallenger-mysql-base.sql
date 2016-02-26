SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

--
-- Database: `catchchallenger_login`
--

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
-- Table structure for table `dictionary_starter`
--

CREATE TABLE IF NOT EXISTS `dictionary_starter` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `starter` text NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
