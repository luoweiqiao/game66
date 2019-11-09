/*
Navicat MySQL Data Transfer

Source Server         : 趣味1
Source Server Version : 50173
Source Host           : 120.24.54.6:3307
Source Database       : chess_backsite

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 65001

Date: 2017-01-18 16:53:46
*/

SET FOREIGN_KEY_CHECKS=0;

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
PRIMARY KEY (`aid`),
UNIQUE INDEX `aname` USING BTREE (`aname`) 
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=47

;

-- ----------------------------
-- Table structure for `adminmenu`
-- ----------------------------
DROP TABLE IF EXISTS `adminmenu`;
CREATE TABLE `adminmenu` (
`id`  int(11) NOT NULL AUTO_INCREMENT ,
`name`  varchar(250) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '菜单名' ,
`url`  varchar(250) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'url地址' ,
`pid`  int(10) NOT NULL COMMENT '父结点id' ,
`ispage`  int(10) NOT NULL COMMENT '是不是页面' ,
`iconCls`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '链接图标' ,
`online`  tinyint(4) NOT NULL COMMENT '是否线上，0:不在线上，1:在线上' ,
`isclose`  tinyint(2) NOT NULL COMMENT '默认是否关闭' ,
`menuorder`  int(5) NOT NULL COMMENT '排序' ,
PRIMARY KEY (`id`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=84

;

-- ----------------------------
-- Table structure for `adminmenucfg`
-- ----------------------------
DROP TABLE IF EXISTS `adminmenucfg`;
CREATE TABLE `adminmenucfg` (
`McfgID`  int(10) UNSIGNED NOT NULL AUTO_INCREMENT ,
`McfgType`  int(10) UNSIGNED NULL DEFAULT NULL COMMENT '类型 1:角色, 2:人' ,
`McfgOID`  int(10) UNSIGNED NULL DEFAULT NULL COMMENT '角色ID或人ID' ,
`McfgMenus`  varchar(800) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '菜单列表' ,
`McfgStatus`  int(10) UNSIGNED NULL DEFAULT NULL COMMENT '状态' ,
`CreateTime`  int(10) UNSIGNED NULL DEFAULT NULL COMMENT '创建时间' ,
`UpdateTime`  int(10) UNSIGNED NULL DEFAULT NULL COMMENT '修改时间' ,
PRIMARY KEY (`McfgID`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=12

;

-- ----------------------------
-- Table structure for `adminrole`
-- ----------------------------
DROP TABLE IF EXISTS `adminrole`;
CREATE TABLE `adminrole` (
`RoleID`  int(10) UNSIGNED NOT NULL AUTO_INCREMENT ,
`RoleName`  varchar(100) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '角色名称' ,
`RoleDesc`  varchar(300) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '角色描述' ,
`RoleStatus`  int(10) UNSIGNED NULL DEFAULT 0 COMMENT '角色状态(0正常1禁用)' ,
PRIMARY KEY (`RoleID`)
)
ENGINE=MyISAM
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=12

;

-- ----------------------------
-- Table structure for `menufield`
-- ----------------------------
DROP TABLE IF EXISTS `menufield`;
CREATE TABLE `menufield` (
`menuid`  int(10) NOT NULL COMMENT 'ad' ,
`fld`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '字段名' ,
`title`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '字段标题' ,
`width`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '宽度可以是px也可以是%' ,
`edittype`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '编辑类型(validatebox|combobox|datebox)' ,
`issearch`  tinyint(1) NOT NULL COMMENT '是否加入搜索条件' ,
`searchtype`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '搜索类型(=,like)' ,
`required`  tinyint(1) NOT NULL COMMENT '是否必填' ,
`display`  varchar(2) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '显示方式(r只读,w读写,h隐藏域,n不显示)' ,
`isprimary`  tinyint(2) NOT NULL COMMENT '是否是主键' ,
UNIQUE INDEX `menuid` USING BTREE (`menuid`, `fld`) 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Table structure for `menuinfo`
-- ----------------------------
DROP TABLE IF EXISTS `menuinfo`;
CREATE TABLE `menuinfo` (
`menuid`  int(10) NOT NULL COMMENT 'adminmenu.id' ,
`title`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '菜单名' ,
`menutype`  tinyint(2) NOT NULL COMMENT '1左列表12左列表右编辑框|2左编辑框' ,
`west`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '布局west' ,
`lefttitle`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '左边框标题' ,
`righttitle`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '右边框标题' ,
`actiontype`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '可操作类型(amd)' ,
`db`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '数据库名' ,
`tbl`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '表名' ,
PRIMARY KEY (`menuid`)
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci

;

-- ----------------------------
-- Auto increment value for `admin`
-- ----------------------------
ALTER TABLE `admin` AUTO_INCREMENT=47;

-- ----------------------------
-- Auto increment value for `adminmenu`
-- ----------------------------
ALTER TABLE `adminmenu` AUTO_INCREMENT=84;

-- ----------------------------
-- Auto increment value for `adminmenucfg`
-- ----------------------------
ALTER TABLE `adminmenucfg` AUTO_INCREMENT=12;

-- ----------------------------
-- Auto increment value for `adminrole`
-- ----------------------------
ALTER TABLE `adminrole` AUTO_INCREMENT=12;
