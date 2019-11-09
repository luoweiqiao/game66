

INSERT INTO `chess`.`user0` (`uid`, `nickname`, `sex`, `rtime`, `logintime`, `offlinetime`, `clogin`, `weeklogin`, `alllogin`, `onlinetime`, `reward`, `bankrupt`, `dgcount`, `setting`, `safepwd`, `safeproblem`, `safeanswer`, `ispay`, `payall`, `paytimes`, `fpaytime`, `lpaytime`, `ip`, `loginip`, `mac`, `signtype`, `imagetype`, `imageurl`, `deviceid`, `ldeviceid`, `parentuid`, `oldname`, `uptime`, `forbid`, `level`, `birthday`, `recharge`) VALUES ('1000000012', '统一账户12', '2', '1481091980', '1531101212', '1531101213', '0', '4', '293', '0', '0', '0', '0', '', '', '', '', '0', '0', '0', '0', '0', ' ', '', '', '', '1', '1', '', '', '0', '', '0', '0', '0', '0', '0');



/*
Navicat MySQL Data Transfer

Source Server         : testserver
Source Server Version : 50173
Source Host           : 120.24.54.6:3306
Source Database       : chess

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 65001

Date: 2017-05-16 11:35:53
*/
use chess;
SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for game_two_people_majiang
-- ----------------------------
DROP TABLE IF EXISTS `game_two_people_majiang`;
CREATE TABLE `game_two_people_majiang` (
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
  `daywinc` bigint(20) NOT NULL COMMENT '进入输赢',
  PRIMARY KEY (`uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=COMPACT;
