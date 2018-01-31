SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

--
-- Database: `catchchallenger_common`
--

-- --------------------------------------------------------

--
-- Table structure for table `character`
--

CREATE TABLE `character` (
  `id` int(11) NOT NULL,
  `account` int(11) NOT NULL,
  `pseudo` varchar(20) NOT NULL,
  `skin` int(11) NOT NULL,
  `type` smallint(6) NOT NULL,
  `clan` int(11) NOT NULL,
  `clan_leader` tinyint(1) NOT NULL,
  `date` int(11) UNSIGNED NOT NULL,
  `cash` bigint(20) UNSIGNED NOT NULL,
  `warehouse_cash` bigint(20) UNSIGNED NOT NULL,
  `time_to_delete` int(11) UNSIGNED NOT NULL,
  `played_time` int(11) UNSIGNED NOT NULL,
  `last_connect` int(11) UNSIGNED NOT NULL,
  `starter` tinyint(4) UNSIGNED NOT NULL,
  `allow` tinyblob NOT NULL,
  `item` blob NOT NULL,
  `item_warehouse` mediumblob NOT NULL,
  `recipes` blob NOT NULL,
  `reputations` blob NOT NULL,
  `encyclopedia_monster` mediumblob NOT NULL,
  `encyclopedia_item` mediumblob NOT NULL,
  `achievements` tinyblob NOT NULL,
  `blob_version` tinyint(4) NOT NULL,
  `lastdaillygift` bigint(20) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `clan`
--

CREATE TABLE `clan` (
  `id` int(11) NOT NULL,
  `name` text NOT NULL,
  `cash` bigint(20) UNSIGNED NOT NULL,
  `date` int(11) UNSIGNED NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `monster`
--

CREATE TABLE `monster` (
  `id` int(11) NOT NULL,
  `character` int(11) UNSIGNED NOT NULL,
  `place` smallint(5) UNSIGNED NOT NULL,
  `hp` smallint(5) UNSIGNED NOT NULL,
  `monster` smallint(5) UNSIGNED NOT NULL,
  `level` tinyint(3) UNSIGNED NOT NULL,
  `xp` int(11) UNSIGNED NOT NULL,
  `sp` int(11) UNSIGNED NOT NULL,
  `captured_with` smallint(5) UNSIGNED NOT NULL,
  `gender` smallint(6) NOT NULL,
  `egg_step` int(11) UNSIGNED NOT NULL,
  `character_origin` int(11) NOT NULL,
  `position` tinyint(3) UNSIGNED NOT NULL,
  `buffs` blob NOT NULL,
  `skills` tinyblob NOT NULL,
  `skills_endurance` tinyblob NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `server_time`
--

CREATE TABLE `server_time` (
  `server` smallint(6) NOT NULL,
  `account` int(11) NOT NULL,
  `played_time` int(11) NOT NULL,
  `last_connect` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `character`
--
ALTER TABLE `character`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `pseudo` (`pseudo`,`clan`),
  ADD UNIQUE KEY `pseudo_2` (`pseudo`),
  ADD KEY `clan` (`clan`),
  ADD KEY `account` (`account`);

--
-- Indexes for table `clan`
--
ALTER TABLE `clan`
  ADD PRIMARY KEY (`id`);

--
-- Indexes for table `monster`
--
ALTER TABLE `monster`
  ADD PRIMARY KEY (`id`),
  ADD KEY `character` (`character`);

--
-- Indexes for table `server_time`
--
ALTER TABLE `server_time`
  ADD PRIMARY KEY (`server`,`account`),
  ADD KEY `account` (`account`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `character`
--
ALTER TABLE `character`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
--
-- AUTO_INCREMENT for table `clan`
--
ALTER TABLE `clan`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
--
-- AUTO_INCREMENT for table `monster`
--
ALTER TABLE `monster`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
