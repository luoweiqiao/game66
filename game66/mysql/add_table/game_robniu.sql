

INSERT INTO `chess`.`serverinfo` (`svrid`, `name`, `group`, `svr_type`, `game_type`, `game_subtype`, `svrip`, `svrlanip`, `svrport`, `svrlanport`, `phpport`, `openrobot`, `state`, `onlines`, `robots`, `report_time`) VALUES ('161', '看牌抢庄牛牛', '1', '2', '16', '1', '120.24.54.6', '120.24.54.6', '0', '7123', '0', '1', '1', '0', '0', '2018-05-15 20:33:46');

INSERT INTO `chess_sysdata`.`roomcfg` (`gametype`, `gamesubtype`, `roomid`, `name`, `deal`, `consume`, `entermin`, `entermax`, `isopen`, `basescore`, `roomtype`, `showhandnum`, `tablenum`, `marry`, `limitenter`, `robot`, `showonline`, `sitdown`, `feetype`, `fee`, `seatnum`, `showtype`, `showpic`, `robotminscore`, `robotmaxscore`, `param`) VALUES ('16', '1', '1', '看牌抢庄牛牛', '1', '2', '0', '1000000000', '1', '100', '0', '0', '100', '0', '1', '1', '0', '0', '2', '500', '4', '0', '0', '0', '10000000', '');

INSERT INTO `chess`.`account0` (`uid`, `diamond`, `coin`, `ingot`, `score`, `cvalue`, `vip`, `safecoin`, `bankrupt`, `feewin`, `feelose`, `sysreward`, `Recharge`, `converts`, `win`, `reward`, `bdingreward`, `expiretime`) VALUES ('1000000016', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0');

INSERT INTO `chess`.`user0` (`uid`, `nickname`, `sex`, `rtime`, `logintime`, `offlinetime`, `clogin`, `weeklogin`, `alllogin`, `onlinetime`, `reward`, `bankrupt`, `dgcount`, `setting`, `safepwd`, `safeproblem`, `safeanswer`, `ispay`, `payall`, `paytimes`, `fpaytime`, `lpaytime`, `ip`, `loginip`, `mac`, `signtype`, `imagetype`, `imageurl`, `deviceid`, `ldeviceid`, `parentuid`, `oldname`, `uptime`, `forbid`, `level`, `birthday`, `recharge`) VALUES ('1000000016', '统一账户16', '2', '1481091980', '1531101212', '1531101213', '0', '4', '293', '0', '0', '0', '0', '', '', '', '', '0', '0', '0', '0', '0', ' ', '', '', '', '1', '1', '', '', '0', '', '0', '0', '0', '0', '0');


use chess;
SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for game_robniu
-- ----------------------------
DROP TABLE IF EXISTS `game_robniu`;
CREATE TABLE `game_robniu` (
  `uid` int(11) NOT NULL COMMENT '用户ID',
  `win` int(11) NOT NULL COMMENT '积分赢',
  `lose` int(11) NOT NULL COMMENT '积分输',
  `maxwin` bigint(20) NOT NULL COMMENT '积分最大赢',
  `winc` int(11) NOT NULL COMMENT '财富币赢',
  `losec` int(11) NOT NULL COMMENT '财富币输',
  `maxwinc` bigint(20) NOT NULL COMMENT '财富币最大赢',
  `maxcard` varchar(20) NOT NULL COMMENT '最大牌型',
  `maxcardc` varchar(20) NOT NULL COMMENT '最大牌型',
  `daywin` bigint(20) NOT NULL COMMENT '今日输赢',
  `daywinc` bigint(20) NOT NULL COMMENT '今日输赢',
  PRIMARY KEY (`uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=COMPACT;
