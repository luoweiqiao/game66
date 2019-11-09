/*
Navicat MySQL Data Transfer

Source Server         : 趣味1
Source Server Version : 50173
Source Host           : 120.24.54.6:3307
Source Database       : chess_sysdata

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 65001

Date: 2017-01-18 17:10:05
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `activity`
-- ----------------------------
DROP TABLE IF EXISTS `activity`;
CREATE TABLE `activity` (
`acid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '活动id' ,
`atype`  int(10) NOT NULL DEFAULT 1 COMMENT '活动类型' ,
`sortno`  int(10) NOT NULL COMMENT '排序' ,
`eventno`  int(10) NOT NULL COMMENT '事件编号' ,
`name`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '活动名称' ,
`desc`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '活动描述' ,
`content`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '活动配置信息' ,
`btime`  int(10) NOT NULL COMMENT '开始时间(0表示立即开启)' ,
`etime`  int(10) NOT NULL COMMENT '结束时间(0表示永久)' ,
`imageurl`  varchar(200) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '活动图片地址' ,
`status`  tinyint(1) NOT NULL COMMENT '状态(0不生效1生效)' ,
PRIMARY KEY (`acid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=8

;

-- ----------------------------
-- Records of activity
-- ----------------------------
BEGIN;
INSERT INTO `activity` VALUES ('1', '1', '1', '1', '充值赠送', '充值赠送', '{\"srate\":0.2,\"gc\":15,\"pid\":0}', '1467907200', '1487088000', 'activity/a3.png', '1'), ('4', '3', '3', '3', '首冲翻倍', '翻倍', '{\"srate\":1,\"gc\":0,\"pid\":1}', '1467907200', '1486137600', 'activity/a3.png', '1'), ('5', '2', '2', '2', '救济金', '救济金', '{\"srate\":1,\"gc\":5,\"pid\":0,\"times\":1}', '1467907200', '1486137600', 'activity/a3.png', '1');
COMMIT;

-- ----------------------------
-- Table structure for `agent`
-- ----------------------------
DROP TABLE IF EXISTS `agent`;
CREATE TABLE `agent` (
`agid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '代理商id' ,
`name`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '名称' ,
PRIMARY KEY (`agid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='代理商'
AUTO_INCREMENT=5

;

-- ----------------------------
-- Records of agent
-- ----------------------------
BEGIN;
INSERT INTO `agent` VALUES ('1', '66游艺');
COMMIT;

-- ----------------------------
-- Table structure for `cateconfig`
-- ----------------------------
DROP TABLE IF EXISTS `cateconfig`;
CREATE TABLE `cateconfig` (
`cate`  int(10) NOT NULL AUTO_INCREMENT COMMENT '统计分类' ,
`type`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL DEFAULT 'daycount' COMMENT '统计类型' ,
`name`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '统计名称' ,
`mtype`  varchar(2) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL DEFAULT '+' COMMENT '计算方法' ,
`dtype`  varchar(100) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '展示公式(各统计类型用<>包含)' ,
`display`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL DEFAULT 'daycount' COMMENT '显示在后台什么地方' ,
PRIMARY KEY (`cate`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='统计类型表'
AUTO_INCREMENT=22

;

-- ----------------------------
-- Records of cateconfig
-- ----------------------------
BEGIN;
INSERT INTO `cateconfig` VALUES ('14', 'daycount', '日订单数', '+', '<4>+<8>', 'daycount'), ('1', 'daycount', '日注册', '+', '', 'daycount'), ('8', 'daycount', '日活跃订单数', '+', '', 'daycount'), ('2', 'daycount', '昨注回头', '+', '', 'daycount'), ('9', 'daycount', '日活跃付费额', '+', '', 'daycount'), ('6', 'daycount', '日活跃', '+', '', 'daycount'), ('21', 'daycount', '日总活跃', '+', '<1>+<6>', 'daycount'), ('15', 'daycount', '日付费额', '+', '<5>+<9>', 'daycount'), ('16', 'method', '注册付费率%', '', 'round(<3>/<1>*100,2)', 'daycount'), ('13', 'daycount', '日付费用户', '+', '<3>+<7>', 'daycount'), ('17', 'daycount', '活跃付费率%', '', 'round(<7>/<6>*100,2)', 'daycount'), ('10', 'daycount', '累计注册', '=', '', 'daycount'), ('12', 'daycount', '累计付费额', '=', '', 'daycount'), ('4', 'daycount', '注册订单数', '+', '', 'daycount'), ('20', 'daycount', '最高在玩', '=', '', 'daycount'), ('18', 'daycount', '日付费率%', '', 'round(<13>/<10>*100,2)', 'daycount'), ('5', 'daycount', '注册付费额', '+', '', 'daycount'), ('11', 'daycount', '累计订单数', '=', '', 'daycount'), ('3', 'daycount', '注册付费人数', '+', '', 'daycount'), ('7', 'daycount', '日活跃付费', '+', '', 'daycount'), ('19', 'daycount', '最高在线', '=', '', 'daycount');
COMMIT;

-- ----------------------------
-- Table structure for `channel`
-- ----------------------------
DROP TABLE IF EXISTS `channel`;
CREATE TABLE `channel` (
`cnid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '渠道id' ,
`name`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '名称' ,
PRIMARY KEY (`cnid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=7

;

-- ----------------------------
-- Records of channel
-- ----------------------------
BEGIN;
INSERT INTO `channel` VALUES ('1', '66游艺自营（Android）');
COMMIT;

-- ----------------------------
-- Table structure for `coinprice`
-- ----------------------------
DROP TABLE IF EXISTS `coinprice`;
CREATE TABLE `coinprice` (
`id`  int(10) NOT NULL AUTO_INCREMENT COMMENT 'id' ,
`siteid`  int(10) NOT NULL COMMENT '平台ID' ,
`coin`  int(10) NOT NULL COMMENT '筹码' ,
`rmb`  int(10) NOT NULL COMMENT '人民币' ,
`ptype`  int(1) NOT NULL COMMENT '0其它1苹果' ,
`spid`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '平台商品id' ,
`status`  int(2) NOT NULL COMMENT '是否下架' ,
`gtype`  int(2) NOT NULL COMMENT '是否首充' ,
PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='商城信息表'
AUTO_INCREMENT=20

;

-- ----------------------------
-- Records of coinprice
-- ----------------------------
BEGIN;
INSERT INTO `coinprice` VALUES ('1', '0', '1000', '1000', '0', '', '0', '1'), ('2', '0', '5000', '5000', '0', '', '0', '0'), ('3', '0', '10000', '10000', '0', '', '0', '0'), ('4', '0', '30000', '30000', '0', '', '0', '0'), ('5', '0', '50000', '50000', '0', '', '0', '0'), ('6', '0', '100000', '100000', '0', '', '0', '0');
COMMIT;

-- ----------------------------
-- Table structure for `diamondprice`
-- ----------------------------
DROP TABLE IF EXISTS `diamondprice`;
CREATE TABLE `diamondprice` (
`id`  int(10) NOT NULL AUTO_INCREMENT COMMENT 'id' ,
`siteid`  int(10) NOT NULL COMMENT '平台ID' ,
`diamond`  int(10) NOT NULL COMMENT '钻石' ,
`rmb`  int(10) NOT NULL COMMENT '人民币' ,
`ptype`  int(1) NOT NULL COMMENT '0其它1苹果' ,
`spid`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '平台商品id' ,
PRIMARY KEY (`id`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=47

;

-- ----------------------------
-- Records of diamondprice
-- ----------------------------
BEGIN;
COMMIT;

-- ----------------------------
-- Table structure for `emailfull`
-- ----------------------------
DROP TABLE IF EXISTS `emailfull`;
CREATE TABLE `emailfull` (
`efid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '邮件id' ,
`agid`  int(10) NOT NULL COMMENT '代理商id' ,
`cnid`  int(10) NOT NULL COMMENT '渠道id' ,
`title`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '标题' ,
`content`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '内容' ,
`attachment`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '附件' ,
`nickname`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '来自' ,
`ptime`  int(10) NOT NULL COMMENT '发布时间' ,
`expiretime`  int(10) NOT NULL COMMENT '过期时间' ,
`status`  tinyint(1) NOT NULL COMMENT '状态(0正常,1下架)' ,
PRIMARY KEY (`efid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='全服邮件模板'
AUTO_INCREMENT=23

;

-- ----------------------------
-- Records of emailfull
-- ----------------------------
BEGIN;
COMMIT;

-- ----------------------------
-- Table structure for `functionfbd`
-- ----------------------------
DROP TABLE IF EXISTS `functionfbd`;
CREATE TABLE `functionfbd` (
`id`  int(10) NOT NULL AUTO_INCREMENT ,
`function`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '功能' ,
`optype`  int(2) NOT NULL DEFAULT 1 COMMENT '操作类型(1逻辑与|2逻辑或)' ,
`version`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '版本号' ,
`siteid`  int(10) NOT NULL COMMENT '平台id' ,
`remark`  varchar(200) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '备注' ,
PRIMARY KEY (`id`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='功能禁用'
AUTO_INCREMENT=7

;

-- ----------------------------
-- Records of functionfbd
-- ----------------------------
BEGIN;
INSERT INTO `functionfbd` VALUES ('4', 'exchangeno', '1', '1.0.0.9', '0', '1');
COMMIT;

-- ----------------------------
-- Table structure for `gameinfo`
-- ----------------------------
DROP TABLE IF EXISTS `gameinfo`;
CREATE TABLE `gameinfo` (
`gid`  tinyint(2) NOT NULL AUTO_INCREMENT COMMENT '游戏id' ,
`gname`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '游戏名称' ,
`packname`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '包名' ,
`status`  tinyint(1) NOT NULL COMMENT '状态(0不显示1已开放2不开放)' ,
`num`  int(11) NOT NULL COMMENT '在玩人数' ,
`ver`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '版本号' ,
`cover`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '封面图片地址' ,
`fbdversion`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '禁止版本' ,
`fbdsite`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '禁止平台' ,
`sortno`  int(11) NOT NULL COMMENT '排序字段' ,
PRIMARY KEY (`gid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='游戏配置列表'
AUTO_INCREMENT=3

;

-- ----------------------------
-- Records of gameinfo
-- ----------------------------
BEGIN;
INSERT INTO `gameinfo` VALUES ('1', '斗地主', 'landload', '1', '100', '1.0', 'http://image3.5253.com/images/game/00/00/24/12/2412_3.jpeg', '1.2,1.3', '2,3', '1'), ('2', '梭哈', 'showhands', '1', '200', '1.0', 'http://image3.5253.com/images/game/00/00/24/12/2412_3.jpeg', '1.5,1.4', '4,5', '2');
COMMIT;

-- ----------------------------
-- Table structure for `gameinfofbd`
-- ----------------------------
DROP TABLE IF EXISTS `gameinfofbd`;
CREATE TABLE `gameinfofbd` (
`id`  int(10) NOT NULL AUTO_INCREMENT ,
`devicetype`  int(2) NOT NULL DEFAULT 2 COMMENT '平台类型，1-苹果，2-安卓' ,
`version`  varchar(10) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '版本信息' ,
`siteid`  int(10) NOT NULL COMMENT '平台信息' ,
`gametypes`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '游戏类型集合' ,
`remark`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '备注' ,
PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=12

;

-- ----------------------------
-- Records of gameinfofbd
-- ----------------------------
BEGIN;
INSERT INTO `gameinfofbd` VALUES ('1', '2', '1.0.0.0', '501', '[1,3,5]', ''), ('2', '2', '2.0.0.0', '502', '[2,3,4,5]', ''), ('10', '2', '1.0.1.0', '1001', '[1,5,3,4,6,2]', ''), ('4', '2', '0', '0', '[1,5,3,4,6,2,7,8,9]', ''), ('5', '2', '1.0.0.8', '1001', '[1,5,3,4,6,2]', ''), ('11', '2', '1.0.0.9', '0', '[1,5,3,4,6,2,7,8,9]', '');
COMMIT;

-- ----------------------------
-- Table structure for `horn`
-- ----------------------------
DROP TABLE IF EXISTS `horn`;
CREATE TABLE `horn` (
`hid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '喇叭id' ,
`name`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '喇叭发送者昵称' ,
`msg`  varchar(90) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '消息' ,
`btime`  int(10) NOT NULL COMMENT '开始时间' ,
`etime`  int(10) NOT NULL COMMENT '结束时间' ,
`intervals`  int(10) NOT NULL COMMENT '发送间隔(秒)' ,
`wagid`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '代理商白名单' ,
`bagid`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '代理商黑名单' ,
`wcnid`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '渠道白名单' ,
`bcnid`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '渠道黑名单' ,
`status`  tinyint(2) NOT NULL COMMENT '状态(0开放1禁止)' ,
PRIMARY KEY (`hid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='后台发喇叭'
AUTO_INCREMENT=42

;

-- ----------------------------
-- Records of horn
-- ----------------------------
BEGIN;
INSERT INTO `horn` VALUES ('40', '', '请大家文明娱乐，严禁赌博，如发现有赌博行为，将封停账号，并向公安机关举报！其他内容请关注唯一官方微信：XXXXXXX，其它均为假冒。代理请加微信XXXXXXX、XXXXXXXX\\X', '1473648911', '1473735313', '16', '[]', '[]', '[]', '[]', '0'), ('41', '系统广播', 'ceshiccewfsdffsdf', '1481110819', '1481111122', '15', '[]', '[]', '[]', '[]', '0');
COMMIT;

-- ----------------------------
-- Table structure for `levelconfig`
-- ----------------------------
DROP TABLE IF EXISTS `levelconfig`;
CREATE TABLE `levelconfig` (
`level`  int(11) NOT NULL ,
`name`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '名称' ,
`needcoin`  int(10) NOT NULL COMMENT '所需财富' ,
`rmb`  int(10) NOT NULL COMMENT 'rmb衡量' ,
PRIMARY KEY (`level`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Records of levelconfig
-- ----------------------------
BEGIN;
INSERT INTO `levelconfig` VALUES ('1', '学生', '1000', '1'), ('2', '硕士', '2000', '2'), ('3', '博士', '5000', '5'), ('4', '白领', '10000', '10'), ('5', '主管', '20000', '20'), ('6', '部门经理', '50000', '50'), ('7', '区域经理', '100000', '100'), ('8', '总监', '200000', '200'), ('9', '副总裁', '500000', '500'), ('10', '高级副总裁', '1000000', '1000'), ('11', '首席执行官', '2000000', '2000'), ('12', '董事长', '5000000', '5000'), ('13', '干部', '10000000', '10000'), ('14', '处长', '20000000', '20000'), ('15', '局长', '50000000', '50000'), ('16', '副市长', '100000000', '100000'), ('17', '市长', '120000000', '120000'), ('18', '省长', '140000000', '140000'), ('19', '委员', '160000000', '160000'), ('20', '总理', '180000000', '180000'), ('21', '副主席', '200000000', '200000'), ('22', '主席', '250000000', '250000');
COMMIT;

-- ----------------------------
-- Table structure for `mission`
-- ----------------------------
DROP TABLE IF EXISTS `mission`;
CREATE TABLE `mission` (
`msid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '任务id' ,
`type`  int(5) NOT NULL COMMENT '动作类型' ,
`status`  tinyint(1) NOT NULL COMMENT '0正常1已禁用' ,
`auto`  tinyint(1) NOT NULL COMMENT '是否自动完成' ,
`title`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '标题' ,
`desc`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '描述' ,
`cate1`  int(11) NOT NULL COMMENT '分类1' ,
`cate2`  int(11) NOT NULL COMMENT '分类2' ,
`cate3`  int(11) NOT NULL COMMENT '分类3' ,
`cate4`  int(11) NOT NULL COMMENT '分类4' ,
`mtimes`  int(11) NOT NULL COMMENT '达到次数' ,
`straight`  int(11) NOT NULL COMMENT '是否连续' ,
`cycle`  tinyint(1) NOT NULL DEFAULT 1 COMMENT '周期(1每日2每周3每月)' ,
`cycletimes`  int(5) NOT NULL DEFAULT 1 COMMENT '可完成次数' ,
`icon`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '图标' ,
PRIMARY KEY (`msid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='任务配置表'
AUTO_INCREMENT=11

;

-- ----------------------------
-- Records of mission
-- ----------------------------
BEGIN;
INSERT INTO `mission` VALUES ('1', '101', '0', '0', '厚积薄发', '游戏对局达到30局', '1', '0', '0', '0', '30', '0', '1', '1', 'missionicon/mission1.png'), ('8', '104', '0', '1', '姿势优美', '每局至少成功压制对手1次', '1', '0', '0', '0', '1', '0', '1', '100', 'missionicon/mission8.png'), ('2', '102', '0', '0', '崭露头角', '赢的局数达到30局', '1', '0', '0', '0', '30', '0', '1', '1', 'missionicon/mission2.png'), ('6', '104', '0', '0', '姿势强势', '压制对手次数累计80次', '1', '0', '0', '0', '80', '0', '1', '1', 'missionicon/mission6_1.png'), ('4', '102', '0', '0', '节节高升', '连赢6局', '1', '0', '0', '0', '6', '1', '1', '1', 'missionicon/mission4.png'), ('5', '103', '0', '0', '狙击高手', '打破产20个玩家', '1', '0', '0', '0', '20', '0', '1', '1', 'missionicon/mission5_1.png'), ('7', '103', '0', '1', '狙击手', '打破产1个玩家', '1', '0', '0', '0', '1', '0', '1', '40', 'missionicon/mission7.png');
COMMIT;

-- ----------------------------
-- Table structure for `missionaction`
-- ----------------------------
DROP TABLE IF EXISTS `missionaction`;
CREATE TABLE `missionaction` (
`type`  int(4) NOT NULL COMMENT '动作类型' ,
`name`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '动作名称' ,
PRIMARY KEY (`type`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='任务动作类型配置'

;

-- ----------------------------
-- Records of missionaction
-- ----------------------------
BEGIN;
INSERT INTO `missionaction` VALUES ('105', '抽水赢'), ('103', '打破产'), ('101', '游戏对局'), ('102', '游戏胜'), ('106', '抽水输'), ('104', '压制');
COMMIT;

-- ----------------------------
-- Table structure for `missionprize`
-- ----------------------------
DROP TABLE IF EXISTS `missionprize`;
CREATE TABLE `missionprize` (
`msid`  int(10) NOT NULL COMMENT '任务id' ,
`poid`  int(10) NOT NULL COMMENT '道具id' ,
`qty`  int(10) NOT NULL COMMENT '数量' ,
PRIMARY KEY (`msid`, `poid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='任务奖励表'

;

-- ----------------------------
-- Records of missionprize
-- ----------------------------
BEGIN;
INSERT INTO `missionprize` VALUES ('1', '3', '5'), ('3', '3', '10'), ('9', '3', '200'), ('7', '3', '5'), ('4', '3', '10'), ('2', '3', '10'), ('5', '3', '50'), ('8', '3', '1'), ('10', '3', '0'), ('6', '3', '50');
COMMIT;

-- ----------------------------
-- Table structure for `notice`
-- ----------------------------
DROP TABLE IF EXISTS `notice`;
CREATE TABLE `notice` (
`nid`  int(10) NOT NULL AUTO_INCREMENT ,
`agid`  int(10) NOT NULL COMMENT '代理商id' ,
`cnid`  int(10) NOT NULL COMMENT '渠道id' ,
`status`  tinyint(1) NOT NULL COMMENT '状态(0正常,1下架)' ,
`title`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '标题' ,
`content`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '内容' ,
`ptime`  int(10) NOT NULL COMMENT '公告时间' ,
`expiretime`  int(10) NOT NULL COMMENT '过期时间' ,
`nickname`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '来自' ,
`mtime`  int(10) NOT NULL COMMENT '最后修改时间' ,
PRIMARY KEY (`nid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='公告信息'
AUTO_INCREMENT=17

;

-- ----------------------------
-- Records of notice
-- ----------------------------
BEGIN;
INSERT INTO `notice` VALUES ('9', '0', '0', '0', '健康游戏忠告', '抵制不良游戏，拒绝盗版游戏。注意自我保护，谨防受骗上当。适度游戏益脑，沉迷游戏伤身。合理安排时间，享受健康生活。', '1468771200', '1596124800', '66游艺管理员', '1482718956'), ('16', '0', '0', '0', 'test2', 'test2javascript:void(22)', '1484292540', '0', 'test2', '1484295539'), ('15', '0', '0', '0', 'test1', 'test1fsdfs1314', '1484236800', '1484323200', 'test1', '1484295847');
COMMIT;

-- ----------------------------
-- Table structure for `roomcfg`
-- ----------------------------
DROP TABLE IF EXISTS `roomcfg`;
CREATE TABLE `roomcfg` (
`id`  int(11) NOT NULL AUTO_INCREMENT COMMENT '序号' ,
`gametype`  tinyint(2) NOT NULL COMMENT '1斗地主,2梭哈' ,
`gamesubtype`  int(11) NOT NULL COMMENT '游戏类型' ,
`roomid`  int(11) NOT NULL COMMENT '房间ID' ,
`name`  varchar(32) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '房间名' ,
`deal`  int(11) NOT NULL DEFAULT 0 COMMENT '发牌类型' ,
`consume`  int(11) NOT NULL DEFAULT 1 COMMENT '消费类型(1积分，2财富币)' ,
`entermin`  bigint(20) NOT NULL COMMENT '进入门槛' ,
`entermax`  bigint(20) NOT NULL COMMENT '进入限制' ,
`isopen`  int(11) NOT NULL COMMENT '是否开放' ,
`basescore`  int(11) NOT NULL COMMENT '底分' ,
`roomtype`  int(11) NOT NULL COMMENT '房间类型(0普通房1私人房2比赛房)' ,
`showhandnum`  int(11) NOT NULL COMMENT '显示手牌数量' ,
`tablenum`  int(11) NOT NULL DEFAULT 100 COMMENT '桌子数' ,
`marry`  int(11) NOT NULL DEFAULT 1 COMMENT '匹配方式(0不匹配，1自动匹配）' ,
`limitenter`  int(11) NOT NULL DEFAULT 1 COMMENT '限制进入' ,
`robot`  int(11) NOT NULL DEFAULT 0 COMMENT '是否机器人' ,
`showonline`  int(11) NOT NULL DEFAULT 0 COMMENT '显示在线倍数' ,
`sitdown`  int(11) NOT NULL DEFAULT 0 COMMENT '坐下携带' ,
`feetype`  int(11) NOT NULL DEFAULT 0 COMMENT '台费类型' ,
`fee`  int(11) NOT NULL DEFAULT 0 COMMENT '台费值' ,
`seatnum`  int(11) NOT NULL DEFAULT 0 COMMENT '座位数' ,
`showtype`  int(11) NOT NULL DEFAULT 0 COMMENT '显示类型' ,
`showpic`  int(11) NOT NULL DEFAULT 1 COMMENT '显示图片' ,
`robotmaxscore`  bigint(20) NOT NULL DEFAULT 10000000 COMMENT '机器人最多带入' ,
`param`  varchar(120) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT '' COMMENT '特殊参数' ,
PRIMARY KEY (`id`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=13

;

-- ----------------------------
-- Records of roomcfg
-- ----------------------------
BEGIN;
INSERT INTO `roomcfg` VALUES ('1', '5', '1', '1', '新手场', '1', '2', '2500', '1000000000', '1', '10', '0', '0', '100', '0', '1', '1', '0', '1600', '2', '500', '5', '0', '1', '9000000', '{\"cmp\":2,\"look\":1,\"maxrd\":20}'), ('2', '5', '1', '2', '初级场', '1', '2', '5000', '1000000000', '1', '20', '0', '0', '100', '0', '1', '1', '0', '3200', '2', '500', '5', '0', '2', '5000000000', '{\"cmp\":2,\"look\":1,\"maxrd\":20}'), ('3', '5', '1', '3', '中级场', '1', '2', '20000', '1000000000', '1', '50', '0', '0', '100', '0', '1', '1', '0', '8000', '2', '500', '5', '0', '3', '5000000000', '{\"cmp\":3,\"look\":2,\"maxrd\":20}'), ('4', '5', '1', '4', '高级场', '1', '2', '50000', '1000000000', '1', '100', '0', '0', '100', '0', '1', '1', '0', '16000', '2', '500', '5', '0', '4', '5000000000', '{\"cmp\":3,\"look\":2,\"maxrd\":20}'), ('5', '5', '1', '5', '大师场', '1', '2', '120000', '1000000000', '1', '500', '0', '0', '100', '0', '1', '1', '0', '80000', '2', '400', '5', '0', '5', '5000000000', '{\"cmp\":3,\"look\":2,\"maxrd\":20}'), ('6', '5', '1', '6', '土豪场', '1', '2', '240000', '1000000000', '1', '1000', '0', '0', '100', '0', '1', '1', '0', '160000', '2', '300', '5', '0', '6', '5000000000', '{\"cmp\":3,\"look\":2,\"maxrd\":20}'), ('8', '1', '1', '2', '初级场', '2', '2', '200', '2500', '1', '10', '0', '1', '100', '1', '1', '0', '0', '0', '2', '500', '0', '0', '2', '5000000000', ''), ('9', '1', '1', '3', '中级场', '3', '2', '2000', '1000000000', '1', '100', '0', '1', '100', '1', '1', '0', '0', '0', '2', '400', '0', '0', '3', '5000000000', ''), ('10', '1', '1', '4', '高级场', '3', '2', '20000', '1000000000', '1', '1000', '0', '1', '100', '1', '1', '0', '0', '0', '2', '300', '0', '0', '4', '5000000000', '');
COMMIT;

-- ----------------------------
-- Table structure for `scoreprice`
-- ----------------------------
DROP TABLE IF EXISTS `scoreprice`;
CREATE TABLE `scoreprice` (
`id`  int(10) NOT NULL AUTO_INCREMENT COMMENT 'id' ,
`score`  int(10) NOT NULL COMMENT '积分' ,
`diamond`  int(10) NOT NULL COMMENT '钻石' ,
PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=9

;

-- ----------------------------
-- Records of scoreprice
-- ----------------------------
BEGIN;
INSERT INTO `scoreprice` VALUES ('1', '200000', '100'), ('8', '20000', '10'), ('2', '400000', '200'), ('6', '20000000', '10000'), ('4', '2000000', '1000'), ('5', '10000000', '5000'), ('3', '1000000', '500'), ('7', '100000000', '50000');
COMMIT;

-- ----------------------------
-- Table structure for `signtype`
-- ----------------------------
DROP TABLE IF EXISTS `signtype`;
CREATE TABLE `signtype` (
`type`  int(5) NOT NULL AUTO_INCREMENT COMMENT '标签类型' ,
`name`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '标签名称' ,
`remark`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '备注' ,
PRIMARY KEY (`type`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='标签配置表'
AUTO_INCREMENT=3

;

-- ----------------------------
-- Records of signtype
-- ----------------------------
BEGIN;
INSERT INTO `signtype` VALUES ('1', '大R用户', '总总值大于1000'), ('2', '中R用户', '总充值在500-1000的用户');
COMMIT;

-- ----------------------------
-- Table structure for `site`
-- ----------------------------
DROP TABLE IF EXISTS `site`;
CREATE TABLE `site` (
`siteid`  int(10) NOT NULL AUTO_INCREMENT COMMENT '代理渠道联合id(1-500无sdk,501-1000有)' ,
`sitename`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '平台名称' ,
`agid`  int(10) NOT NULL COMMENT '代理' ,
`cnid`  int(10) NOT NULL COMMENT '渠道id' ,
PRIMARY KEY (`siteid`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=1002

;

-- ----------------------------
-- Records of site
-- ----------------------------
BEGIN;
INSERT INTO `site` VALUES ('1', '66游艺', '1', '1');
COMMIT;

-- ----------------------------
-- Table structure for `sysconfig`
-- ----------------------------
DROP TABLE IF EXISTS `sysconfig`;
CREATE TABLE `sysconfig` (
`sckey`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'key' ,
`value`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '值' ,
`mean`  text CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '意义' ,
`mkfile`  tinyint(2) NOT NULL COMMENT '是否生成配置文件' ,
`vtype`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '数值类型' ,
PRIMARY KEY (`sckey`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
COMMENT='各种配置表'

;

-- ----------------------------
-- Records of sysconfig
-- ----------------------------
BEGIN;
INSERT INTO `sysconfig` VALUES ('proomprice', '500', '私人房日租金', '1', 'int'), ('zdshareinfo', '{\"title\":\"中大斗地主\",\"content\":\"原汁原味的线下斗地主模式，“单张发牌、五或七张连发”等多种发牌方式自选。快约上朋友一起组局吧。\",\"link\":\"http://www.zdgame.com\"}', '中大分享信息', '1', 'json'), ('bankruptbase', '4000', '低于此积分算破产', '1', 'int'), ('landbasescore', '[{\"deal\":1,\"score\":[10000,50000,100000,500000,1000000,2000000,5000000],\"entermin\":[40000,200000,400000,2000000,4000000,8000000,20000000]},{\"deal\":2,\"score\":[10000,50000,100000,500000,1000000,2000000,5000000],\"entermin\":[80000,400000,800000,4000000,8000000,16000000,40000000]},{\"deal\":3,\"score\":[10000,50000,100000,500000,1000000,2000000,5000000],\"entermin\":[320000,1600000,3200000,16000000,32000000,64000000,160000000]}]', '斗地主私人房底分配置\r\n{\"发牌类型\":底注金额数组}', '1', 'json'), ('pltype', '[{\"type\":1,\"name\":\"平和型\"},{\"type\":2,\"name\":\"激进型\"},{\"type\":3,\"name\":\"保守型\"}]', '机器人打牌风格', '0', 'json'), ('wloginreward', '[0,0,10,0,20,50,0]', '每周累计登陆奖励', '1', 'json'), ('shbasescore', '[500,1000,2000,5000,10000,20000,50000]', '梭哈私人房底分配置\r\n[底分....]', '1', 'json'), ('showhandbasescore', '[{\"min\":4000,\"max\":500000,\"score\":1024,\"pos\":0},{\"min\":500000,\"max\":1000000,\"score\":4096,\"pos\":1},{\"min\":1000000,\"max\":100000000000,\"score\":16384,\"pos\":2}]', '梭哈底注计算系数', '0', ''), ('giveviptax', '2', 'VIP玩家赠送税率', '0', 'int'), ('landenterparam', '[{\"deal\":1,\"param\":4},{\"deal\":2,\"param\":8},{\"deal\":3,\"param\":16}]', '发牌类型对应系数计算最小带入', '0', ''), ('shminbuyin', '[100000,500000,1000000,5000000,10000000,100000000,200000000]', '梭哈私人房最小携带\r\n[带....]', '1', 'json'), ('proomfee', '[{\"feetype\":0,\"fee\":0,\"desc\":\"不收台费\"},{\"feetype\":1,\"fee\":100,\"desc\":\"每人:底分x1%\"},{\"feetype\":1,\"fee\":200,\"desc\":\"每人:底分x2%\"},{\"feetype\":1,\"fee\":500,\"desc\":\"每人:底分x5%\"},{\"feetype\":1,\"fee\":1000,\"desc\":\"每人:底分x10%\"},{\"feetype\":2,\"fee\":50,\"desc\":\"赢家0.5%\"},{\"feetype\":2,\"fee\":1,\"desc\":\"赢家1%\"},{\"feetype\":2,\"fee\":150,\"desc\":\"赢家1.5%\"},{\"feetype\":2,\"fee\":200,\"desc\":\"赢家2%\"}]', 'feetype:抽成类型(0不抽1每人按底分百分比抽2赢家所赢百分比抽)', '1', 'json'), ('roll', '{\"cost\":{\"poid\":3,\"qty\":100},\"prize\":[{\"rate\":30,\"poid\":3,\"qty\":10,\"imageurl\":\"activity/rollprize3.png\",\"desc\":\"10个元宝\"},{\"rate\":15,\"poid\":3,\"qty\":20,\"imageurl\":\"activity/rollprize3.png\",\"desc\":\"20元宝\"},{\"rate\":10,\"poid\":3,\"qty\":50,\"imageurl\":\"activity/rollprize3.png\",\"desc\":\"50元宝\"},{\"rate\":25,\"poid\":3,\"qty\":100,\"imageurl\":\"activity/rollprize3.png\",\"desc\":\"100元宝\"},{\"rate\":10,\"poid\":2,\"qty\":1,\"imageurl\":\"activity/rollprize2.png\",\"desc\":\"1个财富币\"},{\"rate\":5,\"poid\":2,\"qty\":2,\"imageurl\":\"activity/rollprize2.png\",\"desc\":\"2个财富币\"},{\"rate\":4,\"poid\":2,\"qty\":5,\"imageurl\":\"activity/rollprize2.png\",\"desc\":\"5个财富币\"},{\"rate\":1,\"poid\":2,\"qty\":10,\"imageurl\":\"activity/rollprize2.png\",\"desc\":\"10个财富币\"}]}', '大转盘配置', '1', 'json'), ('broadcast', '{\"content\":\"为了维护公平的游戏环境，官方严厉打击游戏内双开、伙牌等作弊行为。欢迎各位玩家举报，一经核实严肃处理。官方QQ：3486184150，3303040027；官方微信：QWAAA003。\",\"time\":15}', '系统广播', '1', 'json'), ('bankruptvalue', '10000', '破产补助积分', '1', 'int'), ('texasminbuyin', '[100000,500000,1000000,5000000,10000000,500000000,100000000]', '德州最小进入', '1', 'json'), ('jumpqueue', '2000', '百牛插队费用', '1', 'int'), ('speakcost', '100', '小喇叭多少财富币', '1', 'int'), ('bankruptcount', '3', '破产补助次数', '1', 'int'), ('richlv', '[{\"type\":0,\"name\":\"等级0\"},{\"type\":1,\"name\":\"等级1\"},{\"type\":2,\"name\":\"等级2\"},{\"type\":3,\"name\":\"等级3\"},{\"type\":4,\"name\":\"等级4\"},{\"type\":5,\"name\":\"等级5\"},{\"type\":6,\"name\":\"等级6\"},{\"type\":7,\"name\":\"等级7\"},{\"type\":8,\"name\":\"等级8\"},{\"type\":9,\"name\":\"等级9\"},{\"type\":10,\"name\":\"等级10\"}]', '财富等级', '0', 'json'), ('texasbasescore', '[1000,5000,10000,50000,100000,500000,1000000]', '德州底分配置', '1', 'json'), ('cloginreward', '[100,100,100,100,100,100,200]', '连续登陆奖励', '1', 'json'), ('bankrupttype', '4', '破产货币4积分2财富币', '1', 'int'), ('givetax', '0', '普通玩家赠送税率', '0', 'int'), ('safeproblem', '[{\"type\":1,\"name\":\"你妈妈的名字是什么?\"},{\"type\":2,\"name\":\"我的出生地在哪?\"},{\"type\":3,\"name\":\"我的小学名字叫什么?\"},{\"type\":4,\"name\":\"我的宠物名字叫什么?\"},{\"type\":5,\"name\":\"我最喜欢的人的名字?\"},{\"type\":6,\"name\":\"我最喜欢的颜色是什么?\"}]', '安全问题设置\r\n{\"数字\":\"问题描述\"}', '1', 'json'), ('banker', '[{\"type\":1,\"name\":\"中国银行\"},{\"type\":2,\"name\":\"中国工商银行\"},{\"type\":3,\"name\":\"中国建设银行\"},{\"type\":4,\"name\":\"中国农业银行\"},{\"type\":5,\"name\":\"招商银行\"},{\"type\":6,\"name\":\"交通银行\"},{\"type\":7,\"name\":\"中信银行\"},{\"type\":8,\"name\":\"兴业银行\"},\r\n{\"type\":9,\"name\":\"中国民生银行\"},\r\n{\"type\":10,\"name\":\"广东发展银行\"},\r\n{\"type\":11,\"name\":\"中国邮政储蓄银行\"},\r\n{\"type\":12,\"name\":\"中国光大银行\"},\r\n{\"type\":13,\"name\":\"上海浦东发展银行\"},{\"type\":14,\"name\":\"北京农村商业银行\"},\r\n{\"type\":15,\"name\":\"华夏银行\"},\r\n{\"type\":16,\"name\":\"平安银行\"},\r\n{\"type\":17,\"name\":\"渤海银行\"},\r\n{\"type\":18,\"name\":\"北京银行\"},\r\n{\"type\":19,\"name\":\"南京市商业银行\"},\r\n{\"type\":20,\"name\":\"宁波银行\"},{\"type\":21,\"name\":\"东亚银行\"}]', '银行描述\r\n{\"数字\":\"银行描述\"}', '1', 'json'), ('signgamecount', '10', '每日签到游戏局数', '1', 'int');
COMMIT;

-- ----------------------------
-- Auto increment value for `activity`
-- ----------------------------
ALTER TABLE `activity` AUTO_INCREMENT=8;

-- ----------------------------
-- Auto increment value for `agent`
-- ----------------------------
ALTER TABLE `agent` AUTO_INCREMENT=5;

-- ----------------------------
-- Auto increment value for `cateconfig`
-- ----------------------------
ALTER TABLE `cateconfig` AUTO_INCREMENT=22;

-- ----------------------------
-- Auto increment value for `channel`
-- ----------------------------
ALTER TABLE `channel` AUTO_INCREMENT=7;

-- ----------------------------
-- Auto increment value for `coinprice`
-- ----------------------------
ALTER TABLE `coinprice` AUTO_INCREMENT=20;

-- ----------------------------
-- Auto increment value for `diamondprice`
-- ----------------------------
ALTER TABLE `diamondprice` AUTO_INCREMENT=47;

-- ----------------------------
-- Auto increment value for `emailfull`
-- ----------------------------
ALTER TABLE `emailfull` AUTO_INCREMENT=23;

-- ----------------------------
-- Auto increment value for `functionfbd`
-- ----------------------------
ALTER TABLE `functionfbd` AUTO_INCREMENT=7;

-- ----------------------------
-- Auto increment value for `gameinfo`
-- ----------------------------
ALTER TABLE `gameinfo` AUTO_INCREMENT=3;

-- ----------------------------
-- Auto increment value for `gameinfofbd`
-- ----------------------------
ALTER TABLE `gameinfofbd` AUTO_INCREMENT=12;

-- ----------------------------
-- Auto increment value for `horn`
-- ----------------------------
ALTER TABLE `horn` AUTO_INCREMENT=42;

-- ----------------------------
-- Auto increment value for `mission`
-- ----------------------------
ALTER TABLE `mission` AUTO_INCREMENT=11;

-- ----------------------------
-- Auto increment value for `notice`
-- ----------------------------
ALTER TABLE `notice` AUTO_INCREMENT=17;

-- ----------------------------
-- Auto increment value for `roomcfg`
-- ----------------------------
ALTER TABLE `roomcfg` AUTO_INCREMENT=13;

-- ----------------------------
-- Auto increment value for `scoreprice`
-- ----------------------------
ALTER TABLE `scoreprice` AUTO_INCREMENT=9;

-- ----------------------------
-- Auto increment value for `signtype`
-- ----------------------------
ALTER TABLE `signtype` AUTO_INCREMENT=3;

-- ----------------------------
-- Auto increment value for `site`
-- ----------------------------
ALTER TABLE `site` AUTO_INCREMENT=1002;
