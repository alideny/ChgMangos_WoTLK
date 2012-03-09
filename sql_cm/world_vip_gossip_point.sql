ALTER TABLE `gossip_menu_option`
ADD COLUMN `box_point`  int(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `box_money`;

DELETE FROM `mangos_string` WHERE entry IN (1412,1413);
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1412', '|cFFFF0000你的 VIP 等级不够使用该项服务！|r');
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES ('1413', '|cFFFF0000你的积分不够使用该项服务！|r');