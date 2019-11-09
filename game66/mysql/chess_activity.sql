/*
Navicat MySQL Data Transfer

Source Server         : 趣味1
Source Server Version : 50173
Source Host           : 120.24.54.6:3307
Source Database       : chess_activity

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 65001

Date: 2017-01-18 16:53:22
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `actunique`
-- ----------------------------
DROP TABLE IF EXISTS `actunique`;
CREATE TABLE `actunique` (
`uqid`  varchar(50) CHARACTER SET latin1 COLLATE latin1_swedish_ci NOT NULL ,
PRIMARY KEY (`uqid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=latin1 COLLATE=latin1_swedish_ci
COMMENT='唯一键表'

;

-- ----------------------------
-- Table structure for `fpdouble`
-- ----------------------------
DROP TABLE IF EXISTS `fpdouble`;
CREATE TABLE `fpdouble` (
`caid`  int(2) NOT NULL AUTO_INCREMENT ,
`uid`  int(10) NOT NULL COMMENT '用户ID' ,
`ptime`  int(10) NOT NULL COMMENT '操作时间' ,
`atype`  int(11) NOT NULL COMMENT '活动ID' ,
`pid`  int(10) NOT NULL COMMENT '产品ID' ,
`status`  int(2) NOT NULL ,
PRIMARY KEY (`caid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=latin1 COLLATE=latin1_swedish_ci
COMMENT='首充活动'
AUTO_INCREMENT=133

;

-- ----------------------------
-- Table structure for `userbenefits`
-- ----------------------------
DROP TABLE IF EXISTS `userbenefits`;
CREATE TABLE `userbenefits` (
`caid`  int(2) NOT NULL AUTO_INCREMENT ,
`uid`  int(10) NOT NULL COMMENT '用户' ,
`atype`  int(10) NOT NULL COMMENT '活动类型' ,
`ptime`  int(10) NOT NULL ,
`status`  int(2) NOT NULL COMMENT '状态' ,
`times`  int(2) NOT NULL COMMENT '次数' ,
PRIMARY KEY (`caid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=latin1 COLLATE=latin1_swedish_ci
AUTO_INCREMENT=427

;

-- ----------------------------
-- Table structure for `userpaysend`
-- ----------------------------
DROP TABLE IF EXISTS `userpaysend`;
CREATE TABLE `userpaysend` (
`caid`  int(10) NOT NULL AUTO_INCREMENT ,
`uid`  int(10) NOT NULL COMMENT '用户UID' ,
`sumtrans`  bigint(20) NOT NULL COMMENT '总流水' ,
`ctrans`  bigint(20) NOT NULL COMMENT '用掉流水' ,
`gotimes`  text CHARACTER SET latin1 COLLATE latin1_swedish_ci NOT NULL COMMENT '领取的信息' ,
`sumpay`  bigint(20) NOT NULL COMMENT '充值数量' ,
`cpay`  bigint(20) NOT NULL COMMENT '用过的赠送' ,
`ptime`  int(11) NOT NULL COMMENT '操作时间' ,
`status`  int(2) NOT NULL COMMENT '是否存在可以领取奖励' ,
PRIMARY KEY (`caid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=latin1 COLLATE=latin1_swedish_ci
COMMENT='用户充值赠送表'
AUTO_INCREMENT=38

;

-- ----------------------------
-- Auto increment value for `fpdouble`
-- ----------------------------
ALTER TABLE `fpdouble` AUTO_INCREMENT=133;

-- ----------------------------
-- Auto increment value for `userbenefits`
-- ----------------------------
ALTER TABLE `userbenefits` AUTO_INCREMENT=427;

-- ----------------------------
-- Auto increment value for `userpaysend`
-- ----------------------------
ALTER TABLE `userpaysend` AUTO_INCREMENT=38;
