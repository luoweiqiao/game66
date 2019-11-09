/*
Navicat MySQL Data Transfer

Source Server         : 趣味1
Source Server Version : 50173
Source Host           : 120.24.54.6:3307
Source Database       : chess_centerr

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 65001

Date: 2017-01-18 16:59:43
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `dayincome`
-- ----------------------------
DROP TABLE IF EXISTS `dayincome`;
CREATE TABLE `dayincome` (
`date`  varchar(11) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '日期' ,
`gametype`  int(10) NOT NULL COMMENT '游戏类型' ,
`diamond`  bigint(20) NOT NULL COMMENT '钻石' ,
`coin`  bigint(20) NOT NULL COMMENT '财富币' ,
`score`  bigint(20) NOT NULL COMMENT '积分' ,
PRIMARY KEY (`date`, `gametype`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='日营收记录'

;

-- ----------------------------
-- Table structure for `income`
-- ----------------------------
DROP TABLE IF EXISTS `income`;
CREATE TABLE `income` (
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`diamonda`  bigint(20) NOT NULL COMMENT '钻石存入' ,
`coina`  bigint(20) NOT NULL COMMENT '财富币存入' ,
`scorea`  bigint(20) NOT NULL COMMENT '积分存入' ,
`diamondd`  bigint(20) NOT NULL COMMENT '钻石取出' ,
`coind`  bigint(20) NOT NULL COMMENT '财富币取出' ,
`scored`  bigint(20) NOT NULL COMMENT '积分取出' ,
`dat`  int(10) NOT NULL COMMENT '钻石存入次数' ,
`cat`  int(10) NOT NULL COMMENT '财富币存入次数' ,
`sat`  int(10) NOT NULL COMMENT '积分存入次数' ,
`ddt`  int(10) NOT NULL COMMENT '钻石取出次数' ,
`cdt`  int(10) NOT NULL COMMENT '财富币取出次数' ,
`sdt`  int(10) NOT NULL COMMENT '积分取出次数' ,
PRIMARY KEY (`uid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='机器人营收表'

;

-- ----------------------------
-- Table structure for `incomelist`
-- ----------------------------
DROP TABLE IF EXISTS `incomelist`;
CREATE TABLE `incomelist` (
`id`  int(11) NOT NULL AUTO_INCREMENT COMMENT '主键' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`atype`  tinyint(2) NOT NULL COMMENT '货币类型' ,
`amount`  int(10) NOT NULL COMMENT '金额' ,
`oldv`  int(10) NOT NULL COMMENT '操作前' ,
`newv`  int(10) NOT NULL COMMENT '操作后' ,
`income`  bigint(20) NOT NULL COMMENT '总营收' ,
`ptime`  int(10) NOT NULL COMMENT '操作时间' ,
PRIMARY KEY (`id`),
INDEX `uid` USING BTREE (`uid`) ,
INDEX `ptime` USING BTREE (`ptime`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='机器人营收明细表'
AUTO_INCREMENT=13211

;

-- ----------------------------
-- Table structure for `keyvalue`
-- ----------------------------
DROP TABLE IF EXISTS `keyvalue`;
CREATE TABLE `keyvalue` (
`gametype`  int(10) NOT NULL COMMENT '用户id' ,
`nkey`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'key' ,
`value`  bigint(20) NOT NULL COMMENT 'value' ,
PRIMARY KEY (`gametype`, `nkey`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='杂项'

;

-- ----------------------------
-- Table structure for `sysincomeact`
-- ----------------------------
DROP TABLE IF EXISTS `sysincomeact`;
CREATE TABLE `sysincomeact` (
`id`  int(10) NOT NULL AUTO_INCREMENT COMMENT 'id' ,
`gametype`  int(10) NOT NULL COMMENT '游戏类型' ,
`atype`  tinyint(2) NOT NULL COMMENT '货币类型' ,
`amount`  bigint(20) NOT NULL COMMENT '金额' ,
`aid`  int(10) NOT NULL COMMENT '管理员名称' ,
`anick`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '管理员姓名' ,
`oldincome`  bigint(20) NOT NULL COMMENT '操作前总营收' ,
`ptime`  int(10) NOT NULL COMMENT '操作时间' ,
PRIMARY KEY (`id`),
INDEX `atype` USING BTREE (`atype`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='货币总量操作日志表'
AUTO_INCREMENT=27

;

-- ----------------------------
-- Auto increment value for `incomelist`
-- ----------------------------
ALTER TABLE `incomelist` AUTO_INCREMENT=13211;

-- ----------------------------
-- Auto increment value for `sysincomeact`
-- ----------------------------
ALTER TABLE `sysincomeact` AUTO_INCREMENT=27;
