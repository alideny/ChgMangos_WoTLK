/*
Navicat MySQL Data Transfer

Source Server         : mangos
Source Server Version : 50509
Source Host           : localhost:3306
Source Database       : auth

Target Server Type    : MYSQL
Target Server Version : 50509
File Encoding         : 65001

Date: 2011-10-28 17:02:56
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `account_vip`
-- ----------------------------
DROP TABLE IF EXISTS `account_vip`;
CREATE TABLE `account_vip` (
  `id` int(11) unsigned NOT NULL,
  `vip` int(3) unsigned NOT NULL DEFAULT '0',
  `point` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

-- ----------------------------
-- Records of account_vip
-- ----------------------------
