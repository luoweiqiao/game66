
alter table chess.game_baccarat add column vecwin varchar(128) DEFAULT '' COMMENT '最近输赢';
alter table chess.game_bainiu add column vecwin varchar(128) DEFAULT '' COMMENT '最近输赢';
alter table chess.game_paijiu add column vecwin varchar(128) DEFAULT '' COMMENT '最近输赢';
alter table chess.game_slot add column vecwin varchar(128) DEFAULT '' COMMENT '最近输赢';
alter table chess.game_dice add column vecwin varchar(128) DEFAULT '' COMMENT '最近输赢';


alter table chess.game_baccarat add column vecbet varchar(128) DEFAULT '' COMMENT '下注金额';
alter table chess.game_bainiu add column vecbet varchar(128) DEFAULT '' COMMENT '下注金额';
alter table chess.game_paijiu add column vecbet varchar(128) DEFAULT '' COMMENT '下注金额';
alter table chess.game_slot add column vecbet varchar(128) DEFAULT '' COMMENT '下注金额';
alter table chess.game_dice add column vecbet varchar(128) DEFAULT '' COMMENT '下注金额';


alter table chess.game_baccarat drop column vecwin;
alter table chess.game_bainiu drop column vecwin;
alter table chess.game_paijiu drop column vecwin;
alter table chess.game_slot drop column vecwin;
alter table chess.game_dice drop column vecwin;


alter table chess.game_baccarat drop column vecbet;
alter table chess.game_bainiu drop column vecbet;
alter table chess.game_paijiu drop column vecbet;
alter table chess.game_slot drop column vecbet;
alter table chess.game_dice drop column vecbet;

drop table chess.game_war;







use chess;
SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for game_bairen_count
-- ----------------------------
DROP TABLE IF EXISTS `game_bairen_count`;
CREATE TABLE `game_bairen_count` (
  `chessid` varchar(64) NOT NULL COMMENT '牌局id',
  `svrid` int(11) NOT NULL COMMENT 'svrid',
  `gametype` int(11) NOT NULL COMMENT 'gametype',
  `roomid` int(11) NOT NULL COMMENT 'roomid',
  `tableid` int(11) NOT NULL COMMENT 'tableid',
  `uid` int(11) NOT NULL COMMENT '用户ID',
  `winscore` bigint(20) NOT NULL COMMENT '积分赢',
  `betscore` bigint(20) NOT NULL COMMENT '下注积分',
  INDEX `chessid` USING BTREE (`chessid`),
  INDEX `uid` USING BTREE (`uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=COMPACT;






