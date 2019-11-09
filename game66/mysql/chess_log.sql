/*
Navicat MySQL Data Transfer

Source Server         : 趣味1
Source Server Version : 50173
Source Host           : 120.24.54.6:3307
Source Database       : chess_log

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 65001

Date: 2017-01-18 17:07:14
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `actionlog`
-- ----------------------------
DROP TABLE IF EXISTS `actionlog`;
CREATE TABLE `actionlog` (
`id`  int(10) NOT NULL AUTO_INCREMENT ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`iconname`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '图标名称' ,
`ptime`  timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '操作时间' ,
`param`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '客户端参数' ,
`data`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '返回结果' ,
PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户行为表'
AUTO_INCREMENT=117667

;

-- ----------------------------
-- Table structure for `clienterrorlog`
-- ----------------------------
DROP TABLE IF EXISTS `clienterrorlog`;
CREATE TABLE `clienterrorlog` (
`eid`  int(11) NOT NULL AUTO_INCREMENT ,
`uid`  int(10) NOT NULL COMMENT '用户ID' ,
`etime`  datetime NOT NULL COMMENT '报错时间' ,
`msg`  longtext CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '报错内容' ,
`ip`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'IP地址' ,
`deviceid`  varchar(12) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '设备号ID' ,
`model`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '设备型号' ,
`version`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '版本信息' ,
PRIMARY KEY (`eid`),
INDEX `etime` USING BTREE (`etime`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='客户端错误日志'
AUTO_INCREMENT=62064

;

-- ----------------------------
-- Table structure for `proomfee`
-- ----------------------------
DROP TABLE IF EXISTS `proomfee`;
CREATE TABLE `proomfee` (
`tableid`  int(10) NOT NULL COMMENT '10' ,
`uid`  int(10) NOT NULL ,
`day`  varchar(11) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`devote`  int(10) NOT NULL ,
INDEX `tableid` USING BTREE (`tableid`, `uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `userlogin`
-- ----------------------------
DROP TABLE IF EXISTS `userlogin`;
CREATE TABLE `userlogin` (
`date`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '日期' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`rtime`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '用户注册时间' ,
`agid`  int(10) NOT NULL COMMENT '代理商id' ,
`cnid`  int(10) NOT NULL COMMENT '渠道id' ,
PRIMARY KEY (`date`, `uid`),
INDEX `agid` USING BTREE (`agid`) ,
INDEX `cnid` USING BTREE (`cnid`) ,
INDEX `rtime` USING BTREE (`rtime`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户登陆日志'

;

-- ----------------------------
-- Table structure for `userloginlog`
-- ----------------------------
DROP TABLE IF EXISTS `userloginlog`;
CREATE TABLE `userloginlog` (
`id`  int(11) NOT NULL AUTO_INCREMENT ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`deviceid`  varchar(12) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '设备id' ,
`ltime`  int(10) NOT NULL COMMENT '登陆时间' ,
PRIMARY KEY (`id`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户'
AUTO_INCREMENT=3815

;

-- ----------------------------
-- Table structure for `userunusual`
-- ----------------------------
DROP TABLE IF EXISTS `userunusual`;
CREATE TABLE `userunusual` (
`id`  int(10) NOT NULL AUTO_INCREMENT COMMENT 'id' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`ip`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'ip地址' ,
`deviceid`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '设备号' ,
`untitle`  varchar(100) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '异常标题' ,
`error`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '异常详情' ,
`param`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '传入参数' ,
`ptime`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '时间' ,
PRIMARY KEY (`id`),
INDEX `uid` USING BTREE (`uid`, `ip`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户异常行为操作记录'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Auto increment value for `actionlog`
-- ----------------------------
ALTER TABLE `actionlog` AUTO_INCREMENT=117667;

-- ----------------------------
-- Auto increment value for `clienterrorlog`
-- ----------------------------
ALTER TABLE `clienterrorlog` AUTO_INCREMENT=62064;

-- ----------------------------
-- Auto increment value for `userloginlog`
-- ----------------------------
ALTER TABLE `userloginlog` AUTO_INCREMENT=3815;

-- ----------------------------
-- Auto increment value for `userunusual`
-- ----------------------------
ALTER TABLE `userunusual` AUTO_INCREMENT=1;
