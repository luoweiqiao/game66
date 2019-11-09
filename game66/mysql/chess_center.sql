/*
Navicat MySQL Data Transfer

Source Server         : 趣味1
Source Server Version : 50173
Source Host           : 120.24.54.6:3307
Source Database       : chess_center

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 65001

Date: 2017-01-18 16:54:08
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `count`
-- ----------------------------
DROP TABLE IF EXISTS `count`;
CREATE TABLE `count` (
`cate`  int(10) NOT NULL COMMENT '统计类型' ,
`value`  bigint(20) NOT NULL COMMENT '值' ,
`date`  varchar(11) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '这个字段在这里无用' ,
`agid`  int(10) NOT NULL DEFAULT 1 ,
`cnid`  int(10) NOT NULL ,
PRIMARY KEY (`cate`, `agid`, `cnid`),
INDEX `agid` USING BTREE (`agid`) ,
INDEX `cnid` USING BTREE (`cnid`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='日统计表'

;

-- ----------------------------
-- Table structure for `cunique`
-- ----------------------------
DROP TABLE IF EXISTS `cunique`;
CREATE TABLE `cunique` (
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`date`  varchar(11) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '日期' ,
`cate`  int(10) NOT NULL COMMENT '统计分类' ,
PRIMARY KEY (`uid`, `date`, `cate`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='统计按用户唯一保证表'

;

-- ----------------------------
-- Table structure for `daycount`
-- ----------------------------
DROP TABLE IF EXISTS `daycount`;
CREATE TABLE `daycount` (
`date`  varchar(11) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '日期' ,
`cate`  int(10) NOT NULL COMMENT '统计类型' ,
`value`  bigint(20) NOT NULL COMMENT '值' ,
`agid`  int(10) NOT NULL DEFAULT 1 ,
`cnid`  int(10) NOT NULL ,
PRIMARY KEY (`date`, `cate`, `agid`, `cnid`),
INDEX `agid` USING BTREE (`agid`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='日统计表'

;

-- ----------------------------
-- Table structure for `monthcount`
-- ----------------------------
DROP TABLE IF EXISTS `monthcount`;
CREATE TABLE `monthcount` (
`date`  varchar(11) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '日期' ,
`cate`  int(10) NOT NULL COMMENT '统计类型' ,
`value`  bigint(20) NOT NULL COMMENT '值' ,
`agid`  int(10) NOT NULL DEFAULT 1 ,
`cnid`  int(10) NOT NULL ,
PRIMARY KEY (`date`, `cate`, `agid`, `cnid`),
INDEX `agid` USING BTREE (`agid`) ,
INDEX `cnid` USING BTREE (`cnid`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='日统计表'

;
