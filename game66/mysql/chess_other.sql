/*
Navicat MySQL Data Transfer

Source Server         : 趣味1
Source Server Version : 50173
Source Host           : 120.24.54.6:3307
Source Database       : chess_other

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 65001

Date: 2017-01-18 17:08:53
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `address`
-- ----------------------------
DROP TABLE IF EXISTS `address`;
CREATE TABLE `address` (
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`name`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '收货人姓名' ,
`phone`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '手机号' ,
`email`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '邮件' ,
`address`  varchar(200) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '地址' ,
`postcode`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '邮编' ,
PRIMARY KEY (`uid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `area`
-- ----------------------------
DROP TABLE IF EXISTS `area`;
CREATE TABLE `area` (
`id`  int(11) NOT NULL AUTO_INCREMENT ,
`areaid`  int(11) NOT NULL ,
`area`  varchar(20) CHARACTER SET gbk COLLATE gbk_chinese_ci NOT NULL ,
`fatherid`  int(11) NOT NULL ,
PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=gb2312 COLLATE=gb2312_chinese_ci
AUTO_INCREMENT=3145

;

-- ----------------------------
-- Table structure for `bankinfo`
-- ----------------------------
DROP TABLE IF EXISTS `bankinfo`;
CREATE TABLE `bankinfo` (
`bid`  int(10) NOT NULL AUTO_INCREMENT ,
`uid`  int(10) NOT NULL COMMENT '用户ID' ,
`name`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '银行开户名' ,
`banker`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '银行名称' ,
`subbranch`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '支行名称' ,
`province`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '开户省份' ,
`city`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '开户城市' ,
`card`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '银行卡号' ,
`ptime`  int(10) NOT NULL COMMENT '绑定时间' ,
PRIMARY KEY (`bid`),
UNIQUE INDEX `bid_2` USING BTREE (`bid`) ,
INDEX `bid` USING BTREE (`bid`) ,
INDEX `uid` USING BTREE (`uid`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=80

;

-- ----------------------------
-- Table structure for `bcpay`
-- ----------------------------
DROP TABLE IF EXISTS `bcpay`;
CREATE TABLE `bcpay` (
`pid`  int(10) NOT NULL AUTO_INCREMENT ,
`uid`  int(10) NOT NULL COMMENT '用户ID' ,
`ptype`  int(2) NOT NULL COMMENT '操作类型' ,
`amount`  bigint(10) NOT NULL COMMENT '兑换金额' ,
`ptime`  int(10) NOT NULL COMMENT '申请时间' ,
`bid`  int(10) NOT NULL COMMENT '银行卡ID' ,
`status`  int(2) NOT NULL DEFAULT 0 COMMENT '0-申请中，1-批准，-1拒绝' ,
`reason`  varchar(200) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '风控审核原因' ,
`chkreason`  varchar(200) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '财务审核原因' ,
PRIMARY KEY (`pid`),
INDEX `status` USING BTREE (`status`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户兑换申请单'
AUTO_INCREMENT=181

;

-- ----------------------------
-- Table structure for `cardno`
-- ----------------------------
DROP TABLE IF EXISTS `cardno`;
CREATE TABLE `cardno` (
`pid`  int(10) NOT NULL COMMENT '商品id' ,
`cardno`  varchar(100) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '卡号' ,
`pwd`  varchar(100) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '密码' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`ptime`  int(10) NOT NULL COMMENT '发货时间' ,
PRIMARY KEY (`cardno`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='卡号'

;

-- ----------------------------
-- Table structure for `city`
-- ----------------------------
DROP TABLE IF EXISTS `city`;
CREATE TABLE `city` (
`id`  int(11) NOT NULL AUTO_INCREMENT ,
`cityid`  int(11) NOT NULL ,
`city`  varchar(20) CHARACTER SET gbk COLLATE gbk_chinese_ci NOT NULL ,
`fatherid`  int(11) NOT NULL ,
PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=gbk COLLATE=gbk_chinese_ci
AUTO_INCREMENT=346

;

-- ----------------------------
-- Table structure for `cuttrans`
-- ----------------------------
DROP TABLE IF EXISTS `cuttrans`;
CREATE TABLE `cuttrans` (
`id`  int(10) NOT NULL AUTO_INCREMENT COMMENT 'id' ,
`asid`  int(10) NOT NULL COMMENT '关联id' ,
`ctype`  tinyint(2) NOT NULL COMMENT '制成类型(1充值抽成,2输赢抽成)' ,
`prid`  int(10) NOT NULL COMMENT '推广员id' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`amt`  decimal(10,2) NOT NULL COMMENT '金额' ,
`ptime`  int(10) NOT NULL COMMENT '时间' ,
PRIMARY KEY (`id`),
INDEX `asid` USING BTREE (`asid`) ,
INDEX `ctype` USING BTREE (`ctype`) ,
INDEX `prid` USING BTREE (`prid`) ,
INDEX `ptime` USING BTREE (`ptime`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='推广员抽成表'
AUTO_INCREMENT=1

;

-- ----------------------------
-- Table structure for `exchangeno`
-- ----------------------------
DROP TABLE IF EXISTS `exchangeno`;
CREATE TABLE `exchangeno` (
`prid`  int(10) NOT NULL COMMENT '推广员id' ,
`exchangeno`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '兑换码' ,
`extimes`  int(10) NOT NULL COMMENT '可兑换次数' ,
`usetimes`  int(10) NOT NULL COMMENT '已使用次数' ,
`etime`  int(10) NOT NULL COMMENT '截止时间' ,
`poid`  int(10) NOT NULL COMMENT '赠送道具id' ,
`qty`  int(10) NOT NULL COMMENT '赠送数量' ,
`etitle`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '邮件标题' ,
`econtent`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '邮件内容' ,
`ptime`  int(10) NOT NULL COMMENT '录入时间' ,
PRIMARY KEY (`exchangeno`),
INDEX `prid` USING BTREE (`prid`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='推广员表'

;

-- ----------------------------
-- Table structure for `exchangenokeys`
-- ----------------------------
DROP TABLE IF EXISTS `exchangenokeys`;
CREATE TABLE `exchangenokeys` (
`exchangeno`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '兑换码' ,
`status`  tinyint(1) NOT NULL COMMENT '状态：0禁用，1启用' ,
INDEX `exchangeno` USING BTREE (`exchangeno`) ,
INDEX `status` USING BTREE (`status`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `iospay`
-- ----------------------------
DROP TABLE IF EXISTS `iospay`;
CREATE TABLE `iospay` (
`pid`  int(10) NOT NULL COMMENT '订单id' ,
`transaction_id`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '苹果支付流水号' ,
`receipt`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '苹果支付记录' ,
PRIMARY KEY (`pid`),
UNIQUE INDEX `transaction_id` USING BTREE (`transaction_id`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='苹果支付记录表'

;

-- ----------------------------
-- Table structure for `pay`
-- ----------------------------
DROP TABLE IF EXISTS `pay`;
CREATE TABLE `pay` (
`id`  int(10) NOT NULL AUTO_INCREMENT ,
`siteid`  int(10) NOT NULL ,
`dpid`  smallint(2) NOT NULL COMMENT '钻石价格id' ,
`rmb`  decimal(10,2) NOT NULL COMMENT '人民币数量' ,
`coin`  int(10) NOT NULL COMMENT '筹码数量' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`status`  tinyint(2) NOT NULL COMMENT '状态(0提单,1支付,2退单,99异常)' ,
`ctime`  int(10) NOT NULL COMMENT '下单时间' ,
`ptime`  int(10) NOT NULL COMMENT '支付时间' ,
`btime`  int(10) NOT NULL COMMENT '退单时间' ,
`remark`  varchar(100) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '备注' ,
PRIMARY KEY (`id`),
INDEX `uid` USING BTREE (`uid`) ,
INDEX `status` USING BTREE (`status`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='支付表'
AUTO_INCREMENT=100002

;

-- ----------------------------
-- Table structure for `paysite`
-- ----------------------------
DROP TABLE IF EXISTS `paysite`;
CREATE TABLE `paysite` (
`id`  int(10) NOT NULL AUTO_INCREMENT COMMENT 'id' ,
`stype`  tinyint(2) NOT NULL COMMENT '平台类型(1支付宝2微信)' ,
`psid`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '平台支付id' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`content`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '内容' ,
`ptime`  int(10) NOT NULL COMMENT '操作时间' ,
PRIMARY KEY (`id`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='平台支付记录表'
AUTO_INCREMENT=418

;

-- ----------------------------
-- Table structure for `product`
-- ----------------------------
DROP TABLE IF EXISTS `product`;
CREATE TABLE `product` (
`pid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '商品id' ,
`pname`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '商品名称' ,
`desc`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '商品描述' ,
`pcate`  tinyint(1) NOT NULL COMMENT '商品大分类(1实物2虚拟)' ,
`pframe`  int(10) NOT NULL COMMENT '商品子分类' ,
`atype`  tinyint(2) NOT NULL COMMENT '兑换货币类型' ,
`price`  int(10) NOT NULL COMMENT '兑换价格' ,
`total`  int(10) NOT NULL COMMENT '总数' ,
`sold`  int(10) NOT NULL COMMENT '已售出' ,
`cover`  varchar(200) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '封面' ,
`images`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '图片' ,
`cost`  decimal(10,2) NOT NULL COMMENT '进货成本(rmb)' ,
`status`  tinyint(2) NOT NULL COMMENT '状态(0下架,1正常)' ,
`sortid`  int(2) NOT NULL COMMENT '排序字段' ,
PRIMARY KEY (`pid`),
INDEX `pcate` USING BTREE (`pcate`, `status`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=19

;

-- ----------------------------
-- Table structure for `promoter`
-- ----------------------------
DROP TABLE IF EXISTS `promoter`;
CREATE TABLE `promoter` (
`prid`  int(11) NOT NULL DEFAULT 0 ,
`aid`  int(10) NOT NULL COMMENT '上级代理商id' ,
`name`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '姓名' ,
`income`  decimal(10,2) NOT NULL COMMENT '总收入' ,
`status`  tinyint(1) NOT NULL COMMENT '0正常1已禁用' ,
`prate`  int(10) NOT NULL COMMENT '充值分成(万分)' ,
`crate`  int(10) NOT NULL COMMENT '抽成分成(万分)' ,
`ptime`  int(10) NOT NULL COMMENT '录入时间' ,
`aname`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '登录名' ,
PRIMARY KEY (`prid`),
INDEX `aid` USING BTREE (`aid`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='推广员表'

;

-- ----------------------------
-- Table structure for `promoteruser`
-- ----------------------------
DROP TABLE IF EXISTS `promoteruser`;
CREATE TABLE `promoteruser` (
`prid`  int(10) NOT NULL COMMENT '推广员id' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`ptime`  int(10) NOT NULL COMMENT '绑定时间' ,
PRIMARY KEY (`prid`, `uid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='推广员用户关系表'

;

-- ----------------------------
-- Table structure for `province`
-- ----------------------------
DROP TABLE IF EXISTS `province`;
CREATE TABLE `province` (
`id`  int(11) NOT NULL AUTO_INCREMENT ,
`provinceid`  int(11) NOT NULL ,
`province`  varchar(20) CHARACTER SET gbk COLLATE gbk_chinese_ci NOT NULL ,
PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=gbk COLLATE=gbk_chinese_ci
AUTO_INCREMENT=35

;

-- ----------------------------
-- Table structure for `richerpay`
-- ----------------------------
DROP TABLE IF EXISTS `richerpay`;
CREATE TABLE `richerpay` (
`orderid`  varchar(35) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'sdk订单号' ,
`username`  varchar(30) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'sdk登陆账号' ,
`gameid`  int(11) NOT NULL COMMENT '游戏id' ,
`roleid`  int(11) NOT NULL COMMENT '游戏角色id' ,
`serverid`  int(11) NOT NULL COMMENT '服务器id' ,
`paytype`  varchar(10) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '支付类型' ,
`amount`  int(11) NOT NULL COMMENT '成功支付金额单位元' ,
`paytime`  int(10) NOT NULL COMMENT '充值时间' ,
`attach`  varchar(10) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '商户拓展参数(订单号pid)' ,
`sign`  varchar(32) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '参数签名' ,
PRIMARY KEY (`orderid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='富豪发货订单数据'

;

-- ----------------------------
-- Table structure for `temp`
-- ----------------------------
DROP TABLE IF EXISTS `temp`;
CREATE TABLE `temp` (
`uid`  int(10) NOT NULL COMMENT '用户ID表' ,
`status`  int(2) NOT NULL COMMENT '是否使用0-未使用，1-使用' ,
`isgood`  int(2) NOT NULL COMMENT '是否是靓号:0-否，1-是' 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `uidstock0`
-- ----------------------------
DROP TABLE IF EXISTS `uidstock0`;
CREATE TABLE `uidstock0` (
`uid`  int(10) NOT NULL COMMENT '用户id' ,
PRIMARY KEY (`uid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `uidstock1`
-- ----------------------------
DROP TABLE IF EXISTS `uidstock1`;
CREATE TABLE `uidstock1` (
`uid`  int(10) NOT NULL COMMENT '用户id' ,
PRIMARY KEY (`uid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `usernumbers`
-- ----------------------------
DROP TABLE IF EXISTS `usernumbers`;
CREATE TABLE `usernumbers` (
`uid`  int(10) NOT NULL COMMENT '用户ID表' ,
`status`  int(2) NOT NULL COMMENT '是否使用0-未使用，1-使用' ,
`isgood`  int(2) NOT NULL COMMENT '是否是靓号:0-否，1-是' ,
PRIMARY KEY (`uid`),
INDEX `status` USING BTREE (`status`) ,
INDEX `isgood` USING BTREE (`isgood`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户ID集合表'

;

-- ----------------------------
-- Table structure for `userproduct`
-- ----------------------------
DROP TABLE IF EXISTS `userproduct`;
CREATE TABLE `userproduct` (
`id`  int(10) NOT NULL AUTO_INCREMENT COMMENT '订单id' ,
`pname`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '商品名称' ,
`uid`  int(10) NOT NULL COMMENT '用户id' ,
`pid`  int(10) NOT NULL COMMENT '商品id' ,
`qty`  int(10) NOT NULL COMMENT '购买数量' ,
`price`  int(10) NOT NULL COMMENT '购买单价' ,
`amount`  int(10) NOT NULL COMMENT '总额' ,
`status`  tinyint(1) NOT NULL COMMENT '状态(1已下单2发货99已收货)' ,
`extime`  int(10) NOT NULL COMMENT '下单时间' ,
PRIMARY KEY (`id`),
INDEX `uid` USING BTREE (`uid`) ,
INDEX `pid` USING BTREE (`pid`) ,
INDEX `extime` USING BTREE (`extime`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='用户兑换记录表'
AUTO_INCREMENT=7

;

-- ----------------------------
-- Procedure structure for `sp_add_usernumbers`
-- ----------------------------
DROP PROCEDURE IF EXISTS `sp_add_usernumbers`;
DELIMITER ;;
CREATE DEFINER=`root`@`localhost` PROCEDURE `sp_add_usernumbers`()
    MODIFIES SQL DATA
BEGIN 
 set @i=0; 
while @i<20000000 
do 
insert into usernumbers values (@i+1,0,0);
set @i = @i + 1; 
end while;
end
;;
DELIMITER ;

-- ----------------------------
-- Auto increment value for `area`
-- ----------------------------
ALTER TABLE `area` AUTO_INCREMENT=3145;

-- ----------------------------
-- Auto increment value for `bankinfo`
-- ----------------------------
ALTER TABLE `bankinfo` AUTO_INCREMENT=80;

-- ----------------------------
-- Auto increment value for `bcpay`
-- ----------------------------
ALTER TABLE `bcpay` AUTO_INCREMENT=181;

-- ----------------------------
-- Auto increment value for `city`
-- ----------------------------
ALTER TABLE `city` AUTO_INCREMENT=346;

-- ----------------------------
-- Auto increment value for `cuttrans`
-- ----------------------------
ALTER TABLE `cuttrans` AUTO_INCREMENT=1;

-- ----------------------------
-- Auto increment value for `pay`
-- ----------------------------
ALTER TABLE `pay` AUTO_INCREMENT=100002;

-- ----------------------------
-- Auto increment value for `paysite`
-- ----------------------------
ALTER TABLE `paysite` AUTO_INCREMENT=418;

-- ----------------------------
-- Auto increment value for `product`
-- ----------------------------
ALTER TABLE `product` AUTO_INCREMENT=19;

-- ----------------------------
-- Auto increment value for `province`
-- ----------------------------
ALTER TABLE `province` AUTO_INCREMENT=35;

-- ----------------------------
-- Auto increment value for `userproduct`
-- ----------------------------
ALTER TABLE `userproduct` AUTO_INCREMENT=7;
