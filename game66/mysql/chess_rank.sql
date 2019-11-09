/*
Navicat MySQL Data Transfer

Source Server         : 趣味1
Source Server Version : 50173
Source Host           : 120.24.54.6:3307
Source Database       : chess_rank

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 65001

Date: 2017-01-18 17:09:23
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `rank20170104`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170104`;
CREATE TABLE `rank20170104` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=32

;

-- ----------------------------
-- Table structure for `rank20170105`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170105`;
CREATE TABLE `rank20170105` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=14

;

-- ----------------------------
-- Table structure for `rank20170106`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170106`;
CREATE TABLE `rank20170106` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=16

;

-- ----------------------------
-- Table structure for `rank20170107`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170107`;
CREATE TABLE `rank20170107` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=8

;

-- ----------------------------
-- Table structure for `rank20170108`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170108`;
CREATE TABLE `rank20170108` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=2

;

-- ----------------------------
-- Table structure for `rank20170109`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170109`;
CREATE TABLE `rank20170109` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=16

;

-- ----------------------------
-- Table structure for `rank20170110`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170110`;
CREATE TABLE `rank20170110` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=13

;

-- ----------------------------
-- Table structure for `rank20170111`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170111`;
CREATE TABLE `rank20170111` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=13

;

-- ----------------------------
-- Table structure for `rank20170112`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170112`;
CREATE TABLE `rank20170112` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=9

;

-- ----------------------------
-- Table structure for `rank20170113`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170113`;
CREATE TABLE `rank20170113` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=6

;

-- ----------------------------
-- Table structure for `rank20170114`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170114`;
CREATE TABLE `rank20170114` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Table structure for `rank20170115`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170115`;
CREATE TABLE `rank20170115` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Table structure for `rank20170116`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170116`;
CREATE TABLE `rank20170116` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Table structure for `rank20170117`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170117`;
CREATE TABLE `rank20170117` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Table structure for `rank20170118`
-- ----------------------------
DROP TABLE IF EXISTS `rank20170118`;
CREATE TABLE `rank20170118` (
`rid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`dwin`  bigint(10) NOT NULL COMMENT '每日输赢' ,
`dreward`  bigint(10) NOT NULL COMMENT '每日领奖' ,
PRIMARY KEY (`rid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='每日盈利和领奖排行榜'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Auto increment value for `rank20170104`
-- ----------------------------
ALTER TABLE `rank20170104` AUTO_INCREMENT=32;

-- ----------------------------
-- Auto increment value for `rank20170105`
-- ----------------------------
ALTER TABLE `rank20170105` AUTO_INCREMENT=14;

-- ----------------------------
-- Auto increment value for `rank20170106`
-- ----------------------------
ALTER TABLE `rank20170106` AUTO_INCREMENT=16;

-- ----------------------------
-- Auto increment value for `rank20170107`
-- ----------------------------
ALTER TABLE `rank20170107` AUTO_INCREMENT=8;

-- ----------------------------
-- Auto increment value for `rank20170108`
-- ----------------------------
ALTER TABLE `rank20170108` AUTO_INCREMENT=2;

-- ----------------------------
-- Auto increment value for `rank20170109`
-- ----------------------------
ALTER TABLE `rank20170109` AUTO_INCREMENT=16;

-- ----------------------------
-- Auto increment value for `rank20170110`
-- ----------------------------
ALTER TABLE `rank20170110` AUTO_INCREMENT=13;

-- ----------------------------
-- Auto increment value for `rank20170111`
-- ----------------------------
ALTER TABLE `rank20170111` AUTO_INCREMENT=13;

-- ----------------------------
-- Auto increment value for `rank20170112`
-- ----------------------------
ALTER TABLE `rank20170112` AUTO_INCREMENT=9;

-- ----------------------------
-- Auto increment value for `rank20170113`
-- ----------------------------
ALTER TABLE `rank20170113` AUTO_INCREMENT=6;

-- ----------------------------
-- Auto increment value for `rank20170114`
-- ----------------------------
ALTER TABLE `rank20170114` AUTO_INCREMENT=1;

-- ----------------------------
-- Auto increment value for `rank20170115`
-- ----------------------------
ALTER TABLE `rank20170115` AUTO_INCREMENT=1;

-- ----------------------------
-- Auto increment value for `rank20170116`
-- ----------------------------
ALTER TABLE `rank20170116` AUTO_INCREMENT=1;

-- ----------------------------
-- Auto increment value for `rank20170117`
-- ----------------------------
ALTER TABLE `rank20170117` AUTO_INCREMENT=1;

-- ----------------------------
-- Auto increment value for `rank20170118`
-- ----------------------------
ALTER TABLE `rank20170118` AUTO_INCREMENT=1;
