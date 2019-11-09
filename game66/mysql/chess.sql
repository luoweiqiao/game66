/*
Navicat MySQL Data Transfer

Source Server         : 趣味1
Source Server Version : 50173
Source Host           : 120.24.54.6:3307
Source Database       : chess

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 65001

Date: 2017-01-18 16:52:42
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `account0`
-- ----------------------------
DROP TABLE IF EXISTS `account0`;
CREATE TABLE `account0` (
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`diamond`  bigint(20) NOT NULL COMMENT '钻石币' ,
`coin`  bigint(20) NOT NULL COMMENT '财富币' ,
`ingot`  bigint(20) NOT NULL COMMENT '元宝' ,
`score`  bigint(20) NOT NULL COMMENT '积分' ,
`cvalue`  int(10) NOT NULL COMMENT '魅力值' ,
`vip`  int(10) NOT NULL COMMENT 'vip' ,
`safecoin`  bigint(20) NOT NULL COMMENT '保险箱财富币' ,
`bankrupt`  int(10) NOT NULL COMMENT '破产次数' ,
`feewin`  bigint(20) NOT NULL DEFAULT 0 COMMENT '抽水赢' ,
`feelose`  bigint(20) NOT NULL DEFAULT 0 COMMENT '抽水输' ,
`sysreward`  bigint(10) NOT NULL COMMENT '系统获利' ,
`Recharge`  bigint(10) NOT NULL COMMENT '充值总金额' ,
`converts`  bigint(10) NOT NULL COMMENT '总提现金额(兑换)' ,
`win`  bigint(10) NOT NULL COMMENT '用户牌局总输赢' ,
`reward`  bigint(10) NOT NULL COMMENT '累计领奖' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `admin`
-- ----------------------------
DROP TABLE IF EXISTS `admin`;
CREATE TABLE `admin` (
`aid`  int(10) UNSIGNED NOT NULL AUTO_INCREMENT ,
`aname`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '管理员名字' ,
`apwd`  char(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '管理员密码' ,
`ajointime`  int(10) UNSIGNED NOT NULL COMMENT '成为管理员的时间' ,
`aactivetime`  int(10) UNSIGNED NOT NULL COMMENT '最后登录时间' ,
`aaddid`  int(10) UNSIGNED NOT NULL COMMENT '添加人ID' ,
`anick`  varchar(30) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`checker`  int(11) NOT NULL DEFAULT 37 COMMENT '审核人' ,
`aphone`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '电话' ,
`atype`  tinyint(3) UNSIGNED NOT NULL COMMENT '管理员状态,0:站点管理员,1:总管理员' ,
`astatus`  tinyint(3) UNSIGNED NOT NULL COMMENT '0:活跃1:禁用' ,
`arules`  varchar(1000) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '角色' ,
`aroleid`  int(10) UNSIGNED NOT NULL COMMENT '角色ID' ,
`agid`  int(10) NOT NULL COMMENT '代理ID' ,
`cnid`  int(10) NOT NULL COMMENT '渠道ID' ,
PRIMARY KEY (`aid`),
UNIQUE INDEX `aname` USING BTREE (`aname`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='后台管理员表'
AUTO_INCREMENT=75

;

-- ----------------------------
-- Table structure for `admingivelog`
-- ----------------------------
DROP TABLE IF EXISTS `admingivelog`;
CREATE TABLE `admingivelog` (
`aid`  int(11) NOT NULL AUTO_INCREMENT ,
`gtime`  int(10) NOT NULL COMMENT '赠送时间' ,
`atype`  int(2) NOT NULL ,
`ruid`  int(20) NOT NULL COMMENT '受赠用户ID' ,
`reason`  varchar(100) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '变更原因' ,
`amount`  bigint(20) NOT NULL COMMENT '赠送数量' ,
`oldv`  bigint(20) NOT NULL COMMENT '变更前数量' ,
`newv`  bigint(20) NOT NULL COMMENT '变更后数量' ,
`agid`  int(2) NOT NULL COMMENT '注册来源-代理ID' ,
`cnid`  int(2) NOT NULL ,
`suid`  int(20) NOT NULL COMMENT '操作员ID' ,
`sname`  varchar(10) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '操作员昵称' ,
`sip`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '操作人IP地址' ,
PRIMARY KEY (`aid`),
INDEX `gtime` USING BTREE (`gtime`) ,
INDEX `atype` USING BTREE (`atype`) ,
INDEX `ruid` USING BTREE (`ruid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='管理员操作记录'
AUTO_INCREMENT=172

;

-- ----------------------------
-- Table structure for `changeuidinfo`
-- ----------------------------
DROP TABLE IF EXISTS `changeuidinfo`;
CREATE TABLE `changeuidinfo` (
`id`  int(11) NOT NULL AUTO_INCREMENT ,
`newuid`  int(10) NOT NULL COMMENT '新的用户ID' ,
`olduid`  int(10) NOT NULL COMMENT '旧的用户ID' ,
PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='新旧用户ID对照表'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Table structure for `emailuser`
-- ----------------------------
DROP TABLE IF EXISTS `emailuser`;
CREATE TABLE `emailuser` (
`eid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '邮件id' ,
`efid`  int(10) NOT NULL COMMENT '邮件模板id' ,
`type`  tinyint(1) NOT NULL DEFAULT 1 COMMENT '消息类型(1系统消息2兑换通知,3支付通知,4好友邮件)' ,
`suid`  int(10) NOT NULL COMMENT '发送者id' ,
`ruid`  int(10) NOT NULL COMMENT '接收者id' ,
`title`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '标题' ,
`content`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '内容' ,
`attachment`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '附件' ,
`status`  tinyint(1) NOT NULL COMMENT '状态(0未读,1已读)' ,
`isget`  tinyint(1) NOT NULL COMMENT '是否领取(0未领取,1已领取)' ,
`nickname`  varchar(10) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '来自' ,
`ptime`  int(10) NOT NULL COMMENT '发送时间' ,
PRIMARY KEY (`eid`),
INDEX `suid` USING BTREE (`suid`) ,
INDEX `ruid` USING BTREE (`ruid`) ,
INDEX `ptime` USING BTREE (`ptime`) ,
INDEX `ptime_2` USING BTREE (`ptime`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户邮件'
AUTO_INCREMENT=554

;

-- ----------------------------
-- Table structure for `feedback`
-- ----------------------------
DROP TABLE IF EXISTS `feedback`;
CREATE TABLE `feedback` (
`fid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '反馈id' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`content`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '反馈内容' ,
`isback`  tinyint(2) NOT NULL COMMENT '是不是系统回复' ,
`ptime`  int(10) NOT NULL COMMENT '时间' ,
`status`  tinyint(1) NOT NULL COMMENT '状态(0未处理,1已处理)' ,
PRIMARY KEY (`fid`),
INDEX `uid` USING BTREE (`uid`) ,
INDEX `ptime` USING BTREE (`ptime`) ,
INDEX `status` USING BTREE (`status`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='反馈数据'
AUTO_INCREMENT=54

;

-- ----------------------------
-- Table structure for `forbiduser`
-- ----------------------------
DROP TABLE IF EXISTS `forbiduser`;
CREATE TABLE `forbiduser` (
`fid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '封号id' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`ptype`  tinyint(1) NOT NULL DEFAULT 1 COMMENT '操作类型1封号,2解封,0封号解除' ,
`ftime`  int(10) NOT NULL COMMENT '封号截止时间0表示永久' ,
`ptime`  int(10) NOT NULL COMMENT '操作时间' ,
`reason`  varchar(100) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '封号原因' ,
PRIMARY KEY (`fid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='封号表'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Table structure for `game_baccarat`
-- ----------------------------
DROP TABLE IF EXISTS `game_baccarat`;
CREATE TABLE `game_baccarat` (
`uid`  int(11) NOT NULL COMMENT '用户ID' ,
`win`  int(11) NOT NULL COMMENT '积分赢' ,
`lose`  int(11) NOT NULL COMMENT '积分输' ,
`maxwin`  bigint(20) NOT NULL COMMENT '积分最大赢' ,
`winc`  int(11) NOT NULL COMMENT '财富币赢' ,
`losec`  int(11) NOT NULL COMMENT '财富币输' ,
`maxwinc`  bigint(20) NOT NULL COMMENT '财富币最大赢' ,
`maxcard`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`maxcardc`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`daywin`  bigint(20) NOT NULL COMMENT '今日输赢' ,
`daywinc`  bigint(20) NOT NULL COMMENT '进入输赢' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `game_bainiu`
-- ----------------------------
DROP TABLE IF EXISTS `game_bainiu`;
CREATE TABLE `game_bainiu` (
`uid`  int(11) NOT NULL COMMENT '用户ID' ,
`win`  int(11) NOT NULL COMMENT '积分赢' ,
`lose`  int(11) NOT NULL COMMENT '积分输' ,
`maxwin`  bigint(20) NOT NULL COMMENT '积分最大赢' ,
`winc`  int(11) NOT NULL COMMENT '财富币赢' ,
`losec`  int(11) NOT NULL COMMENT '财富币输' ,
`maxwinc`  bigint(20) NOT NULL COMMENT '财富币最大赢' ,
`maxcard`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`maxcardc`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`daywin`  bigint(20) NOT NULL COMMENT '今日输赢' ,
`daywinc`  bigint(20) NOT NULL COMMENT '进入输赢' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `game_land`
-- ----------------------------
DROP TABLE IF EXISTS `game_land`;
CREATE TABLE `game_land` (
`uid`  int(10) UNSIGNED NOT NULL ,
`win`  int(10) UNSIGNED NOT NULL COMMENT '赢局数' ,
`lose`  int(10) UNSIGNED NOT NULL COMMENT '输局数' ,
`land`  int(10) UNSIGNED NOT NULL COMMENT '地主次数' ,
`spring`  int(10) UNSIGNED NOT NULL COMMENT '春天次数' ,
`winc`  int(11) NOT NULL COMMENT '财富币赢局' ,
`losec`  int(11) NOT NULL COMMENT '财富币输' ,
`landc`  int(11) NOT NULL COMMENT '财富币地主' ,
`maxwin`  bigint(20) NOT NULL COMMENT '单局赢最大积分' ,
`maxwinc`  bigint(20) NOT NULL COMMENT '单局最大赢财富币' ,
`springc`  varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '财富币春天次数' ,
`daywin`  bigint(20) NOT NULL COMMENT '今日赢' ,
`daywinc`  bigint(20) NOT NULL COMMENT '今日赢' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `game_niuniu`
-- ----------------------------
DROP TABLE IF EXISTS `game_niuniu`;
CREATE TABLE `game_niuniu` (
`uid`  int(11) NOT NULL COMMENT '用户ID' ,
`win`  int(11) NOT NULL COMMENT '积分赢' ,
`lose`  int(11) NOT NULL COMMENT '积分输' ,
`maxwin`  bigint(20) NOT NULL COMMENT '积分最大赢' ,
`winc`  int(11) NOT NULL COMMENT '财富币赢' ,
`losec`  int(11) NOT NULL COMMENT '财富币输' ,
`maxwinc`  bigint(20) NOT NULL COMMENT '财富币最大赢' ,
`maxcard`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`maxcardc`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`daywin`  bigint(20) NOT NULL COMMENT '今日输赢' ,
`daywinc`  bigint(20) NOT NULL COMMENT '进入输赢' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `game_paijiu`
-- ----------------------------
DROP TABLE IF EXISTS `game_paijiu`;
CREATE TABLE `game_paijiu` (
`uid`  int(11) NOT NULL COMMENT '用户ID' ,
`win`  int(11) NOT NULL COMMENT '积分赢' ,
`lose`  int(11) NOT NULL COMMENT '积分输' ,
`maxwin`  bigint(20) NOT NULL COMMENT '积分最大赢' ,
`winc`  int(11) NOT NULL COMMENT '财富币赢' ,
`losec`  int(11) NOT NULL COMMENT '财富币输' ,
`maxwinc`  bigint(20) NOT NULL COMMENT '财富币最大赢' ,
`maxcard`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`maxcardc`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`daywin`  bigint(20) NOT NULL COMMENT '今日输赢' ,
`daywinc`  bigint(20) NOT NULL COMMENT '进入输赢' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `game_sangong`
-- ----------------------------
DROP TABLE IF EXISTS `game_sangong`;
CREATE TABLE `game_sangong` (
`uid`  int(11) NOT NULL COMMENT '用户ID' ,
`win`  int(11) NOT NULL COMMENT '积分赢' ,
`lose`  int(11) NOT NULL COMMENT '积分输' ,
`maxwin`  bigint(20) NOT NULL COMMENT '积分最大赢' ,
`winc`  int(11) NOT NULL COMMENT '财富币赢' ,
`losec`  int(11) NOT NULL COMMENT '财富币输' ,
`maxwinc`  bigint(20) NOT NULL COMMENT '财富币最大赢' ,
`maxcard`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`maxcardc`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`daywin`  bigint(20) NOT NULL COMMENT '今日输赢' ,
`daywinc`  bigint(20) NOT NULL COMMENT '进入输赢' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `game_showhand`
-- ----------------------------
DROP TABLE IF EXISTS `game_showhand`;
CREATE TABLE `game_showhand` (
`uid`  int(11) NOT NULL COMMENT '用户ID' ,
`win`  int(11) NOT NULL COMMENT '积分赢' ,
`lose`  int(11) NOT NULL COMMENT '积分输' ,
`maxwin`  bigint(20) NOT NULL COMMENT '积分最大赢' ,
`winc`  int(11) NOT NULL COMMENT '财富币赢' ,
`losec`  int(11) NOT NULL COMMENT '财富币输' ,
`maxwinc`  bigint(20) NOT NULL COMMENT '财富币最大赢' ,
`maxcard`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`maxcardc`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`daywin`  bigint(20) NOT NULL COMMENT '今日输赢' ,
`daywinc`  bigint(20) NOT NULL COMMENT '进入输赢' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `game_texas`
-- ----------------------------
DROP TABLE IF EXISTS `game_texas`;
CREATE TABLE `game_texas` (
`uid`  int(11) NOT NULL COMMENT '用户ID' ,
`win`  int(11) NOT NULL COMMENT '积分赢' ,
`lose`  int(11) NOT NULL COMMENT '积分输' ,
`maxwin`  bigint(20) NOT NULL COMMENT '积分最大赢' ,
`winc`  int(11) NOT NULL COMMENT '财富币赢' ,
`losec`  int(11) NOT NULL COMMENT '财富币输' ,
`maxwinc`  bigint(20) NOT NULL COMMENT '财富币最大赢' ,
`maxcard`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`maxcardc`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`daywin`  bigint(20) NOT NULL COMMENT '今日输赢' ,
`daywinc`  bigint(20) NOT NULL COMMENT '进入输赢' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `game_zajinhua`
-- ----------------------------
DROP TABLE IF EXISTS `game_zajinhua`;
CREATE TABLE `game_zajinhua` (
`uid`  int(11) NOT NULL COMMENT '用户ID' ,
`win`  int(11) NOT NULL COMMENT '积分赢' ,
`lose`  int(11) NOT NULL COMMENT '积分输' ,
`maxwin`  bigint(20) NOT NULL COMMENT '积分最大赢' ,
`winc`  int(11) NOT NULL COMMENT '财富币赢' ,
`losec`  int(11) NOT NULL COMMENT '财富币输' ,
`maxwinc`  bigint(20) NOT NULL COMMENT '财富币最大赢' ,
`maxcard`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`maxcardc`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最大牌型' ,
`daywin`  bigint(20) NOT NULL COMMENT '今日输赢' ,
`daywinc`  bigint(20) NOT NULL COMMENT '进入输赢' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `memberonline`
-- ----------------------------
DROP TABLE IF EXISTS `memberonline`;
CREATE TABLE `memberonline` (
`uid`  int(10) UNSIGNED NOT NULL COMMENT '用户ID' ,
`mokey`  varchar(32) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '用户key,session_key' ,
`sid`  int(10) UNSIGNED NOT NULL ,
`mocount`  int(10) UNSIGNED NOT NULL COMMENT '计数器,用于统计更新了多少次' ,
`motime`  int(10) UNSIGNED NOT NULL ,
`mologintime`  int(10) UNSIGNED NOT NULL ,
`rsvid`  tinyint(3) NOT NULL DEFAULT 0 COMMENT '所属房间服务器id' ,
`roomid`  int(10) UNSIGNED NOT NULL DEFAULT 0 COMMENT '所在房间id,0表示空' ,
`gsvid`  tinyint(3) NOT NULL DEFAULT 0 COMMENT '所在的游戏服务器id' ,
`groomid`  int(10) UNSIGNED NOT NULL DEFAULT 0 COMMENT '所在游戏房间id,0表示空' ,
`status`  int(10) UNSIGNED NOT NULL DEFAULT 0 COMMENT '状态(0在家，1在小镇，2在战斗，3在训练场)' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户在线表'

;

-- ----------------------------
-- Table structure for `memcachecfg`
-- ----------------------------
DROP TABLE IF EXISTS `memcachecfg`;
CREATE TABLE `memcachecfg` (
`id`  int(5) NOT NULL AUTO_INCREMENT ,
`cname`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '缓存名称' ,
`hindex`  tinyint(2) NOT NULL COMMENT 'hash索引' ,
`ip`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'ip' ,
`port`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '端口' ,
PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='缓存配置表'
AUTO_INCREMENT=5

;

-- ----------------------------
-- Table structure for `onlinecount`
-- ----------------------------
DROP TABLE IF EXISTS `onlinecount`;
CREATE TABLE `onlinecount` (
`id`  int(10) NOT NULL AUTO_INCREMENT ,
`ctime`  int(10) NOT NULL COMMENT '10' ,
`online`  int(10) NOT NULL COMMENT '玩家在线' ,
`robot`  int(10) NOT NULL COMMENT '机器人在线' ,
`play`  int(10) NOT NULL COMMENT '玩家在玩' ,
`robotplay`  int(10) NOT NULL COMMENT '机器人在玩' ,
`playdetail`  longtext CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '在玩详情' ,
PRIMARY KEY (`id`),
INDEX `ctime` USING BTREE (`ctime`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=2364

;

-- ----------------------------
-- Table structure for `onlinedetail`
-- ----------------------------
DROP TABLE IF EXISTS `onlinedetail`;
CREATE TABLE `onlinedetail` (
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`svrid`  int(10) NOT NULL COMMENT '服务器id' ,
`roomid`  int(10) NOT NULL COMMENT '房间id' ,
`isrobot`  int(11) NOT NULL DEFAULT 0 COMMENT '是否机器人' ,
`coin`  bigint(20) NOT NULL COMMENT '财富币' ,
`safecoin`  int(11) NOT NULL COMMENT '保险箱财富币' ,
`score`  bigint(20) NOT NULL COMMENT '积分' ,
`ingot`  bigint(20) NOT NULL ,
PRIMARY KEY (`uid`),
INDEX `svrid` USING BTREE (`svrid`, `roomid`) ,
INDEX `coin` USING BTREE (`coin`) ,
INDEX `safecoin` USING BTREE (`safecoin`) ,
INDEX `score` USING BTREE (`score`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='在线详情'

;

-- ----------------------------
-- Table structure for `passport`
-- ----------------------------
DROP TABLE IF EXISTS `passport`;
CREATE TABLE `passport` (
`uid`  int(10) NOT NULL AUTO_INCREMENT ,
`phone`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '手机号' ,
`email`  varchar(30) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '邮箱' ,
`ano`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '账号' ,
`rtype`  int(10) NOT NULL COMMENT '注册类型（0手机1邮箱2微信3qq）' ,
`sitemid`  varchar(100) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '第三方账号id' ,
`pwd`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '密码' ,
`siteid`  int(10) NOT NULL DEFAULT 1 COMMENT '代理商渠道id' ,
PRIMARY KEY (`uid`),
INDEX `ttype` USING BTREE (`rtype`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='通行证表'
AUTO_INCREMENT=60

;

-- ----------------------------
-- Table structure for `payfor`
-- ----------------------------
DROP TABLE IF EXISTS `payfor`;
CREATE TABLE `payfor` (
`pid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '支付id' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`consume`  tinyint(2) NOT NULL COMMENT '货币类型' ,
`amount`  int(10) NOT NULL COMMENT '原币金额' ,
`qty`  int(10) NOT NULL COMMENT '获得数量' ,
`rmb`  int(10) NOT NULL COMMENT '人民币金额' ,
`ptime`  int(10) NOT NULL COMMENT '操作时间' ,
PRIMARY KEY (`pid`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='支付记录表'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Table structure for `proom`
-- ----------------------------
DROP TABLE IF EXISTS `proom`;
CREATE TABLE `proom` (
`tableid`  int(10) UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '桌子ID' ,
`tablename`  varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '桌子名称' ,
`hostname`  varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '房主名称' ,
`hostid`  int(10) NOT NULL COMMENT '房主ID' ,
`passwd`  varchar(16) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '桌子密码' ,
`duetime`  int(10) NOT NULL COMMENT '过期时间' ,
`basescore`  int(10) NOT NULL COMMENT '底分' ,
`deal`  smallint(6) NOT NULL COMMENT '发牌方式' ,
`entermin`  int(10) NOT NULL COMMENT '最小进入' ,
`consume`  smallint(6) NOT NULL COMMENT '消费类型' ,
`feetype`  smallint(6) NOT NULL COMMENT '台费方式' ,
`feevalue`  int(11) NOT NULL COMMENT '台费万分比值' ,
`display`  smallint(6) NOT NULL COMMENT '是否显示' ,
`hostincome`  bigint(20) NOT NULL COMMENT '房主收益' ,
`sysincome`  bigint(20) NOT NULL COMMENT '系统收益' ,
`gametype`  int(10) NOT NULL COMMENT '游戏类型' ,
`createtime`  int(10) NOT NULL COMMENT '创建时间' ,
PRIMARY KEY (`tableid`),
INDEX `hostid` USING BTREE (`hostid`) ,
INDEX `gametype` USING BTREE (`gametype`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=1

;

-- ----------------------------
-- Table structure for `rank`
-- ----------------------------
DROP TABLE IF EXISTS `rank`;
CREATE TABLE `rank` (
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`score`  bigint(20) NOT NULL COMMENT '积分' ,
`coin`  bigint(20) NOT NULL COMMENT '财富币' ,
PRIMARY KEY (`uid`),
INDEX `score` USING BTREE (`score`) ,
INDEX `diamond` USING BTREE (`coin`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='排行榜'

;

-- ----------------------------
-- Table structure for `rebate`
-- ----------------------------
DROP TABLE IF EXISTS `rebate`;
CREATE TABLE `rebate` (
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`totalsend`  bigint(20) NOT NULL COMMENT '赠送总计' ,
`hadget`  bigint(20) NOT NULL COMMENT '已领取' ,
PRIMARY KEY (`uid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='赠送返点表'

;

-- ----------------------------
-- Table structure for `robot`
-- ----------------------------
DROP TABLE IF EXISTS `robot`;
CREATE TABLE `robot` (
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`attype`  tinyint(2) NOT NULL DEFAULT 0 COMMENT '活跃类型' ,
`pltype`  tinyint(2) NOT NULL DEFAULT 0 COMMENT '打牌类型' ,
`scorelv`  tinyint(4) NOT NULL ,
`richlv`  tinyint(4) NOT NULL DEFAULT 5 COMMENT '财富等级' ,
`status`  tinyint(2) NOT NULL COMMENT '机器人状态(0正常1禁用)' ,
`d1`  tinyint(1) NOT NULL COMMENT '星期一是否活跃' ,
`d2`  tinyint(1) NOT NULL COMMENT '星期二是否活跃' ,
`d3`  tinyint(1) NOT NULL COMMENT '星期三' ,
`d4`  tinyint(1) NOT NULL COMMENT '星期四' ,
`d5`  tinyint(1) NOT NULL COMMENT '星期五' ,
`d6`  tinyint(1) NOT NULL COMMENT '星期六' ,
`d7`  tinyint(1) NOT NULL COMMENT '星期天' ,
`actiontime`  int(11) NOT NULL DEFAULT 7200 COMMENT '活跃时间' ,
`gametype`  int(11) NOT NULL DEFAULT 0 COMMENT '游戏类型' ,
`logincount`  int(11) NOT NULL DEFAULT 0 COMMENT '登陆次数' ,
`loginstate`  int(11) NOT NULL DEFAULT 0 COMMENT '登陆状态' ,
PRIMARY KEY (`uid`),
INDEX `status` USING BTREE (`status`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='机器人表'

;

-- ----------------------------
-- Table structure for `sacinvertory`
-- ----------------------------
DROP TABLE IF EXISTS `sacinvertory`;
CREATE TABLE `sacinvertory` (
`id`  int(10) NOT NULL AUTO_INCREMENT ,
`date`  int(10) NOT NULL COMMENT '日期' ,
`vtype`  int(5) NOT NULL COMMENT '标签类别' ,
`gametype`  int(10) NOT NULL COMMENT '游戏类型' ,
`icounts`  int(10) NOT NULL COMMENT '用户数' ,
`diamond`  bigint(20) NOT NULL COMMENT '钻石' ,
`coin`  bigint(20) NOT NULL COMMENT '财富币' ,
`score`  bigint(20) NOT NULL COMMENT '积分' ,
`ingot`  int(20) NOT NULL COMMENT '元宝' ,
PRIMARY KEY (`id`),
UNIQUE INDEX `date_2` USING BTREE (`date`, `vtype`, `gametype`) ,
INDEX `date` USING BTREE (`date`) ,
INDEX `vtype` USING BTREE (`vtype`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='日库存结算表'
AUTO_INCREMENT=589

;

-- ----------------------------
-- Table structure for `scoinrecord`
-- ----------------------------
DROP TABLE IF EXISTS `scoinrecord`;
CREATE TABLE `scoinrecord` (
`sid`  bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '记录id(编号)' ,
`suid`  int(10) NOT NULL COMMENT '发送用户id' ,
`ruid`  int(10) NOT NULL COMMENT '接收用户id' ,
`sdeviceid`  varchar(12) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '转账方设备号' ,
`rdeviceid`  varchar(12) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '接收方设备号' ,
`amount`  int(10) NOT NULL COMMENT '金额' ,
`ptime`  int(10) NOT NULL COMMENT '操作时间' ,
`remark`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '备注' ,
`tax`  bigint(20) NOT NULL COMMENT '手续费' ,
`scoin`  bigint(20) NOT NULL COMMENT '发送方最后财富币' ,
`ssafecoin`  bigint(20) NOT NULL COMMENT '发送方最后保险箱财富币' ,
`rcoin`  bigint(20) NOT NULL COMMENT '接收方最后财富币' ,
`rsafecoin`  bigint(20) NOT NULL COMMENT '接收方最后保险箱财富币' ,
`ip`  varchar(30) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '操作ip' ,
PRIMARY KEY (`sid`),
INDEX `suid` USING BTREE (`suid`) ,
INDEX `ruid` USING BTREE (`ruid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户财富币赠送记录表'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Table structure for `secondmsg`
-- ----------------------------
DROP TABLE IF EXISTS `secondmsg`;
CREATE TABLE `secondmsg` (
`id`  int(10) NOT NULL AUTO_INCREMENT ,
`uno`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '手机或者邮箱' ,
`status`  int(2) NOT NULL COMMENT '状态' ,
`ptime`  int(10) NOT NULL COMMENT '插入时间' ,
PRIMARY KEY (`id`),
INDEX `uno` USING BTREE (`uno`, `status`) ,
INDEX `ptime` USING BTREE (`ptime`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='第二条短信'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Table structure for `serverinfo`
-- ----------------------------
DROP TABLE IF EXISTS `serverinfo`;
CREATE TABLE `serverinfo` (
`svrid`  int(10) UNSIGNED NOT NULL COMMENT '服务器ID' ,
`name`  varchar(128) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '服务器名字' ,
`group`  int(10) NOT NULL COMMENT '服务器组' ,
`svr_type`  int(10) UNSIGNED NOT NULL COMMENT '服务器类型(1大厅，2游戏服)' ,
`game_type`  int(10) NOT NULL COMMENT '游戏类型(1斗地主2梭哈3斗牛)' ,
`game_subtype`  int(10) NOT NULL DEFAULT 1 COMMENT '1普通，2私人，3比赛' ,
`svrip`  varchar(128) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`svrlanip`  varchar(128) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,
`svrport`  int(10) UNSIGNED NOT NULL ,
`svrlanport`  int(10) NOT NULL ,
`phpport`  int(10) NOT NULL COMMENT 'php连接端口' ,
`openrobot`  int(11) NOT NULL DEFAULT 0 COMMENT '开启机器人' ,
`state`  int(10) UNSIGNED NOT NULL COMMENT '服务器状态' ,
`onlines`  int(10) UNSIGNED NOT NULL ,
`robots`  int(11) NOT NULL COMMENT '机器人数量' ,
`report_time`  datetime NOT NULL COMMENT '上报时间' ,
PRIMARY KEY (`svrid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `user0`
-- ----------------------------
DROP TABLE IF EXISTS `user0`;
CREATE TABLE `user0` (
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`nickname`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '用户昵称' ,
`sex`  int(10) UNSIGNED NOT NULL COMMENT '性别，0-未知，2-女，1-男' ,
`rtime`  int(10) NOT NULL COMMENT '注册时间' ,
`logintime`  int(10) NOT NULL COMMENT '最后登陆时间' ,
`offlinetime`  int(10) UNSIGNED NOT NULL COMMENT '最后离开游戏时间' ,
`clogin`  int(10) NOT NULL COMMENT '连续登陆天数' ,
`weeklogin`  int(10) NOT NULL COMMENT '本周累计登陆' ,
`alllogin`  int(10) NOT NULL COMMENT '累计登录天数' ,
`onlinetime`  int(10) NOT NULL COMMENT '累计在线时间' ,
`reward`  int(10) NOT NULL COMMENT '奖励标记' ,
`bankrupt`  int(10) NOT NULL COMMENT '破产补助次数' ,
`dgcount`  int(10) NOT NULL DEFAULT 0 COMMENT '今日游戏局数' ,
`setting`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '游戏配置' ,
`safepwd`  varchar(12) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '保险箱密码' ,
`safeproblem`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '安全问题' ,
`safeanswer`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '问题答案' ,
`ispay`  tinyint(1) NOT NULL COMMENT '是否付费用户' ,
`payall`  int(10) NOT NULL COMMENT '支付总额' ,
`paytimes`  int(10) NOT NULL COMMENT '支付次数' ,
`fpaytime`  int(10) NOT NULL COMMENT '第一次支付时间' ,
`lpaytime`  int(10) NOT NULL COMMENT '最后一次支付时间' ,
`ip`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'ip地址' ,
`loginip`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '最后登陆ip' ,
`mac`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '注册mac地址' ,
`signtype`  varchar(200) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '用户标签' ,
`imagetype`  tinyint(1) NOT NULL DEFAULT 1 COMMENT '头像类型(1系统头像2自定义)' ,
`imageurl`  varchar(200) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL DEFAULT '1' COMMENT '头像地址' ,
`deviceid`  varchar(12) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '注册机器' ,
`ldeviceid`  varchar(12) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '登陆机器' ,
`parentuid`  int(10) NOT NULL COMMENT '邀请码' ,
`oldname`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '修改过的昵称' ,
`uptime`  int(10) NOT NULL COMMENT '操作时间' ,
PRIMARY KEY (`uid`),
UNIQUE INDEX `nickname` USING BTREE (`nickname`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户表'

;

-- ----------------------------
-- Table structure for `valid`
-- ----------------------------
DROP TABLE IF EXISTS `valid`;
CREATE TABLE `valid` (
`uno`  varchar(30) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '手机号或者邮箱' ,
`uid`  int(11) NOT NULL COMMENT '10' ,
`mtype`  tinyint(2) NOT NULL COMMENT '类型(0注册1修改密码,2设置支付密码)' ,
`ptime`  int(10) NOT NULL COMMENT '操作时间' ,
`verify`  varchar(10) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '验证码' ,
`status`  tinyint(1) NOT NULL COMMENT '状态(0)' ,
PRIMARY KEY (`uno`),
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户验证码表'

;

-- ----------------------------
-- Table structure for `version`
-- ----------------------------
DROP TABLE IF EXISTS `version`;
CREATE TABLE `version` (
`vid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '版本id' ,
`version`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '版本号' ,
`sites`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '平台' ,
`vtype`  tinyint(2) NOT NULL COMMENT '版本类型(1热更新,2整包)' ,
`packname`  varchar(200) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '包名' ,
`ptime`  int(10) NOT NULL COMMENT '更新时间' ,
`remark`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '备注' ,
`force`  tinyint(1) NOT NULL COMMENT '强制' ,
PRIMARY KEY (`vid`),
UNIQUE INDEX `version` USING BTREE (`version`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='版本更新'
AUTO_INCREMENT=4

;

-- ----------------------------
-- Table structure for `vipreport`
-- ----------------------------
DROP TABLE IF EXISTS `vipreport`;
CREATE TABLE `vipreport` (
`vid`  int(11) NOT NULL AUTO_INCREMENT ,
`ptime`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'VIP报表日期' ,
`total`  bigint(20) NOT NULL COMMENT '总货币量' ,
`intotal`  bigint(20) NOT NULL COMMENT '总累计转入' ,
`outtotal`  bigint(20) NOT NULL COMMENT '总累计转出' ,
`rebate`  int(10) NOT NULL COMMENT '返点比例' ,
`totalrebate`  bigint(20) NOT NULL COMMENT '总累计返额' ,
`gettotal`  bigint(20) NOT NULL COMMENT '总已领取' ,
`resttotal`  bigint(20) NOT NULL COMMENT '可领取剩余' ,
PRIMARY KEY (`vid`),
UNIQUE INDEX `ptime` USING BTREE (`ptime`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='VIP报表'
AUTO_INCREMENT=43

;

-- ----------------------------
-- Auto increment value for `admin`
-- ----------------------------
ALTER TABLE `admin` AUTO_INCREMENT=75;

-- ----------------------------
-- Auto increment value for `admingivelog`
-- ----------------------------
ALTER TABLE `admingivelog` AUTO_INCREMENT=172;

-- ----------------------------
-- Auto increment value for `changeuidinfo`
-- ----------------------------
ALTER TABLE `changeuidinfo` AUTO_INCREMENT=1;

-- ----------------------------
-- Auto increment value for `emailuser`
-- ----------------------------
ALTER TABLE `emailuser` AUTO_INCREMENT=554;

-- ----------------------------
-- Auto increment value for `feedback`
-- ----------------------------
ALTER TABLE `feedback` AUTO_INCREMENT=54;

-- ----------------------------
-- Auto increment value for `forbiduser`
-- ----------------------------
ALTER TABLE `forbiduser` AUTO_INCREMENT=1;

-- ----------------------------
-- Auto increment value for `memcachecfg`
-- ----------------------------
ALTER TABLE `memcachecfg` AUTO_INCREMENT=5;

-- ----------------------------
-- Auto increment value for `onlinecount`
-- ----------------------------
ALTER TABLE `onlinecount` AUTO_INCREMENT=2364;

-- ----------------------------
-- Auto increment value for `passport`
-- ----------------------------
ALTER TABLE `passport` AUTO_INCREMENT=60;

-- ----------------------------
-- Auto increment value for `payfor`
-- ----------------------------
ALTER TABLE `payfor` AUTO_INCREMENT=1;

-- ----------------------------
-- Auto increment value for `proom`
-- ----------------------------
ALTER TABLE `proom` AUTO_INCREMENT=1;

-- ----------------------------
-- Auto increment value for `sacinvertory`
-- ----------------------------
ALTER TABLE `sacinvertory` AUTO_INCREMENT=589;
DROP TRIGGER IF EXISTS `rebate`;
DELIMITER ;;
CREATE TRIGGER `rebate` BEFORE INSERT ON `scoinrecord` FOR EACH ROW begin
if((select vip from account0 where uid = new.ruid) = 0) then
              insert into rebate set uid = new.suid,totalsend = new.amount  ON DUPLICATE KEY UPDATE  totalsend = totalsend + new.amount;        
end if;
set new.rdeviceid = (select ldeviceid from user0 where uid = new.ruid);
if(new.sdeviceid = '') then
set new.sdeviceid = (select ldeviceid from user0 where uid = new.suid);
end if;
end
;;
DELIMITER ;

-- ----------------------------
-- Auto increment value for `scoinrecord`
-- ----------------------------
ALTER TABLE `scoinrecord` AUTO_INCREMENT=1;

-- ----------------------------
-- Auto increment value for `secondmsg`
-- ----------------------------
ALTER TABLE `secondmsg` AUTO_INCREMENT=1;

-- ----------------------------
-- Auto increment value for `version`
-- ----------------------------
ALTER TABLE `version` AUTO_INCREMENT=4;

-- ----------------------------
-- Auto increment value for `vipreport`
-- ----------------------------
ALTER TABLE `vipreport` AUTO_INCREMENT=43;
