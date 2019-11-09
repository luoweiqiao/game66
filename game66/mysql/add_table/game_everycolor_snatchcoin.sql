INSERT INTO `chess`.`user0` (`uid`, `nickname`, `sex`, `rtime`, `logintime`, `offlinetime`, `clogin`, `weeklogin`, `alllogin`, `onlinetime`, `reward`, `bankrupt`, `dgcount`, `setting`, `safepwd`, `safeproblem`, `safeanswer`, `ispay`, `payall`, `paytimes`, `fpaytime`, `lpaytime`, `ip`, `loginip`, `mac`, `signtype`, `imagetype`, `imageurl`, `deviceid`, `ldeviceid`, `parentuid`, `oldname`, `uptime`, `forbid`, `level`, `birthday`, `recharge`) VALUES ('1000000011', '统一账户11', '2', '1481091980', '1531101212', '1531101213', '0', '4', '293', '0', '0', '0', '0', '', '', '', '', '0', '0', '0', '0', '0', ' ', '', '', '', '1', '1', '', '', '0', '', '0', '0', '0', '0', '0');

use chess;
SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for game_everycolor_snatchcoin
-- ----------------------------
DROP TABLE IF EXISTS `game_everycolor_snatchcoin`;
CREATE TABLE `game_everycolor_snatchcoin` (
  `uid` int(11) DEFAULT '0' NOT NULL COMMENT '用户ID',
  `type` int(11) DEFAULT '3' NOT NULL COMMENT '夺宝类型',
  `card` varchar(512) NOT NULL DEFAULT '' COMMENT '扑克数据'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=COMPACT;
