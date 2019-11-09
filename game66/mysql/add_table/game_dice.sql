
use chess;
SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for game_dice
-- ----------------------------
DROP TABLE IF EXISTS `game_dice`;
CREATE TABLE `game_dice` (
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
